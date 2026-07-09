// SPDX-FileCopyrightText: 2026 Frans van Dorsselaer
//
// SPDX-License-Identifier: GPL-3.0-only

using System.ComponentModel;
using System.Runtime.InteropServices;
using Windows.Win32;
using Windows.Win32.Devices.DeviceAndDriverInstallation;
using Windows.Win32.Foundation;
using Windows.Win32.Storage.FileSystem;

namespace TestTool;

sealed class Program
{
    public static string[] GetAllDeviceInterfaces(Guid interfaceClassGuid)
    {
        if (PInvoke.CM_Get_Device_Interface_List_Size(out var bufferSize, interfaceClassGuid, null,
            CM_GET_DEVICE_INTERFACE_LIST_FLAGS.CM_GET_DEVICE_INTERFACE_LIST_PRESENT) != CONFIGRET.CR_SUCCESS)
        {
            return [];
        }
        if (bufferSize <= 1)
        {
            // Empty list.
            return [];
        }
        var buffer = new char[checked((int)bufferSize)];
        unsafe // DevSkim: ignore DS172412
        {
            fixed (char* pBuffer = buffer)
            {
                if (PInvoke.CM_Get_Device_Interface_List(interfaceClassGuid, null, pBuffer, bufferSize,
                    CM_GET_DEVICE_INTERFACE_LIST_FLAGS.CM_GET_DEVICE_INTERFACE_LIST_PRESENT) != CONFIGRET.CR_SUCCESS)
                {
                    return [];
                }
            }
        }
        // The list is double-NUL terminated.
        return new string(buffer.AsSpan()[..^2]).Split('\0');
    }


    static void Main()
    {
        using var hubFilter = PInvoke.CreateFile(@"\\.\" + Interop.USB_HUB_FILTER_NAME, 0,
            FILE_SHARE_MODE.FILE_SHARE_READ | FILE_SHARE_MODE.FILE_SHARE_WRITE,
            null, FILE_CREATION_DISPOSITION.OPEN_EXISTING, 0);

        if (hubFilter.IsInvalid)
        {
            throw new Win32Exception(Marshal.GetLastWin32Error(), "CreateFile");
        }

        unsafe // DevSkim: ignore DS172412
        {
            if (!PInvoke.DeviceIoControl(hubFilter, (uint)Interop.IOCTL_USB_HUB_FILTER.TEST))
            {
                throw new Win32Exception(Marshal.GetLastWin32Error(), "DeviceIoControl");
            }
        }

        foreach (var deviceInterface in GetAllDeviceInterfaces(Interop.GUID_DEVINTERFACE_USBIPD_STUB))
        {
            Console.WriteLine($"Testing {deviceInterface}");
            {
                Console.WriteLine($"  As User");
                using var stub = PInvoke.CreateFile(deviceInterface, 0,
                    FILE_SHARE_MODE.FILE_SHARE_READ | FILE_SHARE_MODE.FILE_SHARE_WRITE,
                    null, FILE_CREATION_DISPOSITION.OPEN_EXISTING, 0);

                unsafe // DevSkim: ignore DS172412
                {
                    Console.WriteLine($"    User call: {(bool)PInvoke.DeviceIoControl(stub, (uint)Interop.IOCTL_USB_STUB.USER_TEST)}");
                    Console.WriteLine($"    Admin call: {(bool)PInvoke.DeviceIoControl(stub, (uint)Interop.IOCTL_USB_STUB.ADMIN_TEST)}");
                }
            }
            {
                Console.WriteLine($"  As Admin");
                using var stub = PInvoke.CreateFile(deviceInterface, (uint)GENERIC_ACCESS_RIGHTS.GENERIC_WRITE,
                    FILE_SHARE_MODE.FILE_SHARE_READ | FILE_SHARE_MODE.FILE_SHARE_WRITE,
                    null, FILE_CREATION_DISPOSITION.OPEN_EXISTING, 0);

                unsafe // DevSkim: ignore DS172412
                {
                    Console.WriteLine($"    User call: {(bool)PInvoke.DeviceIoControl(stub, (uint)Interop.IOCTL_USB_STUB.USER_TEST)}");
                    Console.WriteLine($"    Admin call: {(bool)PInvoke.DeviceIoControl(stub, (uint)Interop.IOCTL_USB_STUB.ADMIN_TEST)}");
                }
            }
        }

        Console.WriteLine("Done");
    }
}
