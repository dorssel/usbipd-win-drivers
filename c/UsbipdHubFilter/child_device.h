#pragma once
// SPDX-FileCopyrightText: 2026 Frans van Dorsselaer
//
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include "stdafx.h"


// UsbChild in reverse.
#define CHILD_DEVICE_MAGIC ((((ULONG64)'dlih') << 32) | 'CbsU')


typedef struct _CHILD_DEVICE_CONTEXT {
    ULONG64 Magic;
    PDEVICE_OBJECT LowerDeviceObject;
} CHILD_DEVICE_CONTEXT, * PCHILD_DEVICE_CONTEXT;

NTSTATUS ChildCreateDevice(_In_ WDFDRIVER Driver, _Inout_ PWDFDEVICE_INIT DeviceInit);

DRIVER_DISPATCH ChildDispatch;
