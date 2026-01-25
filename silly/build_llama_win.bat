@echo off
cls

mkdir lib
mkdir build
cd build

cmake ..\..\thirdparty\llama.cpp -DBUILD_SHARED_LIBS=ON
cmake --build . --config Release

copy bin\Release\llama.dll ..\lib\llama.dll /Y
copy bin\Release\ggml.dll ..\lib\ggml.dll /Y
copy bin\Release\ggml-cpu.dll ..\lib\ggml-cpu.dll /Y
copy bin\Release\ggml-base.dll ..\lib\ggml-base.dll /Y
cd ..