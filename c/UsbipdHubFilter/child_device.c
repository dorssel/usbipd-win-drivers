// SPDX-FileCopyrightText: 2026 Frans van Dorsselaer
//
// SPDX-License-Identifier: GPL-3.0-only

#include "stdafx.h"

#include "child_device.h"
#include "trace.h"
#include "child_device.tmh"


_Use_decl_annotations_
NTSTATUS ChildDispatch(PDEVICE_OBJECT DeviceObject, PIRP Irp) {
    PCHILD_DEVICE_CONTEXT deviceContext = (PCHILD_DEVICE_CONTEXT)DeviceObject->DeviceExtension;

    IoSkipCurrentIrpStackLocation(Irp);

    return IoCallDriver(deviceContext->LowerDeviceObject, Irp);
}
