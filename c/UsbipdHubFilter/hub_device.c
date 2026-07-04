// SPDX-FileCopyrightText: 2026 Frans van Dorsselaer
//
// SPDX-License-Identifier: GPL-3.0-only

#include "stdafx.h"

#include "hub_device.h"
#include "trace.h"
#include "hub_device.tmh"

#include "control_device.h"
#include "driver.h"


#if false
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

static NTSTATUS QueryIdCompletion(_In_ PDEVICE_OBJECT DeviceObject, _In_ PIRP Irp, _In_ PVOID Context) {
    UNREFERENCED_PARAMETER(DeviceObject);

    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DEVICE, "%!FUNC! Entry");

    if (NT_SUCCESS(Irp->IoStatus.Status) && (Irp->IoStatus.Information != 0)) {
        TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DEVICE, "%!FUNC! instanceId: %ws", (PWCHAR)Irp->IoStatus.Information);
    }
    IoFreeIrp(Irp);

    PIRP OriginalIrp = (PIRP)Context;
    if (OriginalIrp->PendingReturned) {
        TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DEVICE, "%!FUNC! pending");
        IoMarkIrpPending(OriginalIrp);
    }
    IoCompleteRequest(OriginalIrp, IO_NO_INCREMENT);

    return STATUS_CONTINUE_COMPLETION;
}
#endif


static NTSTATUS SendSynchronousQueryId(WDFDEVICE Device, PDEVICE_OBJECT pdo);
#pragma alloc_text(PAGE, SendSynchronousQueryId)
static NTSTATUS SendSynchronousQueryId(WDFDEVICE Device, PDEVICE_OBJECT pdo) {
    PAGED_CODE();

    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DEVICE, "%!FUNC! Entry");

    WDFIOTARGET ioTarget;
    NTSTATUS status = WdfIoTargetCreate(Device, WDF_NO_OBJECT_ATTRIBUTES, &ioTarget);
    if (!NT_SUCCESS(status)) {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_DEVICE, "%!FUNC! WdfIoTargetCreate %!STATUS!", status);
        return status;
    }

    WDF_IO_TARGET_OPEN_PARAMS openParameters;
    WDF_IO_TARGET_OPEN_PARAMS_INIT_EXISTING_DEVICE(&openParameters, pdo);
    status = WdfIoTargetOpen(ioTarget, &openParameters);
    if (!NT_SUCCESS(status)) {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_DEVICE, "%!FUNC! WdfIoTargetOpen %!STATUS!", status);
        WdfObjectDelete(ioTarget);
        return status;
    }

    WDFREQUEST request;
    status = WdfRequestCreate(WDF_NO_OBJECT_ATTRIBUTES, ioTarget, &request);
    if (!NT_SUCCESS(status)) {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_DEVICE, "%!FUNC! WdfRequestCreate %!STATUS!", status);
        WdfObjectDelete(ioTarget);
        return status;
    }

    WDF_REQUEST_REUSE_PARAMS reuseParameters;
    WDF_REQUEST_REUSE_PARAMS_INIT(&reuseParameters, WDF_REQUEST_REUSE_NO_FLAGS, STATUS_NOT_SUPPORTED);
    status = WdfRequestReuse(request, &reuseParameters);
    if (!NT_SUCCESS(status)) {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_DEVICE, "%!FUNC! WdfRequestReuse %!STATUS!", status);
        WdfObjectDelete(request);
        WdfObjectDelete(ioTarget);
        return status;
    }

    IO_STACK_LOCATION stackLocation;
    RtlZeroMemory(&stackLocation, sizeof(IO_STACK_LOCATION));
    stackLocation.MajorFunction = IRP_MJ_PNP;
    stackLocation.MinorFunction = IRP_MN_QUERY_ID;
    stackLocation.Parameters.QueryId.IdType = BusQueryDeviceID;
    WdfRequestWdmFormatUsingStackLocation(request, &stackLocation);

    WDF_REQUEST_SEND_OPTIONS sendOptions;
    WDF_REQUEST_SEND_OPTIONS_INIT(&sendOptions, WDF_REQUEST_SEND_OPTION_SYNCHRONOUS);
    (void)WdfRequestSend(request, ioTarget, &sendOptions);
    status = WdfRequestGetStatus(request);
    if (!NT_SUCCESS(status)) {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_DEVICE, "%!FUNC! WdfRequestGetStatus %!STATUS!", status);
        WdfObjectDelete(request);
        WdfObjectDelete(ioTarget);
        return status;
    }

    PWCHAR deviceId = (PWCHAR)WdfRequestGetInformation(request);
    if (deviceId == NULL) {
        TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DEVICE, "%!FUNC! DeviceId is NULL");
        WdfObjectDelete(request);
        WdfObjectDelete(ioTarget);
        return STATUS_INVALID_DEVICE_REQUEST;
    }

    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DEVICE, "%!FUNC! DeviceId: %ws", deviceId);

    ExFreePool(deviceId);
    WdfObjectDelete(request);
    WdfObjectDelete(ioTarget);

    return STATUS_SUCCESS;
}


