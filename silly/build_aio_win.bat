@echo off
cls

mkdir bin 2>nul
g++ -I..\include src\**.cpp -L.\lib -lllama -lggml -o bin\aione-cli.exe

IF errorlevel 0 (
  xcopy "lib\*.*" "bin\" /Y /I >nul
)