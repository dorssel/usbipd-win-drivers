// SPDX-FileCopyrightText: 2026 Frans van Dorsselaer
//
// SPDX-License-Identifier: GPL-3.0-only

#include "stdafx.h"

#include "driver.h"
#include "trace.h"
#include "driver.tmh"

#include "device.h"


static EVT_WDF_OBJECT_CONTEXT_CLEANUP DriverContextCleanup;
#pragma alloc_text(PAGE, DriverContextCleanup)
_Use_decl_annotations_
static void DriverContextCleanup(WDFOBJECT DriverObject) {
    UNREFERENCED_PARAMETER(DriverObject);
    PAGED_CODE();

    TraceEvents(TRACE_LEVEL_VERBOSE, TRACE_DRIVER, "%!FUNC! Entry");

    WPP_CLEANUP(WdfDriverWdmGetDriverObject((WDFDRIVER)DriverObject));
}


#pragma alloc_text(INIT, DriverEntry)
_Use_decl_annotations_
NTSTATUS DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath) {
    PAGED_CODE();

    //
    // Required for POOL_ZERO_DOWN_LEVEL_SUPPORT (< Windows 10 2004). We do not need POOL_NX_OPTIN (< Windows 8).
    // Must be called before anything else.
    //
    ExInitializeDriverRuntime(0);

    //
    // Initialize WPP Tracing
    //
    WPP_INIT_TRACING(DriverObject, RegistryPath);

    TraceEvents(TRACE_LEVEL_VERBOSE, TRACE_DRIVER, "%!FUNC! Entry");

    //
    // We need a cleanup callback for WPP_CLEANUP during driver unload.
    //
    WDF_OBJECT_ATTRIBUTES attributes;
    WDF_OBJECT_ATTRIBUTES_INIT(&attributes);
    attributes.EvtCleanupCallback = DriverContextCleanup;

    WDF_DRIVER_CONFIG config;
    WDF_DRIVER_CONFIG_INIT(&config, StubCreateDevice);

    NTSTATUS status = WdfDriverCreate(DriverObject, RegistryPath, &attributes, &config, WDF_NO_HANDLE);

    if (!NT_SUCCESS(status)) {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_DRIVER, "WdfDriverCreate failed %!STATUS!", status);
        WPP_CLEANUP(DriverObject);
        return status;
    }

    TraceEvents(TRACE_LEVEL_VERBOSE, TRACE_DRIVER, "%!FUNC! Exit");

    return STATUS_SUCCESS;
}
