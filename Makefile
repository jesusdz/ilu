.PHONY: default build_and_run build_and_debug ilu dll reflex data clean directories unit_test_ilu main_interpreter main_spirv main_atof main_cast main_alsa main_gamepad

UNAME_S := $(shell uname -s)

ifeq (${UNAME_S},Darwin)
  CXX=clang++
  PLATFORM_LANG= -x objective-c++ # platform.cpp includes platform_macos.mm
  PLATFORM_LIBS= -framework Cocoa -framework QuartzCore -framework Metal -framework GameController -framework AudioToolbox -framework CoreServices
  SHARED_FLAGS= -dynamiclib
  # dlopen doesn't search Homebrew's folders by default to find Vulkan validation layer
  RUN_ENV= DYLD_FALLBACK_LIBRARY_PATH=/opt/homebrew/lib
else
  CXX=g++
  PLATFORM_LANG=
  PLATFORM_LIBS= -lxcb -lpthread
  SHARED_FLAGS= -fPIC -shared
  RUN_ENV=
endif

CXXFLAGS= -g -DDEVELOPMENT_BUILD
BUILD_DIR=build
DATA_DIR=${BUILD_DIR}
DATA_SHADERS_DIR=${DATA_DIR}/shaders

default: build_and_run

build_and_run: ilu
	${RUN_ENV} ${BUILD_DIR}/ilu

build_and_debug: ilu
	gf2 ${BUILD_DIR}/ilu

VULKAN_INCLUDE= -I"vulkan/include"

# Builds code/misc/<target>.cpp into ${BUILD_DIR}/<target>
BUILD_MISC_TOOL= ${CXX} ${CXXFLAGS} -o ${BUILD_DIR}/$@ code/misc/$@.cpp

ilu: dll
	${CXX} ${CXXFLAGS} -o ${BUILD_DIR}/ilu ${PLATFORM_LANG} code/platform.cpp ${VULKAN_INCLUDE} ${PLATFORM_LIBS}

dll: directories ${BUILD_DIR}/reflex
	${BUILD_DIR}/reflex code/engine.h code/game.cpp > code/reflex.generated.h
	${CXX} ${CXXFLAGS} ${SHARED_FLAGS} -o ${BUILD_DIR}/engine.so code/engine.cpp ${VULKAN_INCLUDE}

# `reflex` always rebuilds the tool, ${BUILD_DIR}/reflex only builds it when missing
reflex: directories
	${CXX} ${CXXFLAGS} -o ${BUILD_DIR}/reflex code/reflex/reflex.cpp

${BUILD_DIR}/reflex: | directories
	${CXX} ${CXXFLAGS} -o ${BUILD_DIR}/reflex code/reflex/reflex.cpp

unit_test_ilu: directories
	${CXX} ${CXXFLAGS} -o ${BUILD_DIR}/unit_test_ilu code/tests/unit_test_ilu.cpp

main_interpreter main_spirv main_atof main_cast main_alsa main_gamepad: directories
	${BUILD_MISC_TOOL}

directories:
	mkdir -p ${BUILD_DIR}
	mkdir -p ${DATA_SHADERS_DIR}

data: ilu
	${BUILD_DIR}/ilu --build-assets

clean:
	rm -rf ${BUILD_DIR}

