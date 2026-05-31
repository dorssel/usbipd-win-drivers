========================================================================
    UsbipdStub Project Overview
========================================================================

This file contains a summary of what you will find in each of the files that make up your project.

UsbipdStub.vcxproj
    This is the main project file for projects generated using an Application Wizard. 
    It contains information about the version of the product that generated the file, and 
    information about the platforms, configurations, and project features selected with the
    Application Wizard.

UsbipdStub.vcxproj.filters
    This is the filters file for VC++ projects generated using an Application Wizard. 
    It contains information about the association between the files in your project 
    and the filters. This association is used in the IDE to show grouping of files with
    similar extensions under a specific node (for e.g. ".cpp" files are associated with the
    "Source Files" filter).

UsbipdStub.inf
    Text file used for driver installation. After creating this driver using the template, the user
    *must* update the hardware ID from "USB\VID_vvvv&PID_pppp" to the hardware ID of the USB device
    that the driver should be installed on.

public.h
    Header file to be shared with applications. 
 
driver.c & driver.h
    DriverEntry and WDFDRIVER related functionality and callbacks.
 
device.c & device.h
    WDFDEVICE and WDFUSBDEVICE related functionality and callbacks.

queue.c & queue.h
    WDFQUEUE related functionality and callbacks.

trace.h
    Definitions for WPP tracing.

/////////////////////////////////////////////////////////////////////////////

Learn more about Kernel Mode Driver Framework here:

http://msdn.microsoft.com/en-us/library/ff544296(v=VS.85).aspx

Learn more about KMDF USB APIs here:

http://msdn.microsoft.com/en-us/library/ff544752(v=vs.85).aspx

Related WDK samples and what they demonstrate:

    osrusbfx2 - bulk/interrupt transfers, continuous readers
    usbsamp - isochronous transfers

/////////////////////////////////////////////////////////////////////////////
