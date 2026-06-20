// SPDX-FileCopyrightText: 2026 Frans van Dorsselaer
//
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include "stdafx.h"


//
// This is a singleton object shared by all devices.
//
typedef struct _DRIVER_CONTEXT {
    WDFWAITLOCK ControlDeviceLock;
    LONG ControlDeviceCount;
    WDFDEVICE ControlDevice;
    int GlobalTestCounter;
} DRIVER_CONTEXT, * PDRIVER_CONTEXT;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(DRIVER_CONTEXT, DriverGetContext)

DRIVER_INITIALIZE DriverEntry;