typedef struct _WORKITEM_CONTEXT {
    PIRP Irp;
} WORKITEM_CONTEXT, * PWORKITEM_CONTEXT;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(WORKITEM_CONTEXT, WdfWorkItemGetContext)


static EVT_WDF_WORKITEM EvtWorkItemCallback;
#pragma alloc_text(PAGE, EvtWorkItemCallback)
_Use_decl_annotations_
static VOID EvtWorkItemCallback(WDFWORKITEM WorkItem) {
    PAGED_CODE();
    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DEVICE, "%!FUNC! Entry");

    // We already know this is a successful completion, so we can safely inspect the results.

    PWORKITEM_CONTEXT context = WdfWorkItemGetContext(WorkItem);
    PDEVICE_RELATIONS relations = (PDEVICE_RELATIONS)context->Irp->IoStatus.Information;

    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DEVICE, "%!FUNC! relations count = %lu", relations->Count);

    for (ULONG i = 0; i < relations->Count; i++) {
        PDEVICE_OBJECT pdo = relations->Objects[i];
        if (pdo != NULL) {
            TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DEVICE, "%!FUNC! found child PDO at index %u: %p", i, pdo);
            (void)SendSynchronousQueryId((WDFDEVICE)WdfWorkItemGetParentObject(WorkItem), pdo);
        }
    }

    IoCompleteRequest(context->Irp, IO_NO_INCREMENT);

    WdfObjectDelete(WorkItem);
}


static IO_COMPLETION_ROUTINE HubBusRelationsCompletion;
_Use_decl_annotations_
static NTSTATUS HubBusRelationsCompletion(PDEVICE_OBJECT DeviceObject, PIRP Irp, PVOID Context) {
    UNREFERENCED_PARAMETER(DeviceObject);

    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DEVICE, "%!FUNC! Entry");

    WDFDEVICE device = (WDFDEVICE)Context;

    if (!NT_SUCCESS(Irp->IoStatus.Status) && (Irp->IoStatus.Information == 0)) {
        TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DEVICE, "%!FUNC! lower driver failed");
        // Not interested in failed results.
        if (Irp->PendingReturned) {
            TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DEVICE, "%!FUNC! pending");
            IoMarkIrpPending(Irp);
        }
        return STATUS_CONTINUE_COMPLETION;
    }

    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DEVICE, "%!FUNC! scheduling worker");

#if false
//    if (relations->Count == 0) {
        if (Irp->PendingReturned) {
            TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DEVICE, "%!FUNC! pending");
            IoMarkIrpPending(Irp);
        }
        return STATUS_CONTINUE_COMPLETION;
//    }
#endif

    WDF_WORKITEM_CONFIG workItemConfig;
    WDF_WORKITEM_CONFIG_INIT(&workItemConfig, EvtWorkItemCallback);

    WDF_OBJECT_ATTRIBUTES attributes;
    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&attributes, WORKITEM_CONTEXT);
    attributes.ParentObject = device;

    WDFWORKITEM workItem;
    NTSTATUS status = WdfWorkItemCreate(&workItemConfig, &attributes, &workItem);
    if (!NT_SUCCESS(status)) {
        TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DEVICE, "%!FUNC! WdfWorkItemCreate %!STATUS!", status);
        if (Irp->PendingReturned) {
            TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DEVICE, "%!FUNC! pending");
            IoMarkIrpPending(Irp);
        }
        return STATUS_CONTINUE_COMPLETION;
    }

    PWORKITEM_CONTEXT workItemContext = WdfWorkItemGetContext(workItem);
    workItemContext->Irp = Irp;

    WdfWorkItemEnqueue(workItem);

    return STATUS_MORE_PROCESSING_REQUIRED;

#if false
    // TEST: Relay an additional IRP to the first child PDO before completing this IRP.
    PIRP newIrp = IoAllocateIrp(relations->Objects[0]->StackSize, FALSE);
    if (newIrp == NULL) {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_DEVICE, "IoAllocateIrp failed");
        Irp->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
        if (Irp->PendingReturned) {
            TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DEVICE, "%!FUNC! pending");
            IoMarkIrpPending(Irp);
        }
        return STATUS_CONTINUE_COMPLETION;
    }

    PIO_STACK_LOCATION NextStack = IoGetNextIrpStackLocation(newIrp);
    NextStack->MajorFunction = IRP_MJ_PNP;
    NextStack->MinorFunction = IRP_MN_QUERY_ID;
    NextStack->Parameters.QueryId.IdType = BusQueryInstanceID;
    IoSetCompletionRoutine(newIrp, QueryIdCompletion, Irp, TRUE, TRUE, TRUE);

    IoCallDriver(relations->Objects[0], newIrp);

    return STATUS_MORE_PROCESSING_REQUIRED;
#endif
}


