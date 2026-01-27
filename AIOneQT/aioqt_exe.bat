@echo off
cd build
mkdir Deployed 2>nul
cd Deployed
copy ..\Desktop_Qt_6_10_1_MSVC2022_64bit-Debug\bin\AIOned.exe . 2>nul

IF errorlevel 0 (
  C:\Qt\6.10.1\msvc2022_64\bin\windeployqt.exe --qmldir ..\.. AIOned.exe
  
  IF errorlevel 0 (
    cls
    AIOned.exe
  )
)

cd ..\..