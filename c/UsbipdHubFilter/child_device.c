// SPDX-FileCopyrightText: 2026 Frans van Dorsselaer
//
// SPDX-License-Identifier: GPL-3.0-only

#include "stdafx.h"

#include "child_device.h"
#include "trace.h"
#include "child_device.tmh"

#include "driver.h"


static DRIVER_DISPATCH ChildDispatchStubPnp;
#pragma alloc_text(PAGE, ChildDispatchStubPnp)
_Use_decl_annotations_
static NTSTATUS ChildDispatchStubPnp(PDEVICE_OBJECT DeviceObject, PIRP Irp) {
    PAGED_CODE();

    TraceEvents(TRACE_LEVEL_VERBOSE, TRACE_CHILD_DEVICE, "%!FUNC! Entry");

    PCHILD_DEVICE_CONTEXT deviceContext = (PCHILD_DEVICE_CONTEXT)DeviceObject->DeviceExtension;
    PIO_STACK_LOCATION stackLocation = IoGetCurrentIrpStackLocation(Irp);

    static DECLARE_CONST_UNICODE_STRING(deviceIdStr, L"USB\\VID_1209&PID_8251");
    static DECLARE_CONST_UNICODE_STRING(deviceTextStr, L"USB/IP Shared Device");

    switch (stackLocation->MinorFunction) {
    case IRP_MN_QUERY_ID:
        TraceEvents(TRACE_LEVEL_VERBOSE, TRACE_CHILD_DEVICE, "%!FUNC! IRP_MN_QUERY_ID %d", stackLocation->Parameters.QueryId.IdType);

        switch (stackLocation->Parameters.QueryId.IdType) {
        case BusQueryDeviceID: {
            TraceEvents(TRACE_LEVEL_VERBOSE, TRACE_CHILD_DEVICE, "%!FUNC! BusQueryDeviceID");

            USHORT bufferLength = deviceIdStr.Length + sizeof(WCHAR);
            PWSTR deviceIdBuffer = (PWSTR)ExAllocatePoolWithTag(PagedPool, bufferLength, POOL_TAG);
            if (deviceIdBuffer == NULL) {
                Irp->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
                IoCompleteRequest(Irp, IO_NO_INCREMENT);
                return STATUS_INSUFFICIENT_RESOURCES;
            }
            RtlZeroMemory(deviceIdBuffer, bufferLength);
            RtlCopyMemory(deviceIdBuffer, deviceIdStr.Buffer, deviceIdStr.Length);
            Irp->IoStatus.Information = (ULONG_PTR)deviceIdBuffer;
            Irp->IoStatus.Status = STATUS_SUCCESS;
            IoCompleteRequest(Irp, IO_NO_INCREMENT);
            return STATUS_SUCCESS;
        }

        case BusQueryHardwareIDs: {
            TraceEvents(TRACE_LEVEL_VERBOSE, TRACE_CHILD_DEVICE, "%!FUNC! BusQueryHardwareIDs");

            USHORT bufferLength = deviceIdStr.Length + 2 * sizeof(WCHAR);
            PWSTR hardwareIdsBuffer = (PWSTR)ExAllocatePoolWithTag(PagedPool, bufferLength, POOL_TAG);
            if (hardwareIdsBuffer == NULL) {
                Irp->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
                IoCompleteRequest(Irp, IO_NO_INCREMENT);
                return STATUS_INSUFFICIENT_RESOURCES;
            }
            RtlZeroMemory(hardwareIdsBuffer, bufferLength);
            RtlCopyMemory(hardwareIdsBuffer, deviceIdStr.Buffer, deviceIdStr.Length);
            Irp->IoStatus.Information = (ULONG_PTR)hardwareIdsBuffer;
            Irp->IoStatus.Status = STATUS_SUCCESS;
            IoCompleteRequest(Irp, IO_NO_INCREMENT);
            return STATUS_SUCCESS;
        }

        case BusQueryCompatibleIDs: {
            TraceEvents(TRACE_LEVEL_VERBOSE, TRACE_CHILD_DEVICE, "%!FUNC! BusQueryCompatibleIDs");

            // return default STATUS_NOT_SUPPORTED (which is already set for every IRP_MJ_PNP)
            IoCompleteRequest(Irp, IO_NO_INCREMENT);
            return Irp->IoStatus.Status;
        }

        default:
            break;
        }
        break;

    case IRP_MN_QUERY_DEVICE_TEXT:
        TraceEvents(TRACE_LEVEL_VERBOSE, TRACE_CHILD_DEVICE, "%!FUNC! IRP_MN_QUERY_DEVICE_TEXT %d", stackLocation->Parameters.QueryDeviceText.DeviceTextType);

        switch (stackLocation->Parameters.QueryDeviceText.DeviceTextType) {
        case DeviceTextDescription: {
            TraceEvents(TRACE_LEVEL_VERBOSE, TRACE_CHILD_DEVICE, "%!FUNC! DeviceTextDescription");
            USHORT bufferLength = deviceTextStr.Length + sizeof(WCHAR);
            PWSTR deviceTextBuffer = (PWSTR)ExAllocatePoolWithTag(PagedPool, bufferLength, POOL_TAG);
            if (deviceTextBuffer == NULL) {
                Irp->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
                IoCompleteRequest(Irp, IO_NO_INCREMENT);
                return STATUS_INSUFFICIENT_RESOURCES;
            }
            RtlZeroMemory(deviceTextBuffer, bufferLength);
            RtlCopyMemory(deviceTextBuffer, deviceTextStr.Buffer, deviceTextStr.Length);
            Irp->IoStatus.Information = (ULONG_PTR)deviceTextBuffer;
            Irp->IoStatus.Status = STATUS_SUCCESS;
            IoCompleteRequest(Irp, IO_NO_INCREMENT);
            return STATUS_SUCCESS;
        }

        default:
            break;
        }
        break;

    default:
        break;
    }

    IoSkipCurrentIrpStackLocation(Irp);
    return IoCallDriver(deviceContext->LowerDeviceObject, Irp);
}


