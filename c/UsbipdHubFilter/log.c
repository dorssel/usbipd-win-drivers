// SPDX-FileCopyrightText: 2026 Frans van Dorsselaer
//
// SPDX-License-Identifier: GPL-3.0-only

#include "stdafx.h"

#include "log.h"


TRACELOGGING_DEFINE_PROVIDER(
    g_LoggingProvider,
    "UsbipdHubFilter",
    // {47EA55DC-8F39-42A0-AE78-051B84C31175}
    (0x47ea55dc, 0x8f39, 0x42a0, 0xae, 0x78, 0x05, 0x1b, 0x84, 0xc3, 0x11, 0x75)
);
