// SPDX-FileCopyrightText: 2026 Frans van Dorsselaer
//
// SPDX-License-Identifier: GPL-3.0-only

#include "stdafx.h"

#include "device.h"
#include "trace.h"
#include "device.tmh"

#include "control_device.h"
#include "driver.h"


static NTSTATUS FilterQueryIdCompletion(_In_ PDEVICE_OBJECT DeviceObject, _In_ PIRP Irp, _In_ PVOID Context) {
    UNREFERENCED_PARAMETER(DeviceObject);
    UNREFERENCED_PARAMETER(Context);

    // Propagate the pending flag if necessary
    if (Irp->PendingReturned) {
        IoMarkIrpPending(Irp);
    }

    if (!NT_SUCCESS(Irp->IoStatus.Status) || (Irp->IoStatus.Information == 0)) {
        // No need to modify failed results. Note that the completion itself succeeds.
        return STATUS_SUCCESS;
    }

    PWCHAR originalBuffer = (PWCHAR)Irp->IoStatus.Information;

    PIO_STACK_LOCATION irpStack = IoGetCurrentIrpStackLocation(Irp);
    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DEVICE, "FilterQueryIdCompletion for type %d: %ws", irpStack->Parameters.QueryId.IdType, originalBuffer);

    // 1. Inspect originalBuffer to see if it is our target device.
    if (_wcsicmp(originalBuffer, L"USB\\VID_1234&PID_5678") == 0) {
        // 2. Define your new string length (Example: replacing with your targeted spoof string)
        // Be sure to account for multi-string double null-terminators if processing HardwareIDs!
        size_t newBufferLengthInBytes = 128;

        // 3. Allocate a brand new buffer from the Paged Pool
        PWCHAR newBuffer = (PWCHAR)ExAllocatePoolZero(POOL_FLAG_PAGED, newBufferLengthInBytes, POOL_TAG);

        if (newBuffer != NULL) {
            if (!NT_SUCCESS(RtlStringCbCopyW(newBuffer, newBufferLengthInBytes, L"USB\\VID_ABCD&PID_9999"))) {
                // This should never happen since we allocated enough memory, but if it does, free the new buffer and
                // return success to avoid crashing the system. The original buffer is still intact and will be used by
                // the child hub driver.
                TraceEvents(TRACE_LEVEL_ERROR, TRACE_DEVICE, "FilterQueryIdCompletion unable to overwrite ID");
                ExFreePoolWithTag(newBuffer, POOL_TAG);
                return STATUS_SUCCESS;
            }

            // 4. Free the old memory buffer returned by the child hub driver
            ExFreePool(originalBuffer);

            // 5. Replace the IRP's information pointer with your new buffer
            Irp->IoStatus.Information = (ULONG_PTR)newBuffer;
        }
    }

    return STATUS_SUCCESS;
}


static NTSTATUS FilterQueryId(_In_ WDFDEVICE Device, _Inout_ PIRP Irp) {
    PIO_STACK_LOCATION irpStack = IoGetCurrentIrpStackLocation(Irp);

    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DEVICE, "FilterQueryId for type %d", irpStack->Parameters.QueryId.IdType);

    switch (irpStack->Parameters.QueryId.IdType) {
    case BusQueryCompatibleIDs:
    case BusQueryDeviceID:
    case BusQueryHardwareIDs:
    case BusQueryInstanceID:
        // Set up the completion routine to catch the response on the way back up.
        IoCopyCurrentIrpStackLocationToNext(Irp);
        IoSetCompletionRoutine(Irp, FilterQueryIdCompletion, Device, TRUE, TRUE, TRUE);
        break;

    // case BusQueryContainerID: // not interested, GUID instead of string
    // case BusQueryDeviceSerialNumber: // reserved
    default:
        // We cannot interpret anything else. Just pass it down without interception.
        IoSkipCurrentIrpStackLocation(Irp);
    }

    return WdfDeviceWdmDispatchPreprocessedIrp(Device, Irp);
}


static EVT_WDF_DEVICE_CONTEXT_CLEANUP FilterEvtDeviceContextCleanup;
#pragma alloc_text(PAGE, FilterEvtDeviceContextCleanup)
_Use_decl_annotations_
static void FilterEvtDeviceContextCleanup(WDFOBJECT Device) {
    PAGED_CODE();

    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "%!FUNC! Entry");

    WDFDRIVER driver = WdfDeviceGetDriver((WDFDEVICE)Device);
    PDRIVER_CONTEXT driverContext = DriverGetContext(driver);
    (void)WdfWaitLockAcquire(driverContext->ControlDeviceLock, NULL);
    driverContext->ControlDeviceCount--;
    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "Count is now %d.", driverContext->ControlDeviceCount);
    if ((driverContext->ControlDeviceCount == 0) && (driverContext->ControlDevice != NULL)) {
        TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "Deleting control device.");
        WdfObjectDelete(driverContext->ControlDevice);
        driverContext->ControlDevice = NULL;
    }
    WdfWaitLockRelease(driverContext->ControlDeviceLock);

    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "%!FUNC! Exit");
}


/// <summary>
/// This is the EvtDeviceAdd callback function that creates and initializes a new instance of the device each time one is enumerated by the PnP manager.
/// </summary>
#pragma alloc_text(PAGE, FilterCreateDevice)
_Use_decl_annotations_
NTSTATUS FilterCreateDevice(PWDFDEVICE_INIT DeviceInit) {
    PAGED_CODE();

    //
    // Tell the framework that we are a filter driver.
    //
    WdfFdoInitSetFilter(DeviceInit);

    WDF_OBJECT_ATTRIBUTES deviceAttributes;
    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&deviceAttributes, DEVICE_CONTEXT);
    deviceAttributes.EvtCleanupCallback = FilterEvtDeviceContextCleanup;

    //
    // Tell KMDF to route all IRP_MJ_PNP / IRP_MN_QUERY_ID requests to our raw WDM handler.
    //
    UCHAR minorCode = IRP_MN_QUERY_ID;
    NTSTATUS status = WdfDeviceInitAssignWdmIrpPreprocessCallback(DeviceInit, FilterQueryId, IRP_MJ_PNP, &minorCode, 1);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    WDFDEVICE device;
    status = WdfDeviceCreate(&DeviceInit, &deviceAttributes, &device);
    if (!NT_SUCCESS(status)) {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_DEVICE, "WdfDeviceCreate failed %!STATUS!", status);
        return status;
    }

    //
    // Initialize the context.
    //
    PDEVICE_CONTEXT deviceContext = DeviceGetContext(device);
    deviceContext->DeviceInterfaceSymbolicLinkName = NULL;

    //
    // We need to create the control device only when the first device is created.
    //
    WDFDRIVER driver = WdfDeviceGetDriver(device);
    PDRIVER_CONTEXT driverContext = DriverGetContext(driver);
    (void)WdfWaitLockAcquire(driverContext->ControlDeviceLock, NULL);
    driverContext->ControlDeviceCount++;
    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "Count is now %d.", driverContext->ControlDeviceCount);
    if (driverContext->ControlDevice == NULL) {
        status = ControlCreateDevice(driver);
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_DEVICE, "ControlCreateDevice status %!STATUS!", status);
        // Too bad if this fails, but we will continue. The control device will not be available, but the filter device will still load correctly.
    }
    WdfWaitLockRelease(driverContext->ControlDeviceLock);

    //
    // NOTE: We don't need a queue, we only need to intercept the QueryId IRPs, which we already did above.
    //

    return STATUS_SUCCESS;
}
