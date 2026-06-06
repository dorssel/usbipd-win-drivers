// SPDX-FileCopyrightText: 2026 Frans van Dorsselaer
//
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include "stdafx.h"


typedef struct _DEVICE_CONTEXT
{
    WDFSTRING DeviceInterfaceSymbolicLinkName;
} DEVICE_CONTEXT, *PDEVICE_CONTEXT;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(DEVICE_CONTEXT, DeviceGetContext)

//
// Function to initialize the filter device and its callbacks
//
NTSTATUS FilterCreateDevice(_Inout_ PWDFDEVICE_INIT DeviceInit);
