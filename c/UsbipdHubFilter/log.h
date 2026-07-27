// SPDX-FileCopyrightText: 2026 Frans van Dorsselaer
//
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include "stdafx.h"

#include "LoggingProviderManifest.h"


TRACELOGGING_DECLARE_PROVIDER(g_LoggingProvider);


#define LogEvent(level, name, ...) \
    do { \
        TraceLoggingWrite(g_LoggingProvider, name, TraceLoggingLevel(level), TraceLoggingChannel(LOG_CHANNEL), \
            TraceLoggingKeyword(LOG_CHANNEL_KEYWORD) __VA_OPT__(,) __VA_ARGS__); \
    } while (0)
