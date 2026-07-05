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

    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "%!FUNC! Entry");

    PCHILD_DEVICE_CONTEXT deviceContext = (PCHILD_DEVICE_CONTEXT)DeviceObject->DeviceExtension;
    PIO_STACK_LOCATION stackLocation = IoGetCurrentIrpStackLocation(Irp);

    static DECLARE_CONST_UNICODE_STRING(deviceIdStr, L"USB\\VID_1209&PID_8251");
    static DECLARE_CONST_UNICODE_STRING(deviceTextStr, L"USBIP Shared Device");

    switch (stackLocation->MinorFunction) {
    case IRP_MN_QUERY_ID:
        TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "%!FUNC! IRP_MN_QUERY_ID %d", stackLocation->Parameters.QueryId.IdType);

        switch (stackLocation->Parameters.QueryId.IdType) {
        case BusQueryDeviceID: {
            TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "%!FUNC! BusQueryDeviceID");

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
            TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "%!FUNC! BusQueryHardwareIDs");

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
            TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "%!FUNC! BusQueryCompatibleIDs");

            // return default STATUS_NOT_SUPPORTED (which is already set for every IRP_MJ_PNP)
            IoCompleteRequest(Irp, IO_NO_INCREMENT);
            return Irp->IoStatus.Status;
        }

        default:
            break;
        }
        break;

    case IRP_MN_QUERY_DEVICE_TEXT:
        TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "%!FUNC! IRP_MN_QUERY_DEVICE_TEXT %d", stackLocation->Parameters.QueryDeviceText.DeviceTextType);

        switch (stackLocation->Parameters.QueryDeviceText.DeviceTextType) {
        case DeviceTextDescription: {
            TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "%!FUNC! DeviceTextDescription");
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


static DRIVER_DISPATCH ChildDispatchPnP;
#pragma alloc_text(PAGE, ChildDispatchPnP)
_Use_decl_annotations_
static NTSTATUS ChildDispatchPnP(PDEVICE_OBJECT DeviceObject, PIRP Irp) {
    PAGED_CODE();

    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "%!FUNC! Entry");

    PCHILD_DEVICE_CONTEXT deviceContext = (PCHILD_DEVICE_CONTEXT)DeviceObject->DeviceExtension;
    PIO_STACK_LOCATION stackLocation = IoGetCurrentIrpStackLocation(Irp);

    if (stackLocation->MinorFunction == IRP_MN_REMOVE_DEVICE) {
        TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "%!FUNC! IRP_MN_REMOVE_DEVICE");

        WDFDRIVER driver = WdfGetDriver();
        if (driver != NULL) {
            PDRIVER_CONTEXT driverContext = DriverGetContext(driver);
            (void)WdfWaitLockAcquire(driverContext->ChildDeviceLock, NULL);
            ULONG count = WdfCollectionGetCount(driverContext->ChildDeviceCollection);
            for (ULONG i = 0; i < count; i++) {
                WDFOBJECT childObject = WdfCollectionGetItem(driverContext->ChildDeviceCollection, i);
                PCHILD_OBJECT_CONTEXT childObjectContext = WdfObjectGetContext(childObject);
                if (childObjectContext->ChildDevice == DeviceObject) {
                    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "%!FUNC! Removing child device %p from collection", DeviceObject);
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
    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "%!FUNC! Entry");

    PCHILD_DEVICE_CONTEXT deviceContext = (PCHILD_DEVICE_CONTEXT)DeviceObject->DeviceExtension;
    PIO_STACK_LOCATION stackLocation = IoGetCurrentIrpStackLocation(Irp);

    if (stackLocation->MajorFunction == IRP_MJ_PNP) {
        TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "%!FUNC! IRP_MJ_PNP MinorFunction: 0x%02x", stackLocation->MinorFunction);
        return ChildDispatchPnP(DeviceObject, Irp);
    }

    IoSkipCurrentIrpStackLocation(Irp);
    return IoCallDriver(deviceContext->LowerDeviceObject, Irp);
}
