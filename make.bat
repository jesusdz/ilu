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
if "%target%" == "main_spirv" goto main_spirv
if "%target%" == "main_clon" goto main_clon
if "%target%" == "data" goto data
if "%target%" == "reflection" goto reflection
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

if not exist %BuildDir%\reflex.exe (
	call :reflex
	if errorlevel 1 exit /b 1
)
pushd %BuildDir%
copy nul ..\code\reflex.generated.h > nul
reflex.exe ..\code\engine.h ..\code\game.cpp > ..\code\reflex.generated.h
if errorlevel 1 (popd & exit /b 1)
set CommonCompilerFlags=%CommonCompilerFlags% -I %RootDir%\vulkan\include
set CommonLinkerFlags=%CommonLinkerFlags% user32.lib
cl %CommonCompilerFlags% ..\code\platform.cpp /Feilu.exe /link %CommonLinkerFlags% || (popd & exit /b 1)
cl %CommonCompilerFlags% /LD ..\code\engine.cpp /Feengine.dll || (popd & exit /b 1)
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
copy nul ..\code\reflex.generated.h > nul
reflex.exe ..\code\engine.h ..\code\game.cpp > ..\code\reflex.generated.h
if errorlevel 1 (popd & exit /b 1)
set CommonCompilerFlags=%CommonCompilerFlags% -I %RootDir%\vulkan\include
cl %CommonCompilerFlags% /LD ..\code\engine.cpp /Feengine.dll || (popd & exit /b 1)
popd
exit /b 0



REM ######################################################
REM game
REM ######################################################
: game

echo No build configuration to compile the game yet.
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
REM cast
REM ######################################################
: cast

pushd %BuildDir%
cl %CommonCompilerFlags% ..\code\misc\cast.cpp /link %CommonLinkerFlags% || (popd & exit /b 1)
popd
exit /b 0



REM ######################################################
REM main_reflect_serialize
REM ######################################################
: main_reflect_serialize

pushd %BuildDir%
cl %CommonCompilerFlags% ..\code\misc\main_reflect_serialize.cpp /link %CommonLinkerFlags% || (popd & exit /b 1)
popd
exit /b 0



REM ######################################################
REM main_clon
REM ######################################################
: main_clon

pushd %BuildDir%
cl %CommonCompilerFlags% ..\code\misc\main_clon.cpp /link %CommonLinkerFlags% || (popd & exit /b 1)
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
REM Reflection
REM ######################################################
: reflection

echo %BuildDir%\reflex.exe %RootDir%\assets\assets.h ^> %RootDir%\code\assets.reflex.h
%BuildDir%\reflex.exe %RootDir%\assets\assets.h > %RootDir%\code\assets.reflex.h || exit /b 1
exit /b 0



REM ######################################################
REM Clean
REM ######################################################
: clean

rmdir /S /Q %BuildDir%
exit /b 0
