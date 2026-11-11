setlocal
@echo off

REM ######################################################
REM Configure environment variables
REM ######################################################

call environment.bat

if %errorlevel% neq 0 (
	echo Environment not ready.
	exit /b 1
)




REM ######################################################
REM Target to build
REM ######################################################

set target=%1
if [%target%] == [] goto build
if "%target%" == "build" goto build
if "%target%" == "apk" goto apk
if "%target%" == "install" goto install
if "%target%" == "clean" goto clean
if "%target%" == "cleanall" goto cleanall

echo Invalid target
exit /b 0




REM ######################################################
REM Compile the C native library
REM ######################################################
: build

REM ------------------------------------------------------
REM Compile native Android code

rmdir /S /Q obj lib 2> nul
mkdir obj lib 2> nul
mkdir %OUT_LIB_DIR% 2> nul

pushd obj

set CODE_DIR="../../code"
set INCLUDES=-I../../ -I../../vulkan/include
set GCC_FLAGS=-fPIC
set GCC_FLAGS=%GCC_FLAGS% -g -O0
set GCC_FLAGS=%GCC_FLAGS% -DDEVELOPMENT_BUILD
set GXX_FLAGS=%GCC_FLAGS% -std=gnu++20
set LINK_FLAGS=-shared -Wl,-z,max-page-size=16384

REM Have a look at this compile arguments
REM /Users/thomas/Documents/android-ndk-r5b/toolchains/arm-eabi-4.4.0/prebuilt/darwin-x86/bin/arm-eabi-g++ --sysroot=/Users/thomas/Documents/android-ndk-r5b/platforms/android-8/arch-arm -march=armv7-a -mfloat-abi=softfp -mfpu=neon -Wl,--fix-cortex-a8 -fno-exceptions -fno-rtti -nostdlib -fpic -shared -o GLmove.so -O3

echo:
echo Compiling native Android code...

echo %GCC% %GCC_FLAGS% -c %NATIVE_APP_GLUE_DIR%\android_native_app_glue.c -o android_native_app_glue.o
call %GCC% %GCC_FLAGS% -c %NATIVE_APP_GLUE_DIR%\android_native_app_glue.c -o android_native_app_glue.o

echo %AR% rcs libandroid_native_app_glue.a android_native_app_glue.o
call %AR% rcs libandroid_native_app_glue.a android_native_app_glue.o

echo %GXX% --sysroot=%TOOLCHAIN%\sysroot -I%NATIVE_APP_GLUE_DIR% %INCLUDES% %GXX_FLAGS% -c %CODE_DIR%\platform.cpp -o main.o
call %GXX% --sysroot=%TOOLCHAIN%\sysroot -I%NATIVE_APP_GLUE_DIR% %INCLUDES% %GXX_FLAGS% -c %CODE_DIR%\platform.cpp -o main.o

echo %GXX% main.o -L. -landroid_native_app_glue -u ANativeActivity_onCreate -landroid -llog -laaudio %LINK_FLAGS% -o ..\%OUT_LIB_DIR%\libgame.so
call %GXX% main.o -L. -landroid_native_app_glue -u ANativeActivity_onCreate -landroid -llog -laaudio %LINK_FLAGS% -o ..\%OUT_LIB_DIR%\libgame.so

echo %GXX% --sysroot=%TOOLCHAIN%\sysroot -I%NATIVE_APP_GLUE_DIR% %INCLUDES% %GXX_FLAGS% %CODE_DIR%\engine.cpp -llog %LINK_FLAGS% -o ..\%OUT_LIB_DIR%\libengine.so
call %GXX% --sysroot=%TOOLCHAIN%\sysroot -I%NATIVE_APP_GLUE_DIR% %INCLUDES% %GXX_FLAGS% %CODE_DIR%\engine.cpp -llog %LINK_FLAGS% -o ..\%OUT_LIB_DIR%\libengine.so

popd

exit /b 0




REM ######################################################
REM Make the APK (Android PacKage)
REM ######################################################
: apk


REM ------------------------------------------------------
REM Start with a fresh bin directory

rmdir /Q /S bin intermediates 2> nul
mkdir bin intermediates 2> nul


REM ------------------------------------------------------
REM Compile resources and link them into the first unsigned
REM version of the APK. Also generates R.java into src.

call %AAPT2% compile --dir res -o intermediates\res.zip
if %errorlevel% neq 0 exit /b 1

call %AAPT2% link -I %ANDROID_PLATFORM_DIR%\android.jar --manifest AndroidManifest.xml --rename-manifest-package %PACKAGE_NAME% --java src -o bin\NativeActivity.unaligned.apk intermediates\res.zip
if %errorlevel% neq 0 exit /b 1


