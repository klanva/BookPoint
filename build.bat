@echo off
set "MSYSTEM="
set "MINGW_PREFIX="
set "MSYSTEM_CHTYPE="
set "PATH=%PATH:C:\Program Files\Git\usr\bin;=%"
set "PYTHONIOENCODING=utf-8"
chcp 65001 >nul
cd /d "%~dp0"
pio run -e x4pro
