.PHONY: default build_and_run build_and_debug main_interpreter engine dll game main_spirv reflex main_reflect_serialize main_clon cast data clean main_alsa main_gamepad directories

CXX=g++
CXXFLAGS= -g -DDEVELOPMENT_BUILD
BUILD_DIR=build
DATA_DIR=${BUILD_DIR}
DATA_SHADERS_DIR=${DATA_DIR}/shaders
DXC=./dxc/linux/bin/dxc

# Requires Vulkan 1.3
#DXC_FLAGS=-Od -fspv-extension=SPV_KHR_non_semantic_info -fspv-debug=vulkan-with-source
DXC_FLAGS=-O3

default: build_and_run

build_and_run: engine
	i3 workspace run
	./build/ilu; i3 workspace 2

build_and_debug: engine
	i3 workspace debug
	gf2 ./build/ilu; i3 workspace 2

engine: directories
	${CXX} ${CXXFLAGS} -o ${BUILD_DIR}/ilu code/platform.cpp -I"vulkan/include" -lxcb -lpthread
	${CXX} ${CXXFLAGS} -fPIC -shared -o ${BUILD_DIR}/engine.so code/engine.cpp -I"vulkan/include"

dll: directories
	touch code/reflex.generated.h
	${CXX} ${CXXFLAGS} -fPIC -shared -o ${BUILD_DIR}/engine.so code/engine.cpp -I"vulkan/include"

game:
	echo No build configuration to compile the game yet

unit_test_ilu: directories
	${CXX} ${CXXFLAGS} -o ${BUILD_DIR}/unit_test_ilu code/tests/unit_test_ilu.cpp

main_interpreter: directories
	${CXX} ${CXXFLAGS} -o ${BUILD_DIR}/main_interpreter code/misc/main_interpreter.cpp

main_spirv: directories
	${CXX} ${CXXFLAGS} -o ${BUILD_DIR}/main_spirv code/misc/main_spirv.cpp

reflex: directories
	${CXX} ${CXXFLAGS} -o ${BUILD_DIR}/reflex code/misc/reflex.cpp

main_reflect_serialize: reflex
	./build/reflex assets/assets.h > code/assets.reflex.h
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
	mkdir -p build
	mkdir -p build/shaders

data: engine
	./build/ilu --build-data

clean:
	rm -rf build

