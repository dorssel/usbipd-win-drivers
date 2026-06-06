// SPDX-FileCopyrightText: 2026 Frans van Dorsselaer
//
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include "stdafx.h"

//
// WDFDRIVER Events
//

DRIVER_INITIALIZE DriverEntry;
EVT_WDF_DRIVER_DEVICE_ADD UsbipdHubFilterEvtDeviceAdd;
EVT_WDF_OBJECT_CONTEXT_CLEANUP UsbipdHubFilterEvtDriverContextCleanup;
