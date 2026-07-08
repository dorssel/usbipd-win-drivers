// SPDX-FileCopyrightText: 2026 Frans van Dorsselaer
//
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include "stdafx.h"


typedef struct _DEVICE_CONTEXT {
    int DummyPlaceholder;
} DEVICE_CONTEXT, *PDEVICE_CONTEXT;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(DEVICE_CONTEXT, DeviceGetContext)

/// <summary>
/// This is the callback function that creates and initializes a new instance of the device each time one is enumerated by the PnP manager.
/// </summary>
EVT_WDF_DRIVER_DEVICE_ADD HubCreateDevice;
