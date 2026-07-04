// SPDX-FileCopyrightText: 2026 Frans van Dorsselaer
//
// SPDX-License-Identifier: GPL-3.0-only

#include "stdafx.h"

#include "driver.h"
#include "trace.h"
#include "driver.tmh"

#include "hub_device.h"
#include "child_device.h"


static EVT_WDF_OBJECT_CONTEXT_CLEANUP FilterEvtDriverContextCleanup;
#pragma alloc_text(PAGE, FilterEvtDriverContextCleanup)
_Use_decl_annotations_
static void FilterEvtDriverContextCleanup(WDFOBJECT DriverObject) {
    UNREFERENCED_PARAMETER(DriverObject);
    PAGED_CODE();
    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "%!FUNC! Entry");

    WPP_CLEANUP(WdfDriverWdmGetDriverObject((WDFDRIVER)DriverObject));
}


static DRIVER_DISPATCH DriverDispatch;
_Use_decl_annotations_
static NTSTATUS DriverDispatch(PDEVICE_OBJECT DeviceObject, PIRP Irp) {
    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "%!FUNC! major function");

    PCHILD_DEVICE_CONTEXT deviceContext = (PCHILD_DEVICE_CONTEXT)DeviceObject->DeviceExtension;
    if (deviceContext == NULL) {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_DRIVER, "%!FUNC! deviceContext is NULL");
        Irp->IoStatus.Status = STATUS_INVALID_DEVICE_REQUEST;
        Irp->IoStatus.Information = 0;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return STATUS_INVALID_DEVICE_REQUEST;
    }

    if (deviceContext->Magic == CHILD_DEVICE_MAGIC) {
        TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "%!FUNC! for WDM Child");
        return ChildDispatch(DeviceObject, Irp);
    }

    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "%!FUNC! for WDF Hub");

    PIO_STACK_LOCATION irpStack = IoGetCurrentIrpStackLocation(Irp);

    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "%!FUNC! for WDF Hub: 0x%02x", irpStack->MajorFunction);

    if (irpStack->MajorFunction > IRP_MJ_MAXIMUM_FUNCTION) {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_DRIVER, "%!FUNC! MajorFunction %d is out of range", irpStack->MajorFunction);
        Irp->IoStatus.Status = STATUS_INVALID_DEVICE_REQUEST;
        Irp->IoStatus.Information = 0;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return STATUS_INVALID_DEVICE_REQUEST;
    }

    WDFDEVICE wdfDevice = WdfWdmDeviceGetWdfDeviceHandle(DeviceObject);
    if (wdfDevice == NULL) {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_DRIVER, "%!FUNC! WdfWdmDeviceGetWdfDeviceHandle failed");
        Irp->IoStatus.Status = STATUS_INVALID_DEVICE_REQUEST;
        Irp->IoStatus.Information = 0;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return STATUS_INVALID_DEVICE_REQUEST;
    }
    WDFDRIVER driver = WdfDeviceGetDriver(wdfDevice);
    if (driver == NULL) {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_DRIVER, "%!FUNC! WdfDeviceGetDriver failed");
        Irp->IoStatus.Status = STATUS_INVALID_DEVICE_REQUEST;
        Irp->IoStatus.Information = 0;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return STATUS_INVALID_DEVICE_REQUEST;
    }
    PDRIVER_CONTEXT driverContext = DriverGetContext(driver);
    if (driverContext == NULL) {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_DRIVER, "%!FUNC! DriverGetContext failed");
        Irp->IoStatus.Status = STATUS_INVALID_DEVICE_REQUEST;
        Irp->IoStatus.Information = 0;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return STATUS_INVALID_DEVICE_REQUEST;
    }

    return driverContext->WdfDispatch[irpStack->MajorFunction](DeviceObject, Irp);
}


#pragma alloc_text(INIT, DriverEntry)
_Use_decl_annotations_
NTSTATUS DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath) {
    //
    // Required for POOL_ZERO_DOWN_LEVEL_SUPPORT (< Windows 10 2004). We do not need POOL_NX_OPTIN (< Windows 8).
    // Must be called before anything else.
    //
    ExInitializeDriverRuntime(0);

    //
    // Initialize WPP Tracing
    //
    WPP_INIT_TRACING(DriverObject, RegistryPath);

    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "%!FUNC! Entry");

    //
    // We need a cleanup callback for WPP_CLEANUP during driver unload.
    //
    WDF_OBJECT_ATTRIBUTES driverAttributes;
    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&driverAttributes, DRIVER_CONTEXT);
    driverAttributes.EvtCleanupCallback = FilterEvtDriverContextCleanup;

    WDF_DRIVER_CONFIG config;
    WDF_DRIVER_CONFIG_INIT(&config, HubCreateDevice);
    config.DriverPoolTag = POOL_TAG;

    WDFDRIVER driver;
    NTSTATUS status = WdfDriverCreate(DriverObject, RegistryPath, &driverAttributes, &config, &driver);
    if (!NT_SUCCESS(status)) {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_DRIVER, "WdfDriverCreate failed %!STATUS!", status);
        WPP_CLEANUP(DriverObject);
        return status;
    }
    // NOTE: Failure after this point can rely on FilterEvtDriverContextCleanup.

    //
    // Initialize the context.
    //
    PDRIVER_CONTEXT driverContext = DriverGetContext(driver);
    status = WdfWaitLockCreate(WDF_NO_OBJECT_ATTRIBUTES, &driverContext->HubDeviceLock);
    if (!NT_SUCCESS(status)) {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_DRIVER, "WdfWaitLockCreate failed %!STATUS!", status);
        return status;
    }
    driverContext->HubDeviceCount = 0;
    driverContext->ControlDevice = NULL;
    driverContext->GlobalTestCounter = 0;
    for (ULONG i = 0; i <= IRP_MJ_MAXIMUM_FUNCTION; i++) {
        driverContext->WdfDispatch[i] = DriverObject->MajorFunction[i];
        DriverObject->MajorFunction[i] = DriverDispatch;
    }

    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "%!FUNC! Exit");

    return STATUS_SUCCESS;
}
