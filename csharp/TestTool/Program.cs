// SPDX-FileCopyrightText: 2026 Frans van Dorsselaer
//
// SPDX-License-Identifier: GPL-3.0-only

using System.ComponentModel;
using System.Diagnostics;
using System.Diagnostics.CodeAnalysis;
using System.Runtime.InteropServices;
using Microsoft.Win32.SafeHandles;
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


    [StackTraceHidden]
    [DoesNotReturn]
    static void ThrowLastPInvokeError(string functionName)
    {
        throw new Win32Exception(Marshal.GetLastPInvokeError(), $"{functionName}: {Marshal.GetLastPInvokeErrorMessage()}");
    }


    static SafeFileHandle OpenDevice(string path, GENERIC_ACCESS_RIGHTS desiredAccess)
    {
        var handle = PInvoke.CreateFile(path, (uint)desiredAccess,
            FILE_SHARE_MODE.FILE_SHARE_READ | FILE_SHARE_MODE.FILE_SHARE_WRITE,
            null, FILE_CREATION_DISPOSITION.OPEN_EXISTING, 0);
        if (handle.IsInvalid)
        {
            Console.WriteLine($"{nameof(PInvoke.CreateFile)}: {Marshal.GetLastPInvokeErrorMessage()}");
        }
        return handle;
    }


    static bool IoControl<T>(SafeHandle device, T ioControlCode, ReadOnlySpan<byte> inBuffer = default,
        Span<byte> outBuffer = default, bool exactOutput = true) where T : Enum
    {
        unsafe // DevSkim: ignore DS172412
        {
            if (!PInvoke.DeviceIoControl(device, Convert.ToUInt32(ioControlCode), inBuffer, outBuffer, out var bytesReturned))
            {
                Console.WriteLine($"{nameof(PInvoke.DeviceIoControl)}: {Marshal.GetLastPInvokeErrorMessage()}");
                return false;
            }
            if (exactOutput && bytesReturned != outBuffer.Length)
            {
                Console.WriteLine($"Expected {outBuffer.Length} bytes, but got {bytesReturned} bytes.");
                return false;
            }
        }
        return true;
    }

    static void TestHubFilter(string entity, GENERIC_ACCESS_RIGHTS desiredAccess)
    {
        Console.WriteLine($"  As {entity}");
        using var hubFilter = OpenDevice(@"\\.\" + Interop.USB_HUB_FILTER_NAME, desiredAccess);
        if (!hubFilter.IsInvalid)
        {
            Console.WriteLine($"    Anonymous Call: {IoControl(hubFilter, Interop.IOCTL_USB_HUB_FILTER.TEST_ANONYMOUS)}");
            Console.WriteLine($"    User Call: {IoControl(hubFilter, Interop.IOCTL_USB_HUB_FILTER.TEST_USER)}");
            Console.WriteLine($"    Admin Call: {IoControl(hubFilter, Interop.IOCTL_USB_HUB_FILTER.TEST_ADMIN)}");
        }
    }


    static void TestStub(string deviceInterface, string entity, GENERIC_ACCESS_RIGHTS desiredAccess)
    {
        Console.WriteLine($"  As {entity}");
        using var stub = OpenDevice(deviceInterface, desiredAccess);
        if (!stub.IsInvalid)
        {
            Console.WriteLine($"    Anonymous call: {IoControl(stub, Interop.IOCTL_USB_STUB.TEST_ANONYMOUS)}");
            Console.WriteLine($"    User call: {IoControl(stub, Interop.IOCTL_USB_STUB.TEST_USER)}");
            Console.WriteLine($"    Admin call: {IoControl(stub, Interop.IOCTL_USB_STUB.TEST_ADMIN)}");
        }
    }


    static void Main()
    {
        {
            Console.WriteLine("Testing Hub Filter:");
            TestHubFilter("AnyOne", 0);
            TestHubFilter("User", GENERIC_ACCESS_RIGHTS.GENERIC_READ);
            TestHubFilter("Admin", GENERIC_ACCESS_RIGHTS.GENERIC_READ | GENERIC_ACCESS_RIGHTS.GENERIC_WRITE);
        }

        foreach (var deviceInterface in GetAllDeviceInterfaces(Interop.GUID_DEVINTERFACE_USBIPD_STUB))
        {
            Console.WriteLine();
            Console.WriteLine($"Testing {deviceInterface}");
            TestStub(deviceInterface, "AnyOne", 0);
            TestStub(deviceInterface, "User", GENERIC_ACCESS_RIGHTS.GENERIC_READ);
            TestStub(deviceInterface, "Admin", GENERIC_ACCESS_RIGHTS.GENERIC_READ | GENERIC_ACCESS_RIGHTS.GENERIC_WRITE);
        }

        Console.WriteLine("Done");
    }
}
