@ECHO OFF

:: Clean and rebuild everything.
SETLOCAL
CALL setenv.cmd
CALL clean.cmd
CALL genver.cmd
ENDLOCAL

:: Set up and build the x86 configuration.
SETLOCAL
CALL setenv.cmd 32
CALL build.cmd
IF %ERRORLEVEL% NEQ 0 (
    GOTO Build_Failed
)
ENDLOCAL

:: Set up and build the x64 configuration.
SETLOCAL
CALL setenv.cmd 64
CALL build.cmd
IF %ERRORLEVEL% NEQ 0 (
    GOTO Build_Failed
)
ENDLOCAL

:: Set up for the packaging process.
SETLOCAL
CALL setenv.cmd

ECHO Packaging x86 distribution from "%OUTPUTDIR_X86%" to "%DISTDIR_X86%".
XCOPY /Y /Q "%OUTPUTDIR_X86%\%CORE_TEST_CLIENT_OUTPUT%" "%DISTDIR_X86%"
XCOPY /Y /Q "%OUTPUTDIR_X86%\%CORE_MEMTEST_OUTPUT%" "%DISTDIR_X86%"
XCOPY /Y /Q "%OUTPUTDIR_X86%\%CORE_HTTP_TEST_OUTPUT%" "%DISTDIR_X86%"

ECHO Packaging x64 distribution from "%OUTPUTDIR_X64%" to "%DISTDIR_X64%".
XCOPY /Y /Q "%OUTPUTDIR_X64%\%CORE_TEST_CLIENT_OUTPUT%" "%DISTDIR_X64%"
XCOPY /Y /Q "%OUTPUTDIR_X64%\%CORE_MEMTEST_OUTPUT%" "%DISTDIR_X64%"
XCOPY /Y /Q "%OUTPUTDIR_X64%\%CORE_HTTP_TEST_OUTPUT%" "%DISTDIR_X64%"

ECHO Compressing distribution.
SET QUERY_FILE=%OUTPUTDIR_X86%\%CORE_TEST_CLIENT_OUTPUT%
SET QUERY_FILE=%QUERY_FILE:\=\\%
FOR /f "usebackq delims=" %%a IN (`"WMIC DATAFILE WHERE name='%QUERY_FILE%' get Version /format:Textvaluelist"`) DO (
    FOR /f "delims=" %%# IN ("%%a") DO SET "%%#"
)
:: Parse the version string #.#.#.# into discrete variables.
FOR /f "tokens=1-4 delims=. " %%a IN ("%VERSION%") DO (
    SET /a VERSION_MAJOR=%%a
    SET /a VERSION_MINOR=%%b
    SET /a VERSION_BUILD_MAJOR=%%c
    SET /a VERSION_BUILD_MINOR=%%d
)
SET /a VERSION_BUILD_HIGH=%VERSION_BUILD_MAJOR%*32767
SET /a VERSION_BUILD=%VERSION_BUILD_HIGH%+%VERSION_BUILD_MINOR%
PUSHD "%ROOTDIR%"
7za a -tzip %PROJECT_NAME%-x86-%VERSION_MAJOR%.%VERSION_MINOR%.%VERSION_BUILD%.zip "%DISTDIR_X86%\" >nul
7za a -tzip %PROJECT_NAME%-x64-%VERSION_MAJOR%.%VERSION_MINOR%.%VERSION_BUILD%.zip "%DISTDIR_X64%\" >nul
POPD
ECHO Created redistributable file %PROJECT_NAME%-x86-%VERSION_MAJOR%.%VERSION_MINOR%.%VERSION_BUILD%.zip.
ECHO Created redistributable file %PROJECT_NAME%-x64-%VERSION_MAJOR%.%VERSION_MINOR%.%VERSION_BUILD%.zip.
ECHO DONE.
ENDLOCAL
EXIT /b 0

:Build_Failed
ECHO Build failed. Packaging cannot continue.
ENDLOCAL
EXIT /b 1

