@ECHO OFF

:: Ensure that prerequisites are met.
IF [%INCLUDESDIR%] EQU [] (
    CALL setenv.cmd
)

:: Generate a file include\CORE_version.h that gets included in the various .rc files to define file version data.
:: Generate a file BUILDNUM that should be added to source control to track the current build number.
PUSHD "%ROOTDIR%"
IF NOT EXIST BUILDNUM >BUILDNUM ECHO 0
FOR /f %%x IN (BUILDNUM) DO (
    SET /a BUILDNUM_VAR=%%x+1
)
>BUILDNUM ECHO %BUILDNUM_VAR%
SET /a BUILDNUM_MAX=32767
SET /a BUILDNUM_H=(%BUILDNUM_VAR%) / (%BUILDNUM_MAX%)
SET /a BUILDNUM_L=(%BUILDNUM_VAR%) %% (%BUILDNUM_MAX%)
POPD
PUSHD "%INCLUDESDIR%"
ECHO >CORE_version.h
ECHO | SET /p="#ifndef __CORE_VERSION_H__">>CORE_version.h
ECHO.>>CORE_version.h
ECHO | SET /p="#define __CORE_VERSION_H__">>CORE_version.h
ECHO.>>CORE_version.h
ECHO.>>CORE_version.h
ECHO | SET /p="#define CORE_VERSION_STRINGIZE2(x) #x">>CORE_version.h
ECHO.>>CORE_version.h
ECHO | SET /p="#define CORE_VERSION_STRINGIZE(x)  CORE_VERSION_STRINGIZE2(x)">>CORE_version.h
ECHO.>>CORE_version.h
ECHO | SET /p="#define CORE_MAJOR_VERSION_NUMBER  1">>CORE_version.h
ECHO.>>CORE_version.h
ECHO | SET /p="#define CORE_MINOR_VERSION_NUMBER  0">>CORE_version.h
ECHO.>>CORE_version.h
ECHO | SET /p="#define CORE_BUILD_NUMBER_H        %BUILDNUM_H%">>CORE_version.h
ECHO.>>CORE_version.h
ECHO | SET /p="#define CORE_BUILD_NUMBER_L        %BUILDNUM_L%">>CORE_version.h
ECHO.>>CORE_version.h
ECHO | SET /p="#define CORE_FILE_VERSION          CORE_MAJOR_VERSION_NUMBER,CORE_MINOR_VERSION_NUMBER,CORE_BUILD_NUMBER_H,CORE_BUILD_NUMBER_L">>CORE_version.h
ECHO.>>CORE_version.h
ECHO | SET /p="#define CORE_PRODUCT_VERSION       CORE_MAJOR_VERSION_NUMBER,CORE_MINOR_VERSION_NUMBER,CORE_BUILD_NUMBER_H,CORE_BUILD_NUMBER_L">>CORE_version.h
ECHO.>>CORE_version.h
ECHO | SET /p="#define CORE_FILE_VERSION_STR      ">>CORE_version.h
ECHO | SET /p="CORE_VERSION_STRINGIZE(CORE_MAJOR_VERSION_NUMBER) ">>CORE_version.h
ECHO | SET /p=" "." ">>CORE_version.h
ECHO | SET /p="CORE_VERSION_STRINGIZE(CORE_MINOR_VERSION_NUMBER) ">>CORE_version.h
ECHO | SET /p=" "." ">>CORE_version.h
ECHO | SET /p="CORE_VERSION_STRINGIZE(CORE_BUILD_NUMBER_H) ">>CORE_version.h
ECHO | SET /p=" "." ">>CORE_version.h
ECHO | SET /p="CORE_VERSION_STRINGIZE(CORE_BUILD_NUMBER_L) ">>CORE_version.h
ECHO.>>CORE_version.h
ECHO | SET /p="#define CORE_PRODUCT_VERSION_STR   ">>CORE_version.h
ECHO | SET /p="CORE_VERSION_STRINGIZE(CORE_MAJOR_VERSION_NUMBER) ">>CORE_version.h
ECHO | SET /p=" "." ">>CORE_version.h
ECHO | SET /p="CORE_VERSION_STRINGIZE(CORE_MINOR_VERSION_NUMBER) ">>CORE_version.h
ECHO | SET /p=" "." ">>CORE_version.h
ECHO | SET /p="CORE_VERSION_STRINGIZE(CORE_BUILD_NUMBER_H) ">>CORE_version.h
ECHO | SET /p=" "." ">>CORE_version.h
ECHO | SET /p="CORE_VERSION_STRINGIZE(CORE_BUILD_NUMBER_L) ">>CORE_version.h
ECHO.>>CORE_version.h
ECHO.>>CORE_version.h
ECHO | SET /p="#endif">>CORE_version.h
ECHO.>>CORE_version.h
POPD