static NTSTATUS ForcePortCycle(_In_ PDEVICE_OBJECT LowerDeviceObject);
#pragma alloc_text(PAGE, ForcePortCycle)
_Use_decl_annotations_
static NTSTATUS ForcePortCycle(PDEVICE_OBJECT LowerDeviceObject) {
    PAGED_CODE();

    TraceEvents(TRACE_LEVEL_VERBOSE, TRACE_CHILD_DEVICE, "%!FUNC! Entry");

    KEVENT event;
    KeInitializeEvent(&event, NotificationEvent, FALSE);

    IO_STATUS_BLOCK ioStatus;
    PIRP irp = IoBuildDeviceIoControlRequest(IOCTL_INTERNAL_USB_CYCLE_PORT, LowerDeviceObject, NULL, 0, NULL, 0, TRUE, &event, &ioStatus);
    if (irp == NULL) {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_CHILD_DEVICE, "%!FUNC! IoBuildDeviceIoControlRequest failed");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    NTSTATUS status = IoCallDriver(LowerDeviceObject, irp);
    if (status == STATUS_PENDING) {
        LARGE_INTEGER timeout = { .QuadPart  = -(LONGLONG)5 * 1000 * 1000 * 10 };  // 5s relative timeout
        status = KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, &timeout);
        if (status == STATUS_TIMEOUT) {
            TraceEvents(TRACE_LEVEL_WARNING, TRACE_CHILD_DEVICE, "%!FUNC! KeWaitForSingleObject timed out");
            return STATUS_IO_TIMEOUT;
        }
        if (!NT_SUCCESS(status)) {
            TraceEvents(TRACE_LEVEL_ERROR, TRACE_CHILD_DEVICE, "%!FUNC! KeWaitForSingleObject failed: 0x%08x", status);
            return status;
        }
        status = ioStatus.Status;
    }
    if (!NT_SUCCESS(status)) {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_CHILD_DEVICE, "%!FUNC! IoCallDriver failed: 0x%08x", status);
        return status;
    }

    return STATUS_SUCCESS;
}


