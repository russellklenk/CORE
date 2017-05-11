@ECHO OFF

:: Specify various project root paths.
SET ROOTDIR=%~dp0
SET ASSETSDIR=%~dp0assets
SET INCLUDESDIR=%~dp0include
SET MANIFESTDIR=%~dp0manifest
SET RESOURCEDIR=%~dp0res
SET SOURCESDIR=%~dp0src
SET OUTPUTDIR_X64=%~dp0bin-x64
SET OUTPUTDIR_X86=%~dp0bin-x86
SET DISTDIR_X64=%~dp0dist-x64
SET DISTDIR_X86=%~dp0dist-x86

:: Specify the version of the Visual C++ development tools to be used for the build.
SET VSVERSION_2013=12.0
SET VSVERSION_2015=14.0
SET VSVERSION_2017=2017\Professional
SET VSVERSION=%VSVERSION_2017%

:: Specify the path to the Visual C++ toolset environment setup script.
IF %VSVERSION% EQU %VSVERSION_2017% (
    SET VSTOOLSETUP="C:\Program Files (x86)\Microsoft Visual Studio\%VSVERSION_2017%\VC\Auxiliary\Build\vcvarsall.bat"
) ELSE (
    SET VSTOOLSETUP="C:\Program Files (x86)\Microsoft Visual Studio %VSVERSION%\VC\vcvarsall.bat"
)

:: Windows version constants for WINVER and _WIN32_WINNT.
SET WINVER_WIN7=0x0601
SET WINVER_WIN8=0x0602
SET WINVER_WIN81=0x0603
SET WINVER_WIN10=0x0A00

:: Windows SDK version constants. Used when building store apps.
SET WINSDK_WIN81=8.1
SET WINSDK_WIN10=10.0.10240.0
SET WINSDK_WIN10_NOV2015=10.0.10586.0
SET WINSDK_WIN10_JUL2016=10.0.14393.0

:: Set the Windows SDK version to use when building.
SET PROJECT_WINVER=%WINVER_WIN7%
SET PROJECT_WINSDK=%WINSDK_WIN81%

:: Set the project name.
SET PROJECT_NAME=CORE

:: Set the project build outputs.
SET CORE_ENGINE_VERSION_RC=CORE_version.res
SET CORE_TEST_CLIENT_OUTPUT=CORE_client.exe
SET CORE_MEMTEST_OUTPUT=CORE_memtest.exe
SET CORE_HTTP_TEST_OUTPUT=CORE_httptest.exe

:: Set the appropriate Visual C++ toolset environment.
IF /I "%1" EQU "32" (
    GOTO Setup_VCTools_x86
) ELSE (
    GOTO Setup_VCTools_x64
)

:Setup_VCTools_x86
CALL %VSTOOLSETUP% x86 %PROJECT_WINSDK%
SET OUTPUTDIR=%OUTPUTDIR_X86%
SET DISTDIR=%DISTDIR_X86%
GOTO Setup_VCTools_Finished

:Setup_VCTools_x64
CALL %VSTOOLSETUP% x64 %PROJECT_WINSDK%
SET OUTPUTDIR=%OUTPUTDIR_X64%
SET DISTDIR=%DISTDIR_X64%
GOTO Setup_VCTools_Finished

:Setup_VCTools_Finished
IF NOT EXIST "%OUTPUTDIR_X64%" MKDIR "%OUTPUTDIR_X64%"
IF NOT EXIST "%OUTPUTDIR_X86%" MKDIR "%OUTPUTDIR_X86%"
IF NOT EXIST "%DISTDIR_X64%" MKDIR "%DISTDIR_X64%"
IF NOT EXIST "%DISTDIR_X86%" MKDIR "%DISTDIR_X86%"
EXIT /b 0

