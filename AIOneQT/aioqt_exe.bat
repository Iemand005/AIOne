@echo off
cd build
mkdir Deployed 2>nul
cd Deployed
copy ..\BatCompiled\bin\Debug\AIOned.exe . 2>nul

IF errorlevel 0 (
  C:\Qt\6.10.1\msvc2022_64\bin\windeployqt.exe --qmldir ..\.. AIOned.exe
  
  IF errorlevel 0 (
    cls
    AIOned.exe
  )
)

cd ..\..