static DRIVER_DISPATCH ChildDispatchPnP;
#pragma alloc_text(PAGE, ChildDispatchPnP)
_Use_decl_annotations_
static NTSTATUS ChildDispatchPnP(PDEVICE_OBJECT DeviceObject, PIRP Irp) {
    PAGED_CODE();

    TraceEvents(TRACE_LEVEL_VERBOSE, TRACE_CHILD_DEVICE, "%!FUNC! Entry");

    PCHILD_DEVICE_CONTEXT deviceContext = (PCHILD_DEVICE_CONTEXT)DeviceObject->DeviceExtension;
    PIO_STACK_LOCATION stackLocation = IoGetCurrentIrpStackLocation(Irp);

    if (stackLocation->MinorFunction == IRP_MN_REMOVE_DEVICE) {
        TraceEvents(TRACE_LEVEL_VERBOSE, TRACE_CHILD_DEVICE, "%!FUNC! IRP_MN_REMOVE_DEVICE");

        // If the function driver was uninstalled, then PnP will remove us from the driver stack as well, but the PDO still exists.
        // Therefore, we force a port cycle, such that the USB bus driver will create a fresh PDO and attach another instance of our lower filter.
        // This is best effort, if it fails, we must still continue with the removal of our child device.
        (void)ForcePortCycle(deviceContext->LowerDeviceObject);

        WDFDRIVER driver = WdfGetDriver();
        if (driver != NULL) {
            PDRIVER_CONTEXT driverContext = DriverGetContext(driver);
            (void)WdfWaitLockAcquire(driverContext->ChildDeviceLock, NULL);
            ULONG count = WdfCollectionGetCount(driverContext->ChildDeviceCollection);
            for (ULONG i = 0; i < count; i++) {
                WDFOBJECT childObject = WdfCollectionGetItem(driverContext->ChildDeviceCollection, i);
                PCHILD_OBJECT_CONTEXT childObjectContext = WdfObjectGetContext(childObject);
                if (childObjectContext->ChildDevice == DeviceObject) {
                    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_CHILD_DEVICE, "%!FUNC! Removing child device %p from collection", DeviceObject);
                    WdfCollectionRemove(driverContext->ChildDeviceCollection, childObject);
                    break;
                }
            }
            WdfWaitLockRelease(driverContext->ChildDeviceLock);
        }

        IoSkipCurrentIrpStackLocation(Irp);
        NTSTATUS status = IoCallDriver(deviceContext->LowerDeviceObject, Irp);
        IoDetachDevice(deviceContext->LowerDeviceObject);
        IoDeleteDevice(DeviceObject);
        return status;
    }

    if (TRUE) {
        return ChildDispatchStubPnp(DeviceObject, Irp);
    }

    IoSkipCurrentIrpStackLocation(Irp);
    return IoCallDriver(deviceContext->LowerDeviceObject, Irp);
}


_Use_decl_annotations_
NTSTATUS ChildDispatch(PDEVICE_OBJECT DeviceObject, PIRP Irp) {
    TraceEvents(TRACE_LEVEL_VERBOSE, TRACE_CHILD_DEVICE, "%!FUNC! Entry");

    PCHILD_DEVICE_CONTEXT deviceContext = (PCHILD_DEVICE_CONTEXT)DeviceObject->DeviceExtension;
    PIO_STACK_LOCATION stackLocation = IoGetCurrentIrpStackLocation(Irp);

    if (stackLocation->MajorFunction == IRP_MJ_PNP) {
        TraceEvents(TRACE_LEVEL_VERBOSE, TRACE_CHILD_DEVICE, "%!FUNC! IRP_MJ_PNP MinorFunction: 0x%02x", stackLocation->MinorFunction);
        return ChildDispatchPnP(DeviceObject, Irp);
    }

    IoSkipCurrentIrpStackLocation(Irp);
    return IoCallDriver(deviceContext->LowerDeviceObject, Irp);
}
