// SPDX-FileCopyrightText: 2026 Frans van Dorsselaer
//
// SPDX-License-Identifier: GPL-3.0-only

#include "stdafx.h"

#include "control_queue.h"
#include "trace.h"
#include "control_queue.tmh"

#include "driver.h"
#include "public.h"


/// <summary>
/// Handle IOCTL for our control device.
/// </summary>
static EVT_WDF_IO_QUEUE_IO_DEVICE_CONTROL ControlEvtIoDeviceControl;
_Use_decl_annotations_
static void ControlEvtIoDeviceControl(WDFQUEUE Queue, WDFREQUEST Request, size_t OutputBufferLength, size_t InputBufferLength, ULONG IoControlCode) {
    UNREFERENCED_PARAMETER(OutputBufferLength);
    UNREFERENCED_PARAMETER(InputBufferLength);

    TraceEvents(TRACE_LEVEL_VERBOSE,
        TRACE_CONTROL_QUEUE,
        "%!FUNC! Queue 0x%p, Request 0x%p OutputBufferLength %d InputBufferLength %d IoControlCode %d",
        Queue, Request, (int)OutputBufferLength, (int)InputBufferLength, IoControlCode);

    NTSTATUS status;
    switch (IoControlCode) {
    case IOCTL_USB_HUB_FILTER_TEST_ANONYMOUS:
    case IOCTL_USB_HUB_FILTER_TEST_USER:
        TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_CONTROL_QUEUE, "Got IOCTL_USB_HUB_FILTER_TEST_* from user mode");
        status = STATUS_SUCCESS;
        break;

    case IOCTL_USB_HUB_FILTER_TEST_ADMIN:
    {
        WDFDEVICE device = WdfIoQueueGetDevice(Queue);
        PDRIVER_CONTEXT driverContext = DriverGetContext(WdfDeviceGetDriver(device));

        TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_CONTROL_QUEUE, "Got IOCTL_USB_HUB_FILTER_TEST_ADMIN from user mode, current value: %d",
            (int)driverContext->GlobalTestCounter);
        driverContext->GlobalTestCounter++;
        status = STATUS_SUCCESS;
        break;
    }

    default:
        TraceEvents(TRACE_LEVEL_WARNING, TRACE_CONTROL_QUEUE, "Invalid IOCTL request 0x%08lx", IoControlCode);
        status = STATUS_INVALID_DEVICE_REQUEST;
        break;
    }

    WdfRequestComplete(Request, status);
}


#pragma alloc_text(PAGE, ControlQueueInitialize)
_Use_decl_annotations_
NTSTATUS ControlQueueInitialize(WDFDEVICE Device) {
    PAGED_CODE();

    WDF_IO_QUEUE_CONFIG queueConfig;
    WDF_IO_QUEUE_CONFIG_INIT_DEFAULT_QUEUE(&queueConfig, WdfIoQueueDispatchSequential);
    queueConfig.EvtIoDeviceControl = ControlEvtIoDeviceControl;

    NTSTATUS status = WdfIoQueueCreate(Device, &queueConfig, WDF_NO_OBJECT_ATTRIBUTES, WDF_NO_HANDLE);
    if (!NT_SUCCESS(status)) {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_CONTROL_QUEUE, "WdfIoQueueCreate failed %!STATUS!", status);
        return status;
    }

    return STATUS_SUCCESS;
}
