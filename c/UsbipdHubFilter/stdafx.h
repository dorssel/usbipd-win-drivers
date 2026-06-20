// SPDX-FileCopyrightText: 2026 Frans van Dorsselaer
//
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

// Support new ExAllocatePoolZero on < Windows 10 2004.
#define POOL_ZERO_DOWN_LEVEL_SUPPORT

#include <initguid.h>
#include <ntddk.h>
#include <ntstrsafe.h>
#include <wdf.h>


// Hub F(ilter), in reverse.
#define POOL_TAG 'FbuH'
