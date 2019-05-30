cmake_minimum_required(VERSION 2.8)
project(obs-aruco-tracker)
set(obs-aruco-tracker_DESCRIPTION "Track ArUco markers and output movement commands to re-center the marker.")
set(obs-aruco-tracker_VERSION_MAJOR 0)
set(obs-aruco-tracker_VERSION_MINOR 1)
set(obs-aruco-tracker_VERSION_PATCH 0)
set(CMAKE_BUILD_TYPE Release)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake/modules/")

find_package(LibObs)
if(${LibObs_FOUND})
    message(STATUS "LibObs FOUND")
else()
    message(FATAL_ERROR "LibObs NOT FOUND")
endif()

if(COMMAND cmake_policy)
	cmake_policy(SET CMP0003 OLD)
	cmake_policy(SET CMP0015 OLD)
endif(COMMAND cmake_policy)

set (CMAKE_CXX_STANDARD 11)
find_package(OpenCV REQUIRED)

math(EXPR BITS "8*${CMAKE_SIZEOF_VOID_P}")
set(PLUGIN_DIRECTORY "${CMAKE_BINARY_DIR}/build/obs-aruco-tracker")
set(PLUGIN_BIN_DIRECTORY "${PLUGIN_DIRECTORY}/bin/${BITS}bit")

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PLUGIN_BIN_DIRECTORY})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PLUGIN_BIN_DIRECTORY})

include_directories(src ${LIBOBS_INCLUDE_DIRS} ${OPENCV_INCLUDE_DIRS})

set(SOURCE_FILES src/obs-tracker.c src/serial.c src/tracker-filter.cpp)

add_library(obs-aruco-tracker MODULE ${SOURCE_FILES})

SET_TARGET_PROPERTIES(obs-aruco-tracker PROPERTIES PREFIX "")
target_link_libraries(obs-aruco-tracker ${LIBOBS_LIBRARIES} ${OpenCV_LIBRARIES})

# install
if(${SYSTEM_INSTALL})
    install(TARGETS obs-aruco-tracker DESTINATION ${LIBOBS_PLUGIN_DESTINATION})
else()
    install(DIRECTORY ${PLUGIN_DIRECTORY} DESTINATION $ENV{HOME}/.config/obs-studio/plugins USE_SOURCE_PERMISSIONS)
endif()
