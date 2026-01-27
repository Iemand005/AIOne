@echo off
cd build
mkdir Deployed 2>nul
cd Deployed
copy ..\BatCompiled\bin\Release\AIOne.exe . 2>nul

IF errorlevel 0 (
  C:\Qt\6.10.1\msvc2022_64\bin\windeployqt.exe --release --qmldir ..\.. AIOne.exe
  
  IF errorlevel 0 (
    cls
    AIOne.exe
  )
)

cd ..\..