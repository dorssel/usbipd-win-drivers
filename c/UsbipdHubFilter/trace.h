// SPDX-FileCopyrightText: 2026 Frans van Dorsselaer
//
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include "stdafx.h"


//
// Define the tracing flags.
//
// Tracing GUID - b4829bf1-3240-4fec-8da3-33acc7178415
//
#define WPP_CONTROL_GUIDS                                                       \
    WPP_DEFINE_CONTROL_GUID(                                                    \
        UsbipdHubFilterTraceGuid, (b4829bf1, 3240, 4fec, 8da3, 33acc7178415),   \
                                                                                \
        WPP_DEFINE_BIT(TRACE_DRIVER)                                            \
        WPP_DEFINE_BIT(TRACE_HUB_DEVICE)                                        \
        WPP_DEFINE_BIT(TRACE_CHILD_DEVICE)                                      \
        WPP_DEFINE_BIT(TRACE_CONTROL_DEVICE)                                    \
        WPP_DEFINE_BIT(TRACE_CONTROL_QUEUE)                                     \
        )

#if DBG
// The In-Flight Recorder (IFR) by default never logs TRACE_LEVEL_VERBOSE.
#define WPP_RECORDER_LEVEL_FLAGS_ARGS(level, flags) WPP_CONTROL(WPP_BIT_ ## flags).AutoLogContext, level, WPP_BIT_ ## flags
#define WPP_RECORDER_LEVEL_FLAGS_FILTER(level, flags) (WPP_CONTROL(WPP_BIT_ ## flags).Level >= level)
#endif

#define WPP_LEVEL_FLAGS_LOGGER(level, flags) WPP_LEVEL_LOGGER(flags)
#define WPP_LEVEL_FLAGS_ENABLED(level, flags) (WPP_LEVEL_ENABLED(flags) && WPP_CONTROL(WPP_BIT_ ## flags).Level >= level)

//
// This comment block is scanned by the trace preprocessor to define our Trace functions.
//
// begin_wpp config
// FUNC TraceEvents(LEVEL, FLAGS, MSG, ...);
// end_wpp
//
