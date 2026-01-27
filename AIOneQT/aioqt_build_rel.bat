@echo off
set Qt6_DIR=C:\Qt\6.10.1\msvc2022_64\lib\cmake\Qt6
cmake -B build\BatCompiled

IF errorlevel 0 (
    cmake --build build\BatCompiled --config Release
)