@echo off
setlocal



REM Prepare the VC compiler environment

set RootDir=%~dp0
set RootDir=%RootDir:~0,-1%

IF NOT EXIST %RootDir%\build mkdir %RootDir%\build
set BuildDir=%RootDir%\build
set DataDir=%BuildDir%

IF NOT EXIST %DataDir%\shaders mkdir %DataDir%\shaders
set DataShadersDir=%DataDir%\shaders

set DXC=%RootDir%\dxc\windows\bin\x64\dxc.exe

REM set CommonCompilerFlags=-diagnostics:column -WL -O2 -nologo -fp:fast -fp:except- -Gm- -GR- -EHa- -Zo -Oi -WX -W4 -wd4201 -wd4100 -wd4189 -wd4505 -wd4127 -FC -Z7 -GS- -Gs9999999
REM set CommonCompilerFlags=-DHANDMADE_INTERNAL=1 -DHANDMADE_SLOW=1 -DHANDMADE_WIN32=1 %CommonCompilerFlags%
REM set CommonLinkerFlags=-STACK:0x100000,0x100000 -incremental:no -opt:ref user32.lib gdi32.lib winmm.lib bcrypt.lib kernel32.lib

REM Flags
set CommonCompilerFlags=-Zi -D_CRT_SECURE_NO_WARNINGS -DDEVELOPMENT_BUILD -std:c++20 -nologo
set CommonLinkerFlags=-incremental:no -opt:ref -nologo

REM Flags for preprocessor pass
REM set CommonCompilerFlags=-P -Fipreprocessed.cpp
REM set CommonLinkerFlags=




REM ######################################################
REM Target to build
REM ######################################################

set target=%1
if [%target%] == [] goto ilu
if "%target%" == "unit_test_ilu" goto unit_test_ilu
if "%target%" == "cast" goto cast
if "%target%" == "reflex" goto reflex
if "%target%" == "ilu" goto ilu
if "%target%" == "dll" goto dll
if "%target%" == "game" goto game
if "%target%" == "main_d3d12" goto main_d3d12
if "%target%" == "main_reflect_serialize" goto main_reflect_serialize
if "%target%" == "main_interpreter" goto main_interpreter
if "%target%" == "main_atof" goto main_atof
if "%target%" == "data" goto data
if "%target%" == "reflection" goto reflection
if "%target%" == "clean" goto clean
echo Invalid target
exit /b 1



REM ######################################################
REM Reflex
REM ######################################################
: reflex

call vcenv.bat
pushd build
set CommonCompilerFlags=%CommonCompilerFlags%
set CommonLinkerFlags=%CommonLinkerFlags%
cl %CommonCompilerFlags% ..\code\misc\reflex.cpp /link %CommonLinkerFlags%
popd
exit /b 0



REM ######################################################
REM ilu
REM ######################################################
: ilu

call vcenv.bat
pushd build
copy nul ..\code\game.generated.h > nul
set CommonCompilerFlags=%CommonCompilerFlags% -I %RootDir%\vulkan\include
set CommonLinkerFlags=%CommonLinkerFlags% user32.lib
cl %CommonCompilerFlags% ..\code\platform.cpp /Feilu.exe /link %CommonLinkerFlags%
cl %CommonCompilerFlags% /LD ..\code\engine.cpp /Feengine.dll
popd
exit /b 0



REM ######################################################
REM DLL
REM ######################################################
: dll

call vcenv.bat
pushd build
copy nul ..\code\game.generated.h > nul
set CommonCompilerFlags=%CommonCompilerFlags% -I %RootDir%\vulkan\include
cl %CommonCompilerFlags% /LD ..\code\engine.cpp /Feengine.dll
popd
exit /b 0



REM ######################################################
REM game
REM ######################################################
: game

call vcenv.bat
pushd build
echo No build configuration to compile the game yet.
popd
exit /b 0



REM ######################################################
REM main_interpreter
REM ######################################################
: main_interpreter

call vcenv.bat
set CommonCompilerFlags=%CommonCompilerFlags%
set CommonLinkerFlags=%CommonLinkerFlags%
pushd build
cl %CommonCompilerFlags% ..\code\misc\main_interpreter.cpp /link %CommonLinkerFlags%
popd
exit /b 0



REM ######################################################
REM main_d3d12
REM ######################################################
: main_d3d12

call vcenv.bat
set CommonCompilerFlags=%CommonCompilerFlags%
set CommonLinkerFlags=%CommonLinkerFlags% d3d12.lib dxgi.lib dxguid.lib user32.lib d3dcompiler.lib
pushd build
cl %CommonCompilerFlags% ..\code\misc\main_d3d12.cpp /link %CommonLinkerFlags%
popd
exit /b 0



REM ######################################################
REM main_atof
REM ######################################################
: main_atof

call vcenv.bat
set CommonCompilerFlags=%CommonCompilerFlags%
set CommonLinkerFlags=%CommonLinkerFlags%
pushd build
cl %CommonCompilerFlags% ..\code\misc\main_atof.cpp /link %CommonLinkerFlags%
popd
exit /b 0



