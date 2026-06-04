#pragma once

#include "stdafx.h"

//
// WDFDRIVER Events
//

DRIVER_INITIALIZE DriverEntry;
EVT_WDF_DRIVER_DEVICE_ADD UsbipdHubFilterEvtDeviceAdd;
EVT_WDF_OBJECT_CONTEXT_CLEANUP UsbipdHubFilterEvtDriverContextCleanup;
