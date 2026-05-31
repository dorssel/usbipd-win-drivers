#pragma once

#include "stdafx.h"

//
// WDFDRIVER Events
//

DRIVER_INITIALIZE DriverEntry;
EVT_WDF_DRIVER_DEVICE_ADD UsbipdStubEvtDeviceAdd;
EVT_WDF_OBJECT_CONTEXT_CLEANUP UsbipdStubEvtDriverContextCleanup;
