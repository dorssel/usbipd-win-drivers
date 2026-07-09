// SPDX-FileCopyrightText: 2026 Frans van Dorsselaer
//
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include "stdafx.h"


#define USB_HUB_FILTER_NAME L"UsbipdHubFilter"

#define IOCTL_USB_HUB_FILTER_TEST CTL_CODE(FILE_DEVICE_UNKNOWN, 0x800, METHOD_BUFFERED, FILE_ANY_ACCESS)
