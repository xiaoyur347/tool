cmake_minimum_required(VERSION 2.8)

project(ffmpeg_pickup_audio)
set(SRC_LIST )

set(CMAKE_BUILD_TYPE "Debug")
SET(CMAKE_SYSTEM_NAME Linux)
SET(CMAKE_USE_RELATIVE_PATHS on)

SET(CFLAGS "-Wall -Wextra -Wno-unused-result -Os -DNDEBUG")
SET(CXXFLAGS "${CFLAGS} -Woverloaded-virtual")
SET(LDFLAGS "-Wl,--no-undefined")

SET(CFLAGS_DEBUG "-O0 -g -ggdb")
SET(CFLAGS_RELEASE "-Os -DNDEBUG")
SET(CMAKE_C_FLAGS_DEBUG "$ENV{CFLAGS} ${CFLAGS} ${CFLAGS_DEBUG}")
SET(CMAKE_C_FLAGS_RELEASE "$ENV{CFLAGS} ${CFLAGS} ${CFLAGS_RELEASE}") 
SET(CMAKE_CXX_FLAGS_DEBUG "$ENV{CXXFLAGS} ${CXXFLAGS} ${CFLAGS_DEBUG}")
SET(CMAKE_CXX_FLAGS_RELEASE "$ENV{CXXFLAGS} ${CXXFLAGS} ${CFLAGS_RELEASE}") 
SET(CMAKE_EXE_LINKER_FLAGS "$ENV{LDFLAGS} ${LDFLAGS} -Wl,--no-as-needed")
SET(CMAKE_MODULE_LINKER_FLAGS "$ENV{LDFLAGS} ${LDFLAGS}")
SET(CMAKE_SHARED_LINKER_FLAGS "$ENV{LDFLAGS} ${LDFLAGS}")

################################################################################
AUX_SOURCE_DIRECTORY(./src SRC_LIST)
AUX_SOURCE_DIRECTORY(./test SRC_LIST)

include_directories(${FFMPEG_ROOT}/include)
link_directories(${FFMPEG_ROOT}/lib)

add_executable(${PROJECT_NAME} ${SRC_LIST})
target_link_libraries(${PROJECT_NAME}
	avformat
	avcodec
	avutil
	swresample
)
