#pragma once

#include "stdafx.h"

//
// This is the context that can be placed per queue
// and would contain per queue information.
//
typedef struct {

    ULONG PrivateDeviceData;  // just a placeholder

} QUEUE_CONTEXT, *PQUEUE_CONTEXT;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(QUEUE_CONTEXT, QueueGetContext)

NTSTATUS
UsbipdHubFilterQueueInitialize(
    _In_ WDFDEVICE Device
    );

//
// Events from the IoQueue object
//
EVT_WDF_IO_QUEUE_IO_DEVICE_CONTROL UsbipdHubFilterEvtIoDeviceControl;
EVT_WDF_IO_QUEUE_IO_STOP UsbipdHubFilterEvtIoStop;
