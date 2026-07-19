// SPDX-FileCopyrightText: 2026 Frans van Dorsselaer
//
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include "stdafx.h"


#define USB_HUB_FILTER_NAME L"UsbipdHubFilter"

#define IOCTL_USB_HUB_FILTER_TEST_ANONYMOUS CTL_CODE(FILE_DEVICE_UNKNOWN, 0x800, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_USB_HUB_FILTER_TEST_USER CTL_CODE(FILE_DEVICE_UNKNOWN, 0x801, METHOD_BUFFERED, FILE_READ_ACCESS)
#define IOCTL_USB_HUB_FILTER_TEST_ADMIN CTL_CODE(FILE_DEVICE_UNKNOWN, 0x802, METHOD_BUFFERED, FILE_WRITE_ACCESS)


typedef struct _VID_PID {
    USHORT Vid;
    USHORT Pid;
} VID_PID, * PVID_PID;


/// <summary>
/// Valid values for the HubId and PortId fields are 1-99.
/// If either field is out of range, set both fields to 0 to indicate an invalid or uninitialized state,
/// i.e., a BusId of 0-0 is invalid, and a valid BusId is always in the range 1-99 for both HubId and PortId.
/// </summary>
typedef struct _BUS_ID {
    USHORT HubId;
    USHORT PortId;
} BUS_ID, * PBUS_ID;