REM ------------------------------------------------------
REM Compile all java code in 'src' (including R.java) into
REM 'intermediates\classes' and pack it into a jar

mkdir intermediates\classes 2> nul
for /r src %%f in (*.java) do (
	call %JAVAC% --release 11 -d intermediates\classes -classpath %ANDROID_PLATFORM_DIR%\android.jar -sourcepath src "%%f"
	if errorlevel 1 exit /b 1
)
call %JAR% cf intermediates\classes.jar -C intermediates\classes .
if %errorlevel% neq 0 exit /b 1


REM ------------------------------------------------------
REM Compile java bytecode to dex bytecode. Reference:
REM https://developer.android.com/studio/command-line/d8

call %D8% intermediates\classes.jar --min-api 24 --classpath %ANDROID_PLATFORM_DIR%\android.jar --output bin\
if %errorlevel% neq 0 exit /b 1


REM ------------------------------------------------------
REM Add classes.dex to the root of the APK

pushd bin
call %AAPT% add NativeActivity.unaligned.apk classes.dex
popd


REM ------------------------------------------------------
REM Add the native activity and other libs to the APK

call %AAPT% add bin\NativeActivity.unaligned.apk "%AAPT_LIB_DIR%/libgame.so"
call %AAPT% add bin\NativeActivity.unaligned.apk "%AAPT_LIB_DIR%/libengine.so"

copy "%TOOLCHAIN_LIB_DIR%\libc++_shared.so" %OUT_LIB_DIR%\
call %AAPT% add bin\NativeActivity.unaligned.apk "%AAPT_LIB_DIR%/libc++_shared.so"

REM copy "..\vulkan\lib\arm64_v8a\libVkLayer_khronos_validation.so" %OUT_LIB_DIR%\
REM call %AAPT% add bin\NativeActivity.unaligned.apk "%AAPT_LIB_DIR%/libVkLayer_khronos_validation.so"


REM ------------------------------------------------------
REM Align the resources of the final APK to 4 bytes

call %ZIPALIGN% -v -f 4 bin\NativeActivity.unaligned.apk bin\NativeActivity.apk || goto ERROR


REM ------------------------------------------------------
REM Create a key and sign the APK with it

if not exist "Tools.keystore" call %KEYTOOL% -genkeypair -validity 1000 -dname "CN=tools,O=jesusdz,C=ES" -keystore Tools.keystore -storepass pass4tools -keypass pass4tools -alias ToolsKey -keyalg RSA
call %APKSIGNER% sign --ks Tools.keystore --ks-key-alias ToolsKey --ks-pass pass:"pass4tools" bin\NativeActivity.apk

exit /b 0




REM ######################################################
REM Install APK into the device
REM ######################################################
: install
REM -k argument would keep the data and cache directories
REM Only uninstall if already installed (avoids DELETE_FAILED_INTERNAL_ERROR)
call %ADB% shell pm list packages %PACKAGE_NAME% | findstr /c:"package:%PACKAGE_NAME%" > nul
if %errorlevel% equ 0 call %ADB% uninstall %PACKAGE_NAME%
call %ADB% install -r bin\NativeActivity.apk
call %ADB% shell mkdir -p %DEVICE_DATA_DIR%/
call %ADB% shell mkdir -p /sdcard/tmp
call %ADB% push ..\build\shaders.dat /sdcard/tmp
call %ADB% shell mv /sdcard/tmp/shaders.dat %DEVICE_DATA_DIR%/
call %ADB% push ..\build\assets.dat /sdcard/tmp
call %ADB% shell mv /sdcard/tmp/assets.dat %DEVICE_DATA_DIR%/
REM call %ADB% push ..\shaders\ /sdcard/tmp
REM call %ADB% shell mv /sdcard/tmp/shaders %DEVICE_DATA_DIR%/
REM call %ADB% push ..\assets\ /sdcard/tmp
REM call %ADB% shell mv /sdcard/tmp/assets %DEVICE_DATA_DIR%/
call %ADB% shell chmod -R 777 %DEVICE_DATA_DIR%/*
exit /b 0




REM ######################################################
REM Clean APK and compilation data
REM ######################################################
: cleanall
echo Cleaning APK data...
rmdir /Q /S intermediates bin 2> nul
del /s /q src\R.java 2> nul
del Tools.keystore

: clean
echo Cleaning compilation data...
rmdir /Q /S obj lib 2> nul

exit /b 0




: ERROR

echo The last command finished with error


endlocal
