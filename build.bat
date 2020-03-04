@echo off

mkdir build
cd build

if not defined DevEnvDir (
    call "%ProgramFiles(x86)%\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvarsall.bat" x64
)
cmake .. -GNinja -DCMAKE_BUILD_TYPE=Debug
cmake --build .

cd ..
