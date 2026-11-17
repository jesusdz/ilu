#!/bin/sh

source ./environment.sh

# Start application in debug mode
${ADB} shell am start -n \
	"${PACKAGE_NAME}/${PACKAGE_NAME}.MainActivity" \
	-a android.intent.action.MAIN \
	-c android.intent.category.LAUNCHER \
	-D
sleep 0.5
# NOTE: If we start the application in non debug mode and we attach later without using jdb, then it attaches and finds the symbols correctly

# Get PID
PID=$(${ADB} shell pidof ${PACKAGE_NAME})
if [ ! ${PID} ]; then
	echo "Could not find process for ${PACKAGE_NAME}"
	exit -1
fi

#SYM_DIR="./lib/arm64-v8a"
SYM_DIR="/home/jesus/Develop/ilu/android/lib/arm64-v8a"
LIB_GAME="./lib/arm64-v8a/libgame.so"
LIB_CXX_SHARED="./lib/arm64-v8a/libc++_shared.so"
SRC_DIR="./jni"

## If the library is already into memory
#${LLDB} \
#--one-line "platform select remote-android" \
#--one-line "platform connect unix-abstract-connect:///${PACKAGE_NAME}/debug.socket" \
#--one-line "add-dsym ${LIB_GAME}" \
#--one-line "add-dsym ${LIB_CXX_SHARED}" \
#--one-line "process attach -p ${PID}" \
#--one-line "settings set target.source-map ${SRC_DIR} ${SRC_DIR}" \
#--one-line "image lookup -vn android_main"

# If the library is not yet loaded in memory
#${LLDB} \
#--one-line "platform select remote-android" \
#--one-line "platform connect unix-abstract-connect:///${PACKAGE_NAME}/debug.socket" \
#--one-line "settings append target.exec-search-paths ${SYM_DIR}" \
#--one-line "process attach -p ${PID}" \
#--one-line "settings set target.source-map ${SRC_DIR} ${SRC_DIR}" \
#--one-line "image lookup -vn android_main"

## If the game is already running
${LLDB} \
--one-line "platform select remote-android" \
--one-line "platform connect unix-abstract-connect:///${PACKAGE_NAME}/debug.socket" \
--one-line "process attach -p ${PID}" \
--one-line "continue"
