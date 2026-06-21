// SPDX-FileCopyrightText: 2026 Frans van Dorsselaer
//
// SPDX-License-Identifier: GPL-3.0-only

#include "stdafx.h"

#include "driver.h"
#include "trace.h"
#include "driver.tmh"

#include "device.h"


static EVT_WDF_DRIVER_DEVICE_ADD FilterEvtDeviceAdd;
#pragma alloc_text(PAGE, FilterEvtDeviceAdd)
_Use_decl_annotations_
static NTSTATUS FilterEvtDeviceAdd(WDFDRIVER Driver, PWDFDEVICE_INIT DeviceInit) {
    UNREFERENCED_PARAMETER(Driver);
    PAGED_CODE();

    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "%!FUNC! Entry");

    NTSTATUS status = FilterCreateDevice(DeviceInit);

    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "%!FUNC! Exit");

    return status;
}


static EVT_WDF_OBJECT_CONTEXT_CLEANUP FilterEvtDriverContextCleanup;
#pragma alloc_text(PAGE, FilterEvtDriverContextCleanup)
_Use_decl_annotations_
static void FilterEvtDriverContextCleanup(WDFOBJECT DriverObject) {
    UNREFERENCED_PARAMETER(DriverObject);
    PAGED_CODE();
    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "%!FUNC! Entry");

    WPP_CLEANUP(WdfDriverWdmGetDriverObject((WDFDRIVER)DriverObject));
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
    WDF_DRIVER_CONFIG_INIT(&config, FilterEvtDeviceAdd);
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
    status = WdfWaitLockCreate(WDF_NO_OBJECT_ATTRIBUTES, &driverContext->ControlDeviceLock);
    if (!NT_SUCCESS(status)) {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_DRIVER, "WdfWaitLockCreate failed %!STATUS!", status);
        return status;
    }
    driverContext->ControlDeviceCount = 0;
    driverContext->ControlDevice = NULL;
    driverContext->GlobalTestCounter = 0;

    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "%!FUNC! Exit");

    return STATUS_SUCCESS;
}
