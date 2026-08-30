@echo off
set "MSYSTEM="
set "MINGW_PREFIX="
set "MSYSTEM_CHTYPE="
set "PATH=%PATH:C:\Program Files\Git\usr\bin;=%"
set "PATH=%PATH:C:\Program Files\Git\bin;=%"
cd /d "%~dp0"
pio run -e default