REM ######################################################
REM main_spirv
REM ######################################################
: main_spirv

call vcenv.bat
set CommonCompilerFlags=%CommonCompilerFlags%
set CommonLinkerFlags=%CommonLinkerFlags%
pushd build
cl %CommonCompilerFlags% ..\code\misc\main_spirv.cpp /link %CommonLinkerFlags%
popd
exit /b 0



REM ######################################################
REM unit_test_ilu
REM ######################################################
: unit_test_ilu

call vcenv.bat
set CommonCompilerFlags=%CommonCompilerFlags%
set CommonLinkerFlags=%CommonLinkerFlags%
pushd build
cl %CommonCompilerFlags% ..\code\tests\unit_test_ilu.cpp /link %CommonLinkerFlags%
popd
exit /b 0



REM ######################################################
REM cast
REM ######################################################
: cast

call vcenv.bat
set CommonCompilerFlags=%CommonCompilerFlags%
set CommonLinkerFlags=%CommonLinkerFlags%
pushd build
cl %CommonCompilerFlags% ..\code\misc\cast.cpp /link %CommonLinkerFlags%
popd
exit /b 0



REM ######################################################
REM main_reflect_serialize
REM ######################################################
: main_reflect_serialize

call vcenv.bat
set CommonCompilerFlags=%CommonCompilerFlags%
set CommonLinkerFlags=%CommonLinkerFlags%
pushd build
cl %CommonCompilerFlags% ..\code\misc\main_reflect_serialize.cpp /link %CommonLinkerFlags%
popd
exit /b 0



REM ######################################################
REM Shaders
REM ######################################################
: data

pushd shaders

%DXC% -spirv -T vs_6_7 -Fo %DataShadersDir%\vs_shading.spv     -Fc %DataShadersDir%\vs_shading.dis     shading.hlsl -E VSMain
%DXC% -spirv -T ps_6_7 -Fo %DataShadersDir%\fs_shading.spv     -Fc %DataShadersDir%\fs_shading.dis     shading.hlsl -E PSMain
%DXC% -spirv -T vs_6_7 -Fo %DataShadersDir%\vs_sky.spv         -Fc %DataShadersDir%\vs_sky.dis         sky.hlsl -E VSMain
%DXC% -spirv -T ps_6_7 -Fo %DataShadersDir%\fs_sky.spv         -Fc %DataShadersDir%\fs_sky.dis         sky.hlsl -E PSMain
%DXC% -spirv -T vs_6_7 -Fo %DataShadersDir%\vs_shadowmap.spv   -Fc %DataShadersDir%\vs_shadowmap.dis   shadowmap.hlsl -E VSMain
%DXC% -spirv -T ps_6_7 -Fo %DataShadersDir%\fs_shadowmap.spv   -Fc %DataShadersDir%\fs_shadowmap.dis   shadowmap.hlsl -E PSMain
%DXC% -spirv -T vs_6_7 -Fo %DataShadersDir%\vs_ui.spv          -Fc %DataShadersDir%\vs_ui.dis          ui.hlsl -E VSMain
%DXC% -spirv -T ps_6_7 -Fo %DataShadersDir%\fs_ui.spv          -Fc %DataShadersDir%\fs_ui.dis          ui.hlsl -E PSMain
%DXC% -spirv -T vs_6_7 -Fo %DataShadersDir%\vs_id.spv          -Fc %DataShadersDir%\vs_id.dis          id.hlsl -E VSMain
%DXC% -spirv -T ps_6_7 -Fo %DataShadersDir%\fs_id.spv          -Fc %DataShadersDir%\fs_id.dis          id.hlsl -E PSMain
%DXC% -spirv -T cs_6_7 -Fo %DataShadersDir%\compute_select.spv -Fc %DataShadersDir%\compute_select.dis compute_select.hlsl -E CSMain
%DXC% -spirv -T cs_6_7 -Fo %DataShadersDir%\compute_clear.spv  -Fc %DataShadersDir%\compute_clear.dis  compute.hlsl -E main_clear
%DXC% -spirv -T cs_6_7 -Fo %DataShadersDir%\compute_update.spv -Fc %DataShadersDir%\compute_update.dis compute.hlsl -E main_update

REM if "%target%"=="main_d3d12.cpp" (
REM 	%DXC% -nologo -E main -T vs_6_0 -Zi -Fd vertex.pdb -Fo vertex.cso vertex.hlsl
REM 	%DXC% -nologo -E main -T ps_6_0 -Zi -Fd fragment.pdb -Fo fragment.cso fragment.hlsl
REM )

popd

exit /b 0



REM ######################################################
REM Reflection
REM ######################################################
: reflection

echo build\reflex.exe assets\assets.h > code\assets.reflex.h
build\reflex.exe assets\assets.h > code\assets.reflex.h
exit /b 0



REM ######################################################
REM Clean
REM ######################################################
: clean

rmdir /S /Q build\
exit /b 0



endlocal