static NTSTATUS HubQueryDeviceRelations(_In_ WDFDEVICE Device, _Inout_ PIRP Irp) {
    PIO_STACK_LOCATION irpStack = IoGetCurrentIrpStackLocation(Irp);

    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DEVICE, "%!FUNC!");

    if (irpStack->Parameters.QueryDeviceRelations.Type != BusRelations) {
        TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DEVICE, "%!FUNC! OtherRelations");
        // We are only interested in BusRelations. Just pass anything else down without interception.
        IoSkipCurrentIrpStackLocation(Irp);
        return WdfDeviceWdmDispatchPreprocessedIrp(Device, Irp);
    }

    // We need to bypass WDF, because we need a worker in our completion routine.

    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DEVICE, "%!FUNC! BusRelations");

    IoMarkIrpPending(Irp);
    IoCopyCurrentIrpStackLocationToNext(Irp);
    IoSetCompletionRoutine(Irp, HubBusRelationsCompletion, Device, TRUE, TRUE, TRUE);
    IoCallDriver(WdfDeviceWdmGetAttachedDevice(Device), Irp);
    return STATUS_PENDING;
}


static EVT_WDF_DEVICE_CONTEXT_CLEANUP HubDeviceContextCleanup;
#pragma alloc_text(PAGE, HubDeviceContextCleanup)
_Use_decl_annotations_
static void HubDeviceContextCleanup(WDFOBJECT Device) {
    PAGED_CODE();

    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DEVICE, "%!FUNC! Entry");

    WDFDRIVER driver = WdfDeviceGetDriver((WDFDEVICE)Device);
    PDRIVER_CONTEXT driverContext = DriverGetContext(driver);
    (void)WdfWaitLockAcquire(driverContext->HubDeviceLock, NULL);
    driverContext->HubDeviceCount--;
    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DEVICE, "HubDeviceCount is now %d.", driverContext->HubDeviceCount);
    if ((driverContext->HubDeviceCount == 0) && (driverContext->ControlDevice != NULL)) {
        TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DEVICE, "Deleting control device.");
        WdfObjectDelete(driverContext->ControlDevice);
        driverContext->ControlDevice = NULL;
    }
    WdfWaitLockRelease(driverContext->HubDeviceLock);

    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DEVICE, "%!FUNC! Exit");
}


/// <summary>
/// This is the EvtDeviceAdd callback function that creates and initializes a new instance of the device each time one is enumerated by the PnP manager.
/// </summary>
#pragma alloc_text(PAGE, HubCreateDevice)
_Use_decl_annotations_
NTSTATUS HubCreateDevice(WDFDRIVER Driver, PWDFDEVICE_INIT DeviceInit) {
    PAGED_CODE();

    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DEVICE, "%!FUNC! Entry");

    //
    // Tell the framework that we are a filter driver.
    //
    WdfFdoInitSetFilter(DeviceInit);

    WDF_OBJECT_ATTRIBUTES deviceAttributes;
    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&deviceAttributes, DEVICE_CONTEXT);
    deviceAttributes.EvtCleanupCallback = HubDeviceContextCleanup;

    //
    // Tell KMDF to route all IRP_MN_QUERY_DEVICE_RELATIONS requests to our raw WDM handler.
    //
    UCHAR minorCode = IRP_MN_QUERY_DEVICE_RELATIONS;
    NTSTATUS status = WdfDeviceInitAssignWdmIrpPreprocessCallback(DeviceInit, HubQueryDeviceRelations, IRP_MJ_PNP, &minorCode, 1);
    if (!NT_SUCCESS(status)) {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_DEVICE, "WdfDeviceInitAssignWdmIrpPreprocessCallback failed %!STATUS!", status);
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
    // We can probably do without a context, but we will keep it for now in case we need to store some state later.
    //
    PDEVICE_CONTEXT deviceContext = DeviceGetContext(device);
    deviceContext->DummyPlaceholder = 0;

    //
    // We need to create the control device only when the first device is created.
    //
    PDRIVER_CONTEXT driverContext = DriverGetContext(Driver);
    (void)WdfWaitLockAcquire(driverContext->HubDeviceLock, NULL);
    driverContext->HubDeviceCount++;
    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DEVICE, "HubDeviceCount is now %d.", driverContext->HubDeviceCount);
    if (driverContext->ControlDevice == NULL) {
        status = ControlCreateDevice(Driver);
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_DEVICE, "ControlCreateDevice status %!STATUS!", status);
        // Too bad if this fails, but we will continue. The control device will not be available, but the filter device will still load correctly.
    }
    WdfWaitLockRelease(driverContext->HubDeviceLock);

    //
    // NOTE: We don't need a queue, we only need to intercept IRP_MN_QUERY_DEVICE_RELATIONS, which we already did above.
    //

    TraceEvents(TRACE_LEVEL_ERROR, TRACE_DEVICE, "%!FUNC! Success");

    return STATUS_SUCCESS;
}
