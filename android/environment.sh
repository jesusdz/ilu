#!/bin/sh

##########################################################
# Environment variables
##########################################################

JAVA_HOME=${JAVA_HOME:-/path/to/java/home}
ANDROID_HOME=${ANDROID_HOME:-${ANDROID_SDK_ROOT:-/path/to/android/home}}

if [ ! -d "${JAVA_HOME}" ]; then
	echo "Configure JAVA_HOME in environment.sh."
	exit -1
fi

if [ ! -d "${ANDROID_HOME}" ]; then
	echo "Configure ANDROID_HOME in environment.sh."
	exit -1
fi

# Application ID used to install/identify the app on the device.
# (The Java package of MainActivity is declared in its own source
# file and in AndroidManifest.xml; it doesn't need to match this.)
PACKAGE_NAME=com.ilu.game
DEVICE_DATA_DIR=/sdcard/Android/data/${PACKAGE_NAME}/files

# Directories
BUILD_TOOLS=${ANDROID_HOME}/build-tools/33.0.2
PLATFORM_TOOLS=${ANDROID_HOME}/platform-tools
ANDROID_PLATFORM_DIR=${ANDROID_HOME}/platforms/android-33
HOST=linux-x86_64
TARGET=aarch64-linux-android33
NDK=${ANDROID_HOME}/ndk/25.2.9519653
TOOLCHAIN=${NDK}/toolchains/llvm/prebuilt/${HOST}
TOOLCHAIN_LIB_DIR=${TOOLCHAIN}/sysroot/usr/lib/aarch64-linux-android
NATIVE_APP_GLUE_DIR=${NDK}/sources/android/native_app_glue
OUT_LIB_DIR=lib/arm64-v8a

# Tools
GCC="${TOOLCHAIN}/bin/clang -target ${TARGET} -march=armv8-a --sysroot=${TOOLCHAIN}/sysroot"
GXX="${TOOLCHAIN}/bin/clang++ -target ${TARGET} -march=armv8-a --sysroot=${TOOLCHAIN}/sysroot"
AR="${TOOLCHAIN}/bin/llvm-ar"
ADB=${PLATFORM_TOOLS}/adb
AAPT=${BUILD_TOOLS}/aapt
JAVAC=${JAVA_HOME}/bin/javac
D8=${BUILD_TOOLS}/d8
KEYTOOL=${JAVA_HOME}/bin/keytool
APKSIGNER=${BUILD_TOOLS}/apksigner
ZIPALIGN=${BUILD_TOOLS}/zipalign
LLDB=${TOOLCHAIN}/bin/lldb.sh
LLDB_SERVER=${TOOLCHAIN}/lib64/clang/14.0.7/lib/linux/aarch64/lldb-server

