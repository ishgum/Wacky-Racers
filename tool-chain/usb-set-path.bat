@ECHO OFF
cls

set GCC_EXEC_PREFIX=
set DEV_PATH=%~dp0

SET PATH=%DEV_PATH%msys\1.0\bin;%DEV_PATH%GnuArmEmb4_9_2014q4\bin;%DEV_PATH%openocd-0.9.0-rc1\bin-x64;%PATH%

ECHO PATH SET FOR ARM EMBEDDED DEVELOPMENT.
ECHO SEE %DEV_PATH% FOR THE FILES.
ECHO **************************************************
ls %DEV_PATH%GnuArmEmb4_9_2014q4\bin