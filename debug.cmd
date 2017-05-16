@ECHO OFF

SETLOCAL 

:: Ensure prerequisites are met.
IF [%INCLUDESDIR%] EQU [] (
    CALL setenv.cmd
)

:: Process command-line arguments passed to the script.
:Process_Argument
IF [%1] EQU [] GOTO Default_Arguments
IF /I "%1" == "client" SET TARGET_OUTPUT=%CORE_TEST_CLIENT_OUTPUT%
IF /I "%1" == "memtest" SET TARGET_OUTPUT=%CORE_MEMTEST_OUTPUT%
IF /I "%1" == "httptest" SET TARGET_OUTPUT=%CORE_HTTP_TEST_OUTPUT%
IF /I "%1" == "tasktest" SET TARGET_OUTPUT=%CORE_TASK_TEST_OUTPUT%
SHIFT
GOTO Process_Argument

:: Default any unspecified command-line arguments.
:Default_Arguments
IF [%TARGET_OUTPUT%] EQU [] (
    ECHO No target specified; expected "client", "memtest", "httptest" or "tasktest". Aborting debug session.
    GOTO Abort_Debug
)
IF NOT EXIST "%OUTPUTDIR%" (
    ECHO Output directory "%OUTPUTDIR%" not found; building...
    CALL build.cmd
)
IF EXIST "%OUTPUTDIR%\%TARGET_OUTPUT%" (
    start devenv /debugexe "%OUTPUTDIR%\%TARGET_OUTPUT%"
    ENDLOCAL
    EXIT /b 0
) ELSE (
    ECHO Build failed; aborting debug session.
    GOTO Abort_Debug
)

:Abort_Debug
ENDLOCAL
EXIT /b 1

