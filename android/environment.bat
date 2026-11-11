
REM ######################################################
REM Java / Android paths
REM Make these variables point to java/android directories
REM ######################################################

if not defined JAVA_HOME set JAVA_HOME="C:\path\to\java\home"
if not defined ANDROID_HOME set ANDROID_HOME="C:\path\to\android\home"



REM ######################################################
REM JAVA
REM ######################################################

if not exist %JAVA_HOME% (
	echo Configure JAVA_HOME in environment.bat.
	exit /b 1
)

set JAVAC=%JAVA_HOME%\bin\javac
set JAR=%JAVA_HOME%\bin\jar
set KEYTOOL=%JAVA_HOME%\bin\keytool



REM ######################################################
REM SDK
REM ######################################################

if not exist %ANDROID_HOME% (
	echo Configure ANDROID_HOME in environment.bat.
	exit /b 1
)

set ANDROID_PLATFORM_DIR=%ANDROID_HOME%\platforms\android-36

if not exist %ANDROID_PLATFORM_DIR% (
	echo Could not find %ANDROID_PLATFORM_DIR% directory. Please install.
	exit /b 1
)



REM ######################################################
REM NDK / TOOLCHAIN
REM ######################################################

set HOST=windows-x86_64
set NDK=%ANDROID_HOME%\ndk\30.0.16248370
set TOOLCHAIN=%NDK%\toolchains\llvm\prebuilt\%HOST%

if not exist %NDK% (
	echo Could not find %NDK% directory. Please install.
	exit /b 1
)
if not exist %TOOLCHAIN% (
	echo Could not find %TOOLCHAIN% directory. Please install.
	exit /b 1
)

REM clang.exe --print-targets to see a full list of targets
REM set TARGET=arm64-linux-android33
set TARGET=aarch64-linux-android33

set GCC=%TOOLCHAIN%\bin\clang --target=%TARGET% -march=armv8-a
set GXX=%TOOLCHAIN%\bin\clang++ --target=%TARGET% -march=armv8-a
set AR=%TOOLCHAIN%\bin\llvm-ar
set TOOLCHAIN_LIB_DIR=%TOOLCHAIN%\sysroot\usr\lib\aarch64-linux-android



REM ######################################################
REM Directory with native_app_glue.h/.cpp
REM ######################################################

set NATIVE_APP_GLUE_DIR=%NDK%\sources\android\native_app_glue

if not exist %NATIVE_APP_GLUE_DIR% (
	echo Could not find %NATIVE_APP_GLUE_DIR% directory. Please install.
	exit /b 1
)



REM ######################################################
REM Build tools
REM ######################################################

REM Build tools must be at least as new as ANDROID_PLATFORM_DIR, otherwise
REM aapt/aapt2 cannot parse the resource table inside android.jar.
set BUILD_TOOLS=%ANDROID_HOME%\build-tools\36.0.0

if not exist %BUILD_TOOLS% (
	echo Could not find %BUILD_TOOLS% directory. Please install.
	exit /b 1
)

set D8=%BUILD_TOOLS%\d8
set AAPT=%BUILD_TOOLS%\aapt
set AAPT2=%BUILD_TOOLS%\aapt2
set ZIPALIGN=%BUILD_TOOLS%\zipalign
set APKSIGNER=%BUILD_TOOLS%\apksigner



REM ######################################################
REM Output
REM ######################################################

set OUT_LIB_DIR=lib\arm64-v8a
REM set OUT_LIB_DIR=lib\armeabi-v7a
REM set OUT_LIB_DIR=lib\x86
REM set OUT_LIB_DIR=lib\x86_64

REM Remove backslashes for the AAPT command
set AAPT_LIB_DIR=%OUT_LIB_DIR:\=/%



REM ######################################################
REM Application
REM ######################################################

REM Application ID used to install/identify the app on the device.
REM (The Java package of MainActivity is declared in its own source
REM file and in AndroidManifest.xml; it doesn't need to match this.)
set PACKAGE_NAME=com.ilu.game
set DEVICE_DATA_DIR=/sdcard/Android/data/%PACKAGE_NAME%/files



REM ######################################################
REM Platform tools
REM ######################################################

set PLATFORM_TOOLS=%ANDROID_HOME%\platform-tools

if not exist %PLATFORM_TOOLS% (
	echo Could not find %PLATFORM_TOOLS% directory. Please install.
	exit /b 1
)

set ADB=%PLATFORM_TOOLS%\adb



exit /b 0

