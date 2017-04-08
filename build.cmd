@ECHO OFF

:: Ensure that prerequisites are met.
IF [%INCLUDESDIR%] EQU [] (
    CALL setenv.cmd
)
IF NOT EXIST "%INCLUDESDIR%\CORE_version.h" (
    CALL genver.cmd
)

:: Process command-line arguments passed to the script.
:Process_Argument
IF [%1] EQU [] GOTO Default_Arguments
IF /I "%1" == "debug" SET BUILD_CONFIGURATION=debug
IF /I "%1" == "release" SET BUILD_CONFIGURATION=release
SHIFT
GOTO Process_Argument

:: Default any unspecified command-line arguments.
:Default_Arguments
IF [%BUILD_CONFIGURATION%] EQU [] SET BUILD_CONFIGURATION=release

:: Define the project resource input filename(s).
SET CORE_ENGINE_VERSION_RESOURCES=CORE_version.rc

:: Define the project entry point filename(s).
SET CORE_TEST_CLIENT_MAIN=CORE_client.c

:: Define the libraries that each output links with.
SET CORE_TEST_CLIENT_LIBRARIES=User32.lib Gdi32.lib Shell32.lib Advapi32.lib winmm.lib

:: Define common compilation and linking variables shared by all projects.
SET DEFINES_COMMON=/D WINVER=%PROJECT_WINVER% /D _WIN32_WINNT=%PROJECT_WINVER% /D UNICODE /D _UNICODE /D _STDC_FORMAT_MACROS /D _CRT_SECURE_NO_WARNINGS
SET DEFINES_COMMON_DEBUG=%DEFINES_COMMON% /D DEBUG /D _DEBUG
SET DEFINES_COMMON_RELEASE=%DEFINES_COMMON% /D NDEBUG /D _NDEBUG
SET INCLUDES_COMMON=%VULKAN_INCLUDES% -I"%INCLUDESDIR%" -I"%MANIFESTDIR%" -I"%RESOURCEDIR%" -I"%SOURCESDIR%"
SET CPPFLAGS_COMMON=%INCLUDES_COMMON% /FC /nologo /W4 /WX /wd4505 /Zi /EHsc
SET CPPFLAGS_COMMON_DEBUG=%CPPFLAGS_COMMON% /Od
SET CPPFLAGS_COMMON_RELEASE=%CPPFLAGS_COMMON% /Ob2it

:: Select the appropriate set of per-project variables based on the build configuration.
IF /I "%BUILD_CONFIGURATION%" == "release" (
    SET CORE_TEST_CLIENT_DEFINES=%DEFINES_COMMON_RELEASE%
    SET CORE_TEST_CLIENT_CPPFLAGS=%CPPFLAGS_COMMON_RELEASE%
    SET CORE_TEST_CLIENT_LNKFLAGS=%CORE_TEST_CLIENT_LIBRARIES% /MT
) ELSE (
    SET CORE_TEST_CLIENT_DEFINES=%DEFINES_COMMON_DEBUG%
    SET CORE_TEST_CLIENT_CPPFLAGS=%CPPFLAGS_COMMON_DEBUG%
    SET CORE_TEST_CLIENT_LNKFLAGS=%CORE_TEST_CLIENT_LIBRARIES% /MTd
)

:: Ensure the output directory exists.
IF NOT EXIST "%OUTPUTDIR%" MKDIR "%OUTPUTDIR%"
SET BUILD_FAILED=

:: Generate resources for all other projects.
PUSHD "%RESOURCEDIR%"
rc.exe /nologo %INCLUDES_COMMON% %CORE_ENGINE_VERSION_RESOURCES%
IF %ERRORLEVEL% NEQ 0 (
    ECHO ERROR: Resource compile failed for %CORE_ENGINE_VERSION_RESOURCES%
    SET BUILD_FAILED=1
)
POPD

:: Compile DLLs and executables.
PUSHD "%OUTPUTDIR%"
cl.exe %CORE_TEST_CLIENT_CPPFLAGS% "%SOURCESDIR%\%CORE_TEST_CLIENT_MAIN%" %CORE_TEST_CLIENT_DEFINES% %CORE_TEST_CLIENT_LNKFLAGS% /Fe%CORE_TEST_CLIENT_OUTPUT% /link "%RESOURCEDIR%\%CORE_ENGINE_VERSION_RC%"
IF %ERRORLEVEL% NEQ 0 (
    ECHO ERROR: Build failed for %CORE_TEST_CLIENT_OUTPUT%.
    SET BUILD_FAILED=1
)
POPD
GOTO Check_Build

:Check_Build
IF [%BUILD_FAILED%] NEQ [] (
    GOTO Build_Failed
) ELSE (
    GOTO Build_Succeeded
)

:Build_Failed
ECHO BUILD FAILED.
EXIT /B 1

:Build_Succeeded
ECHO BUILD SUCCEEDED.
EXIT /B 0

