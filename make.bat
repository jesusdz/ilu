@echo off

REM Prepare the VC compiler environment
call "%~dp0vcenv.bat"

setlocal

set RootDir=%~dp0
set RootDir=%RootDir:~0,-1%

set BuildDir=%RootDir%\build
IF NOT EXIST %BuildDir% mkdir %BuildDir%
set DataDir=%BuildDir%

set DataShadersDir=%DataDir%\shaders
IF NOT EXIST %DataShadersDir% mkdir %DataShadersDir%

REM Flags
set CommonCompilerFlags=-Zi -D_CRT_SECURE_NO_WARNINGS -DDEVELOPMENT_BUILD -std:c++20 -nologo
set CommonLinkerFlags=-incremental:no -opt:ref -nologo
set VulkanInclude=-I %RootDir%\vulkan\include

REM Flags for preprocessor pass
REM set CommonCompilerFlags=-P -Fipreprocessed.cpp
REM set CommonLinkerFlags=




REM ######################################################
REM Target to build
REM ######################################################

set target=%1
if [%target%] == [] goto ilu
if "%target%" == "unit_test_ilu" goto unit_test_ilu
if "%target%" == "main_cast" goto main_cast
if "%target%" == "reflex" goto reflex
if "%target%" == "ilu" goto ilu
if "%target%" == "dll" goto dll
if "%target%" == "main_d3d12" goto main_d3d12
if "%target%" == "main_interpreter" goto main_interpreter
if "%target%" == "main_atof" goto main_atof
if "%target%" == "main_spirv" goto main_spirv
if "%target%" == "data" goto data
if "%target%" == "clean" goto clean
echo Invalid target
exit /b 1



REM ######################################################
REM Reflex
REM ######################################################
: reflex

pushd %BuildDir%
cl %CommonCompilerFlags% ..\code\reflex\reflex.cpp /link %CommonLinkerFlags% || (popd & exit /b 1)
popd
exit /b 0



REM ######################################################
REM ilu
REM ######################################################
: ilu

call :dll
if errorlevel 1 exit /b 1
pushd %BuildDir%
cl %CommonCompilerFlags% %VulkanInclude% ..\code\platform.cpp /Feilu.exe /link %CommonLinkerFlags% user32.lib || (popd & exit /b 1)
popd
exit /b 0



REM ######################################################
REM DLL
REM ######################################################
: dll

if not exist %BuildDir%\reflex.exe (
	call :reflex
	if errorlevel 1 exit /b 1
)
pushd %BuildDir%
reflex.exe ..\code\engine.h ..\code\game.cpp > ..\code\reflex.generated.h
if errorlevel 1 (popd & exit /b 1)
cl %CommonCompilerFlags% %VulkanInclude% /LD ..\code\engine.cpp /Feengine.dll || (popd & exit /b 1)
popd
exit /b 0



REM ######################################################
REM main_interpreter
REM ######################################################
: main_interpreter

pushd %BuildDir%
cl %CommonCompilerFlags% ..\code\misc\main_interpreter.cpp /link %CommonLinkerFlags% || (popd & exit /b 1)
popd
exit /b 0



REM ######################################################
REM main_d3d12
REM ######################################################
: main_d3d12

set CommonLinkerFlags=%CommonLinkerFlags% d3d12.lib dxgi.lib dxguid.lib user32.lib d3dcompiler.lib
pushd %BuildDir%
cl %CommonCompilerFlags% ..\code\misc\main_d3d12.cpp /link %CommonLinkerFlags% || (popd & exit /b 1)
popd
exit /b 0



REM ######################################################
REM main_atof
REM ######################################################
: main_atof

pushd %BuildDir%
cl %CommonCompilerFlags% ..\code\misc\main_atof.cpp /link %CommonLinkerFlags% || (popd & exit /b 1)
popd
exit /b 0



REM ######################################################
REM main_spirv
REM ######################################################
: main_spirv

pushd %BuildDir%
cl %CommonCompilerFlags% ..\code\misc\main_spirv.cpp /link %CommonLinkerFlags% || (popd & exit /b 1)
popd
exit /b 0



REM ######################################################
REM unit_test_ilu
REM ######################################################
: unit_test_ilu

pushd %BuildDir%
cl %CommonCompilerFlags% ..\code\tests\unit_test_ilu.cpp /link %CommonLinkerFlags% || (popd & exit /b 1)
popd
exit /b 0



REM ######################################################
REM main_cast
REM ######################################################
: main_cast

pushd %BuildDir%
cl %CommonCompilerFlags% ..\code\misc\main_cast.cpp /link %CommonLinkerFlags% || (popd & exit /b 1)
popd
exit /b 0



REM ######################################################
REM Data
REM ######################################################
: data

call :ilu
if errorlevel 1 exit /b 1
pushd %RootDir%
%BuildDir%\ilu.exe --build-assets || (popd & exit /b 1)
popd
exit /b 0



REM ######################################################
REM Clean
REM ######################################################
: clean

rmdir /S /Q %BuildDir%
exit /b 0
