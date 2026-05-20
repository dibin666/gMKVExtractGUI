cmake_minimum_required(VERSION 3.21)

project(gMKVExtractGUI_Cpp
    VERSION 2.13.0
    DESCRIPTION "Native C++/Qt rewrite of gMKVExtractGUI"
    LANGUAGES CXX
)

option(GMKV_BUILD_CPP_TESTS "Build C++ rewrite tests" ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_AUTOMOC ON)

include(GNUInstallDirs)

find_package(Qt6 6.2 REQUIRED COMPONENTS Core Widgets)

if(GMKV_BUILD_CPP_TESTS)
    enable_testing()
endif()

add_subdirectory(src/gMKVToolNix.Cpp)
add_subdirectory(src/gMKVExtractGUI.Cpp)

if(GMKV_BUILD_CPP_TESTS)
    add_subdirectory(tests/gMKVToolNix.Cpp.Tests)
endif()
