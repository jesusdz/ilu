.PHONY: default build_and_run build_and_debug main_interpreter ilu dll game unit_test_ilu main_spirv main_atof reflex reflection main_reflect_serialize main_clon cast data clean main_alsa main_gamepad directories

UNAME_S := $(shell uname -s)

ifeq (${UNAME_S},Darwin)
  CXX=clang++
  PLATFORM_LIBS= -framework Cocoa -framework QuartzCore -framework Metal
  SHARED_FLAGS= -dynamiclib
else
  CXX=g++
  PLATFORM_LIBS= -lxcb -lpthread
  SHARED_FLAGS= -fPIC -shared
endif

CXXFLAGS= -g -DDEVELOPMENT_BUILD
BUILD_DIR=build
DATA_DIR=${BUILD_DIR}
DATA_SHADERS_DIR=${DATA_DIR}/shaders

default: build_and_run

build_and_run: ilu
	${BUILD_DIR}/ilu

build_and_debug: ilu
	gf2 ${BUILD_DIR}/ilu

ilu: directories ${BUILD_DIR}/reflex
	${BUILD_DIR}/reflex code/engine.h code/game.cpp > code/reflex.generated.h
	${CXX} ${CXXFLAGS} -o ${BUILD_DIR}/ilu code/platform.cpp -I"vulkan/include" ${PLATFORM_LIBS}
	${CXX} ${CXXFLAGS} ${SHARED_FLAGS} -o ${BUILD_DIR}/engine.so code/engine.cpp -I"vulkan/include"

dll: directories ${BUILD_DIR}/reflex
	${BUILD_DIR}/reflex code/engine.h code/game.cpp > code/reflex.generated.h
	${CXX} ${CXXFLAGS} ${SHARED_FLAGS} -o ${BUILD_DIR}/engine.so code/engine.cpp -I"vulkan/include"

game:
	echo No build configuration to compile the game yet

unit_test_ilu: directories
	${CXX} ${CXXFLAGS} -o ${BUILD_DIR}/unit_test_ilu code/tests/unit_test_ilu.cpp

main_interpreter: directories
	${CXX} ${CXXFLAGS} -o ${BUILD_DIR}/main_interpreter code/misc/main_interpreter.cpp

main_spirv: directories
	${CXX} ${CXXFLAGS} -o ${BUILD_DIR}/main_spirv code/misc/main_spirv.cpp

main_atof: directories
	${CXX} ${CXXFLAGS} -o ${BUILD_DIR}/main_atof code/misc/main_atof.cpp

reflex: directories
	${CXX} ${CXXFLAGS} -o ${BUILD_DIR}/reflex code/reflex/reflex.cpp

${BUILD_DIR}/reflex: | directories
	${CXX} ${CXXFLAGS} -o ${BUILD_DIR}/reflex code/reflex/reflex.cpp

reflection: reflex
	${BUILD_DIR}/reflex assets/assets.h > code/assets.reflex.h

main_reflect_serialize: reflection
	${CXX} ${CXXFLAGS} -o ${BUILD_DIR}/main_reflect_serialize code/misc/main_reflect_serialize.cpp

main_clon: directories
	${CXX} ${CXXFLAGS} -o ${BUILD_DIR}/main_clon code/misc/main_clon.cpp

cast: directories
	${CXX} ${CXXFLAGS} -o ${BUILD_DIR}/cast code/misc/cast.cpp

main_alsa: directories
	${CXX} ${CXXFLAGS} -o ${BUILD_DIR}/main_alsa code/misc/main_alsa.cpp

main_gamepad: directories
	${CXX} ${CXXFLAGS} -o ${BUILD_DIR}/main_gamepad code/misc/main_gamepad.cpp

directories:
	mkdir -p ${BUILD_DIR}
	mkdir -p ${DATA_SHADERS_DIR}

data: ilu
	${BUILD_DIR}/ilu --build-assets

clean:
	rm -rf ${BUILD_DIR}

