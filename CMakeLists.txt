cmake_minimum_required(VERSION 3.28)
project(CPP_PRACTICE 
    VERSION 0.1.0
    LANGUAGES CXX)

#
# Set the C++ standard
#
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

#
# Enable generation of compile_commands.json
#
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

#
# ONNX Runtime (WITH_ONNXRUNTIME=ON のとき). app / lib いずれからも参照するため
# サブディレクトリより先に取得する.
#
include(${CMAKE_SOURCE_DIR}/cmake/onnxruntime.cmake)

#
# Add subdirectories
#
option(BUILD_APP "Build the application target" ON)
if(BUILD_APP)
    include(${CMAKE_SOURCE_DIR}/app/app.cmake)
endif()
include(${CMAKE_SOURCE_DIR}/lib/lib.cmake)
