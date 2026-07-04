// SPDX-FileCopyrightText: 2026 Frans van Dorsselaer
//
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include "stdafx.h"


//
// This is a singleton object shared by all devices.
//
typedef struct _DRIVER_CONTEXT {
    PDRIVER_DISPATCH WdfDispatch[IRP_MJ_MAXIMUM_FUNCTION + 1];

    WDFWAITLOCK HubDeviceLock;
    LONG HubDeviceCount;
    WDFDEVICE ControlDevice;

    int GlobalTestCounter;
} DRIVER_CONTEXT, * PDRIVER_CONTEXT;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(DRIVER_CONTEXT, DriverGetContext)

DRIVER_INITIALIZE DriverEntry;
