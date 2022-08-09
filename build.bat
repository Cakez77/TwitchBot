@echo off

SetLocal EnableDelayedExpansion
if not defined DevEnvDir (
    call "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvars64.bat"
)

SET includeFlags=/Isrc
SET linkerFlags=/link Winhttp.lib Ws2_32.lib Httpapi.lib Shell32.lib Dsound.lib User32.lib
SET defines=/D DEBUG

echo "Building main..."

if not exist build\NUL mkdir build

cl /nologo /FC /EHsc /Z7 /std:c++17 /Fe"main" /Fobuild/ %defines% %includeFlags% src/win32_platform.cpp %linkerFlags%