// SPDX-FileCopyrightText: 2026 Frans van Dorsselaer
//
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

// Support new ExAllocatePoolZero on < Windows 10 2004.
#define POOL_ZERO_DOWN_LEVEL_SUPPORT

#include <guiddef.h>
#include <ntddk.h>
#include <usb.h>
#include <usbdlib.h>
#include <wdf.h>
#include <wdfusb.h>


// Stub, in reverse.
#define POOL_TAG 'butS'
