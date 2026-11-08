#!/bin/sh

source ./environment.sh


##########################################################
# Target to build
##########################################################

target=$1
if [[ ${target} = "" ]]; then
	target="build"
fi


##########################################################
# Compile the C native library
##########################################################

if [[ ${target} = "build" ]]; then

	rm -rf obj lib
	mkdir obj lib
	mkdir lib/arm64-v8a

	echo "Pushd..."
	pushd obj
	echo ""

	CODE_DIR="../../code"
	INCLUDES="-I../../ -I../../vulkan/include"
	GCC_FLAGS="-fPIC"
	GCC_FLAGS="${GCC_FLAGS} -g -O0" # With debug info
	GXX_FLAGS="${GCC_FLAGS} -std=gnu++20"
	LDD_FLAGS="-Wl,--build-id=sha1 -Wl,-z,max-page-size=16384"

	echo "android_native_app_glue.o:"
	echo ${GCC} ${GCC_FLAGS} -c ${NATIVE_APP_GLUE_DIR}/android_native_app_glue.c -o android_native_app_glue.o
	${GCC} ${GCC_FLAGS} -c ${NATIVE_APP_GLUE_DIR}/android_native_app_glue.c -o android_native_app_glue.o
	echo ""

	echo "android_native_app_glue.a:"
	echo ${AR} rcs libandroid_native_app_glue.a android_native_app_glue.o
	${AR} rcs libandroid_native_app_glue.a android_native_app_glue.o
	echo ""

	echo "main.o:"
	echo ${GXX} -I${NATIVE_APP_GLUE_DIR} ${INCLUDES} ${GXX_FLAGS} -c ${CODE_DIR}/platform.cpp -o main.o
	${GXX} -I${NATIVE_APP_GLUE_DIR} ${INCLUDES} ${GXX_FLAGS} -c ${CODE_DIR}/platform.cpp -o main.o
	echo ""

	echo "libgame.so:"
	echo ${GXX} ${LDD_FLAGS} main.o -L. -landroid_native_app_glue -u ANativeActivity_onCreate -landroid -llog -laaudio -shared -o ../${OUT_LIB_DIR}/libgame.so
	${GXX} ${LDD_FLAGS} main.o -L. -landroid_native_app_glue -u ANativeActivity_onCreate -landroid -llog -laaudio -shared -o ../${OUT_LIB_DIR}/libgame.so
	echo ""

	echo "libengine.so:"
	echo ${GXX} -I${NATIVE_APP_GLUE_DIR} ${INCLUDES} ${GXX_FLAGS} ${LDD_FLAGS} ${CODE_DIR}/engine.cpp -llog -shared -o ../${OUT_LIB_DIR}/libengine.so
	${GXX} -I${NATIVE_APP_GLUE_DIR} ${INCLUDES} ${GXX_FLAGS} ${LDD_FLAGS} ${CODE_DIR}/engine.cpp -llog -shared -o ../${OUT_LIB_DIR}/libengine.so
	echo ""

	echo "Popd..."
	popd
	echo ""

fi


##########################################################
# Make the APK (Android PacKage)
##########################################################

if [[ ${target} = "apk" ]]; then
	echo Building the APK

    # ----------------------------------------------------
    # Start with a fresh bin directory
	rm -rf bin
	mkdir bin

	# ----------------------------------------------------
	# Generate the R.java file into the src directory

	${AAPT} package -v -f -m -S res -J src -M AndroidManifest.xml -I ${ANDROID_PLATFORM_DIR}/android.jar

	# ----------------------------------------------------
	# Compile java code from 'src' to 'obj' directory

	${JAVAC} -source 1.7 -target 1.7 -d obj -classpath ${ANDROID_PLATFORM_DIR}/android.jar -sourcepath src src/${APPLICATION_REL_PATH}/R.java
	${JAVAC} -source 1.7 -target 1.7 -d obj -classpath ${ANDROID_PLATFORM_DIR}/android.jar -sourcepath src src/${APPLICATION_REL_PATH}/MainActivity.java

	# ----------------------------------------------------
	# Compile java bytecode to dex bytecode. Reference
	# https://developer.android.com/studio/command-line/d8

	${D8} obj/${APPLICATION_REL_PATH}/* --classpath ${ANDROID_PLATFORM_DIR}/android.jar --output bin/

	# ----------------------------------------------------
	# Generate the first unsigned version of the APK

	${AAPT} package -v -f -M AndroidManifest.xml -S res -I ${ANDROID_PLATFORM_DIR}/android.jar -F bin/NativeActivity.unaligned.apk bin/

	# ------------------------------------------------------
	# Add the native activity and other libs to the APK

	${AAPT} add bin/NativeActivity.unaligned.apk "${OUT_LIB_DIR}/libgame.so"
	${AAPT} add bin/NativeActivity.unaligned.apk "${OUT_LIB_DIR}/libengine.so"
	cp "${TOOLCHAIN_LIB_DIR}/libc++_shared.so" ${OUT_LIB_DIR}
	${AAPT} add bin/NativeActivity.unaligned.apk "${OUT_LIB_DIR}/libc++_shared.so"
	#cp "../vulkan/lib/arm64_v8a/libVkLayer_khronos_validation.so" ${OUT_LIB_DIR}
	#${AAPT} add bin/NativeActivity.unaligned.apk "${OUT_LIB_DIR}/libVkLayer_khronos_validation.so"


	# ------------------------------------------------------
	# Align the resources of the final APK to 4 bytes

	${ZIPALIGN} -v -f 4 bin/NativeActivity.unaligned.apk bin/NativeActivity.apk #|| goto ERROR


	# ------------------------------------------------------
	# Create a key and sign the APK with it

	if [ ! -f "Tools.keystore" ]; then
		${KEYTOOL} -genkeypair -validity 1000 -dname "CN=tools,O=jesusdz,C=ES" -keystore Tools.keystore -storepass pass4tools -keypass pass4tools -alias ToolsKey -keyalg RSA
	fi
	${APKSIGNER} sign --ks Tools.keystore --ks-key-alias ToolsKey --ks-pass pass:"pass4tools" bin/NativeActivity.apk

fi


##########################################################
# Install APK into the device
##########################################################

if [[ ${target} = "install" ]]; then
	echo Installing application
	# -k argument would keep the data and cache directories
	${ADB} uninstall com.tools.game
	${ADB} install -r bin/NativeActivity.apk
	${ADB} shell mkdir -p /sdcard/Android/data/com.tools.game/files/
	${ADB} shell chmod o+rwx /sdcard/Android/data/com.tools.game/files
	${ADB} push ../build/shaders.dat /sdcard/Android/data/com.tools.game/files/
	${ADB} shell chmod o+r /sdcard/Android/data/com.tools.game/files/shaders.dat
	${ADB} push ../build/assets.dat /sdcard/Android/data/com.tools.game/files/
	${ADB} shell chmod o+r /sdcard/Android/data/com.tools.game/files/assets.dat
	#${ADB} push ../build/shaders/ /sdcard/Android/data/com.tools.game/files/
	#${ADB} shell chmod o+rwx /sdcard/Android/data/com.tools.game/files/shaders
fi


##########################################################
# Clean APK and compilation data
##########################################################

if [[ ${target} = "cleanall" ]]; then
	rm -rf bin obj lib 2>/dev/null
	rm -rf bin intermediates 2>/dev/null
	rm -rf src/com/tools/game/R.java
	rm -rf Tools.keystore
fi

if [[ ${target} = "clean" ]]; then
	rm -rf bin obj lib 2>/dev/null
fi

