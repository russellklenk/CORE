@ECHO OFF
IF NOT DEFINED DevEnvDir (
    CALL setenv.cmd
)
CALL build.cmd %1

