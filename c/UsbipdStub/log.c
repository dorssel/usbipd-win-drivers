// SPDX-FileCopyrightText: 2026 Frans van Dorsselaer
//
// SPDX-License-Identifier: GPL-3.0-only

#include "stdafx.h"

#include "log.h"


TRACELOGGING_DEFINE_PROVIDER(
    g_LoggingProvider,
    "UsbipdStub",
    // {FEFAD98F-FC36-4BFC-9341-C4429C8CDD8C}
    (0xfefad98f, 0xfc36, 0x4bfc, 0x93, 0x41, 0xc4, 0x42, 0x9c, 0x8c, 0xdd, 0x8c)
);
