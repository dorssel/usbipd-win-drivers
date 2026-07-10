// SPDX-FileCopyrightText: 2026 Frans van Dorsselaer
//
// SPDX-License-Identifier: GPL-3.0-only

#include "stdafx.h"

#include "queue.h"
#include "trace.h"
#include "queue.tmh"

#include "public.h"


static EVT_WDF_IO_QUEUE_IO_DEVICE_CONTROL StubEvtIoDeviceControl;
#pragma alloc_text(PAGE, StubEvtIoDeviceControl)
_Use_decl_annotations_
static void StubEvtIoDeviceControl(WDFQUEUE Queue, WDFREQUEST Request, size_t OutputBufferLength, size_t InputBufferLength, ULONG IoControlCode) {
    UNREFERENCED_PARAMETER(Queue);
    PAGED_CODE();

    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_QUEUE, "%!FUNC! IoControlCode 0x%08x OutputBufferLength %!Iu! InputBufferLength %!Iu! ",
        IoControlCode, OutputBufferLength, InputBufferLength);

    switch (IoControlCode) {
    case IOCTL_USB_STUB_TEST_ANONYMOUS:
    case IOCTL_USB_STUB_TEST_USER:
    case IOCTL_USB_STUB_TEST_ADMIN:
        break;

    default:
        WdfRequestComplete(Request, STATUS_INVALID_DEVICE_REQUEST);
        return;
    }

    WdfRequestComplete(Request, STATUS_SUCCESS);
}


static EVT_WDF_IO_QUEUE_IO_STOP StubEvtIoStop;
#pragma alloc_text(PAGE, StubEvtIoStop)
_Use_decl_annotations_
static void StubEvtIoStop(WDFQUEUE Queue, WDFREQUEST Request, ULONG ActionFlags) {
    PAGED_CODE();

    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_QUEUE, "%!FUNC! Queue 0x%p, Request 0x%p ActionFlags %d", Queue, Request, ActionFlags);

    //
    // In most cases, the EvtIoStop callback function completes, cancels, or postpones
    // further processing of the I/O request.
    //
    // Typically, the driver uses the following rules:
    //
    // - If the driver owns the I/O request, it either postpones further processing
    //   of the request and calls WdfRequestStopAcknowledge, or it calls WdfRequestComplete
    //   with a completion status value of STATUS_SUCCESS or STATUS_CANCELLED.
    //
    //   The driver must call WdfRequestComplete only once, to either complete or cancel
    //   the request. To ensure that another thread does not call WdfRequestComplete
    //   for the same request, the EvtIoStop callback must synchronize with the driver's
    //   other event callback functions, for instance by using interlocked operations.
    //
    // - If the driver has forwarded the I/O request to an I/O target, it either calls
    //   WdfRequestCancelSentRequest to attempt to cancel the request, or it postpones
    //   further processing of the request and calls WdfRequestStopAcknowledge.
    //
    // A driver might choose to take no action in EvtIoStop for requests that are
    // guaranteed to complete in a small amount of time. For example, the driver might
    // take no action for requests that are completed in one of the driver’s request handlers.
    //
}


#pragma alloc_text(PAGE, StubQueueInitialize)
_Use_decl_annotations_
NTSTATUS StubQueueInitialize(WDFDEVICE Device) {
    PAGED_CODE();

    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_QUEUE, "%!FUNC! Entry");

    WDF_IO_QUEUE_CONFIG queueConfig;
    WDF_IO_QUEUE_CONFIG_INIT_DEFAULT_QUEUE(&queueConfig, WdfIoQueueDispatchSequential);
    queueConfig.EvtIoDeviceControl = StubEvtIoDeviceControl;
    queueConfig.EvtIoStop = StubEvtIoStop;

    WDF_OBJECT_ATTRIBUTES queueAttributes;
    WDF_OBJECT_ATTRIBUTES_INIT(&queueAttributes);
    queueAttributes.ExecutionLevel = WdfExecutionLevelPassive;

    NTSTATUS status = WdfIoQueueCreate(Device, &queueConfig, &queueAttributes, WDF_NO_HANDLE);
    if (!NT_SUCCESS(status)) {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_QUEUE, "WdfIoQueueCreate failed %!STATUS!", status);
        return status;
    }

    return STATUS_SUCCESS;
}
