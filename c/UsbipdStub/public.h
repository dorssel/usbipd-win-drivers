// SPDX-FileCopyrightText: 2026 Frans van Dorsselaer
//
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include "stdafx.h"


//
// Define an Interface Guid so that app can find the device and talk to it.
//
// {ab363f87-1066-4541-a1f6-c4a7f9ab2077}
DEFINE_GUID(GUID_DEVINTERFACE_USBIPD_STUB, 0xab363f87, 0x1066, 0x4541, 0xa1, 0xf6, 0xc4, 0xa7, 0xf9, 0xab, 0x20, 0x77);


#define IOCTL_USB_STUB_USER_TEST CTL_CODE(FILE_DEVICE_UNKNOWN, 0x800, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_USB_STUB_ADMIN_TEST CTL_CODE(FILE_DEVICE_UNKNOWN, 0x801, METHOD_BUFFERED, FILE_WRITE_DATA)
