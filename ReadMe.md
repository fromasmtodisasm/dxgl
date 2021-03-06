# DXGL 0.5.16
https://www.dxgl.org

## Introduction
DXGL is a free replacement for the Windows ddraw.dll library, running on OpenGL. It is designed to overcome driver bugs, particularly in Windows Vista and newer operating systems. It also adds various enhancements to the graphics output such as display scaling and filtering options. DXGL supports the DirectX 7.0 graphics APIs, however it is currently under development and many programs are not yet compatible with DXGL.

## Updgrade notes
If you upgrade from DXGL 0.5.8 or earlier, the configuration format will be changed, and earlier versions of DXGL will no longer recognize the profiles.
In addition, any profiles generated by older versions of DXGL going forward will not be recognized by DXGL 0.5.9 or later.
If you wish to migrate any old profiles generated after installing DXGL 0.5.9 or later, you will need to use Regedit to delete the "Configuration Version" registry value from the HKEY_CURRENT_USER\SOFTWARE\DXGL registry key to force the upgrade to be run again.

## GitHub Notice
If you downloaded the DXGL source code from GitHub, please note that when compiling DXGL, the version number will not indicate the revision number.  This issue is also present when compiling from a zipped source code distribution from the www.dxgl.org, www.dxgl.info, or www.williamfeely.info website.

## System Requirements

* Windows Vista (with SP2), 7 (with SP1), 8, 8.1, or 10 (standard build)
* Windows XP (with SP3), Vista, 7, 8, 8.1, or 10 (legacy build)
  * Also compatible with most versions of Wine, by setting the ddraw DLL override to "native, builtin" which is done automatically at installation.
  * Not compatible with Windows 10 in S mode.
* OpenGL 2.0 or higher compatible video card
  * Requires support for Framebuffer objects
  * Requires support for hardware accelerated non-power-of-two textures
  * OpenGL 3.2 or higher recommended.
* The standard build requires a SSE2-capable processor; older processors require the legacy build.
* For the standard build, Visual C++ 2017 x86 runtime, available at https://aka.ms/vs/15/release/vc_redist.x86.exe (note this link may track visitors) (will be installed if not present)
* For the legacy build, Visual C++ 2010 x86 runtime, available at https://www.microsoft.com/en-us/download/details.aspx?id=8328 (will be installed if not present)

## Build Requirements
* For the legacy build, Visual Studio 2010 or Visual C++ 2010 Express Edition.
* For the standard build, Visual Studio 2017, Community or higher, Update 8.
* The following components are optional.  The build process will ask for these if they do not exist:
  * TortoiseSVN (to fill in revision on SVN builds)
  * HTML Help Workshop (to build help)
  * NSIS (to build installer, requires TortoiseSVN and HTML Help Workshop to succeed)

## Build Instructions
These instructions assume that you do not have any of the required software installed. If you already have any or all of this software installed and set up, skip those steps.
* Install Visual Studio 2017 Community at https://visualstudio.microsoft.com/
* Install TortoiseSVN from https://tortoisesvn.net/
* Install HTML Help Workshop from https://www.microsoft.com/en-us/download/details.aspx?id=21138
* Install NSIS from http://nsis.sourceforge.net/Main_Page
* Open the dxgl.sln file, select your build configuration (Debug or Release) in the toolbar, and press F7 to build.

## Progress
For detailed progress information, please check https://www.williamfeely.info/wiki/DXGL_Features
What works:
* DirectDraw object creation and destruction (versions 1 to 7)
* Display mode enumeration and switching (with emulated mode switching)
* Fullscreen and windowed modes.
* Basic Blt() functionality
* 8-bit color emulated with GLSL shader

What partially works:
* 3D graphics are only partially supported.

What doesn't work:
* Many functions are stubbed out and return an error

## Installation

Run the installer.  When the installer completes, open DXGL Config and add your program files to the config program.
To uninstall, go to the Add/Remove Programs or Programs and Features control panel and uninstall.

## SVN

SVN readonly access is available at:
https://www.dxgl.org/svn/dxgl/

There is a Mediawiki-based SVN log at:
https://www.dxgl.info/wiki/Special:Code/DXGL

## AppDB

An AppDB system (similar to that on winehq.org) is now available at:
https://www.dxgl.info/appdb/

This requires a user account separate from the other services.

Please note that the AppDB is now deprecated and will be made read-only once the new DXGL Wiki launches.

## Discussion boards

You may discuss DXGL at:
https://forum.dxgl.info

You must create a forum account to post content.  For bug reports, please refer to the next section.

## Bug reports

Bug reports are managed by a Bugzilla system available at:
https://www.dxgl.info/bugzilla/

A user account needs to be created at this site to post bug reports.
