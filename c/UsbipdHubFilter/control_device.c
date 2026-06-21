// SPDX-FileCopyrightText: 2026 Frans van Dorsselaer
//
// SPDX-License-Identifier: GPL-3.0-only

#include "stdafx.h"

#include "control_device.h"
#include "trace.h"
#include "control_device.tmh"

#include "control_queue.h"
#include "driver.h"


#pragma alloc_text(PAGE, ControlCreateDevice)
_Use_decl_annotations_
NTSTATUS ControlCreateDevice(WDFDRIVER driver) {
    static DECLARE_CONST_UNICODE_STRING(deviceName, L"\\Device\\UsbipdHubFilter");
    static DECLARE_CONST_UNICODE_STRING(deviceSymbolicLinkName, L"\\DosDevices\\UsbipdHubFilter");

    PAGED_CODE();

    PWDFDEVICE_INIT deviceInit = WdfControlDeviceInitAllocate(driver, &SDDL_DEVOBJ_SYS_ALL_ADM_ALL);
    if (deviceInit == NULL) {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_CONTROL_DEVICE, "WdfControlDeviceInitAllocate failed");
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    WdfDeviceInitSetExclusive(deviceInit, FALSE);
    NTSTATUS status = WdfDeviceInitAssignName(deviceInit, &deviceName);
    if (!NT_SUCCESS(status)) {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_CONTROL_DEVICE, "WdfDeviceInitAssignName failed %!STATUS!", status);
        WdfDeviceInitFree(deviceInit);
        return status;
    }

    WDFDEVICE device = NULL;
    status = WdfDeviceCreate(&deviceInit, WDF_NO_OBJECT_ATTRIBUTES, &device);
    if (!NT_SUCCESS(status)) {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_CONTROL_DEVICE, "WdfDeviceCreate failed %!STATUS!", status);
        WdfDeviceInitFree(deviceInit);
        return status;
    }

    status = WdfDeviceCreateSymbolicLink(device, &deviceSymbolicLinkName);
    if (!NT_SUCCESS(status)) {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_CONTROL_DEVICE, "WdfDeviceCreateSymbolicLink failed %!STATUS!", status);
        WdfObjectDelete(device);
        return status;
    }

    //
    // Initialize the Queue
    //
    status = ControlQueueInitialize(device);
    if (!NT_SUCCESS(status)) {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_CONTROL_DEVICE, "ControlQueueInitialize failed %!STATUS!", status);
        WdfObjectDelete(device);
        return status;
    }

    WdfControlFinishInitializing(device);

    // NOTE: The lock is already taken by the caller.
    PDRIVER_CONTEXT driverContext = DriverGetContext(driver);
    driverContext->ControlDevice = device;

    return STATUS_SUCCESS;
}
