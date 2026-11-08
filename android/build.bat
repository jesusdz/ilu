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

rmdir /Q /S bin 2> nul
mkdir bin 2> nul


REM ------------------------------------------------------
REM Generate the R.java file into the src directory

call %AAPT% package -v -f -m  -S res -J src -M AndroidManifest.xml -I %ANDROID_PLATFORM_DIR%\android.jar


REM ------------------------------------------------------
REM Compile java code from 'src' to 'obj' directory

call %JAVAC% -source 1.7 -target 1.7 -d obj -classpath %ANDROID_PLATFORM_DIR%\android.jar -sourcepath src src\%APPLICATION_REL_PATH%\R.java
call %JAVAC% -source 1.7 -target 1.7 -d obj -classpath %ANDROID_PLATFORM_DIR%\android.jar -sourcepath src src\%APPLICATION_REL_PATH%\MainActivity.java


REM ------------------------------------------------------
REM Compile java bytecode to dex bytecode. Reference:
REM https://developer.android.com/studio/command-line/d8

call %D8% obj\%APPLICATION_REL_PATH%\* --classpath %ANDROID_PLATFORM_DIR%\android.jar --output bin\
if %errorlevel% neq 0 exit /b 1


REM ------------------------------------------------------
REM Generate the first unsigned version of the APK

call %AAPT% package -v -f -M AndroidManifest.xml -S res -I %ANDROID_PLATFORM_DIR%\android.jar -F bin\NativeActivity.unaligned.apk bin\


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
call %ADB% uninstall com.tools.game
call %ADB% install -r bin\NativeActivity.apk
call %ADB% shell mkdir -p /sdcard/Android/data/com.tools.game/files/
call %ADB% shell mkdir -p /sdcard/tmp
call %ADB% push ..\build\shaders.dat /sdcard/tmp
call %ADB% shell mv /sdcard/tmp/shaders.dat /sdcard/Android/data/com.tools.game/files/
call %ADB% push ..\build\assets.dat /sdcard/tmp
call %ADB% shell mv /sdcard/tmp/assets.dat /sdcard/Android/data/com.tools.game/files/
REM call %ADB% push ..\shaders\ /sdcard/tmp
REM call %ADB% shell mv /sdcard/tmp/shaders /sdcard/Android/data/com.tools.game/files/
REM call %ADB% push ..\assets\ /sdcard/tmp
REM call %ADB% shell mv /sdcard/tmp/assets /sdcard/Android/data/com.tools.game/files/
call %ADB% shell chmod -R 777 /sdcard/Android/data/com.tools.game/files/*
exit /b 0




REM ######################################################
REM Clean APK and compilation data
REM ######################################################
: cleanall
echo Cleaning APK data...
rmdir /Q /S intermediates bin 2> nul
del src\com\tools\game\R.java
del Tools.keystore

: clean
echo Cleaning compilation data...
rmdir /Q /S obj lib 2> nul

exit /b 0




: ERROR

echo The last command finished with error


endlocal
