// SPDX-FileCopyrightText: 2026 Frans van Dorsselaer
//
// SPDX-License-Identifier: GPL-3.0-only

using Windows.Win32;

namespace TestTool;

static class Interop
{
    /// <summary>UsbipdHubFilter: public.h</summary>
    public const string USB_HUB_FILTER_NAME = "UsbipdHubFilter";

    /// <summary>UsbipdHubFilter: public.h</summary>
    internal enum IOCTL_USB_HUB_FILTER : uint
    {
        TEST = (PInvoke.FILE_DEVICE_UNKNOWN << 16) | (PInvoke.FILE_ANY_ACCESS << 14) | (0x800 << 2) | PInvoke.METHOD_BUFFERED,
    }

    /// <summary>UsbipdStub: public.h</summary>
    public static readonly Guid GUID_DEVINTERFACE_USBIPD_STUB = new(0xab363f87, 0x1066, 0x4541, 0xa1, 0xf6, 0xc4, 0xa7, 0xf9, 0xab, 0x20, 0x77);

    /// <summary>UsbipdStub: public.h</summary>
    internal enum IOCTL_USB_STUB : uint
    {
        USER_TEST = (PInvoke.FILE_DEVICE_UNKNOWN << 16) | (PInvoke.FILE_ANY_ACCESS << 14) | (0x800 << 2) | PInvoke.METHOD_BUFFERED,
        ADMIN_TEST = (PInvoke.FILE_DEVICE_UNKNOWN << 16) | (PInvoke.FILE_WRITE_ACCESS << 14) | (0x801 << 2) | PInvoke.METHOD_BUFFERED,
    }
}
