@echo off
call build_aio_win.bat

IF errorlevel 0 (
  cd bin
  aione-cli.exe %*
  cd ..
)