cmake_minimum_required(VERSION 3.21)

project(gMKVExtractGUI_Cpp
    VERSION 2.13.0
    DESCRIPTION "Native C++/Qt rewrite of gMKVExtractGUI"
    LANGUAGES CXX
)

option(GMKV_BUILD_CPP_TESTS "Build C++ rewrite tests" ON)
set(GMKV_PACKAGE_VERSION "${PROJECT_VERSION}" CACHE STRING "Package version used by CPack and release artifacts")

set(GMKV_APP_ID "io.github.dibin666.gmkvextractgui")
set(GMKV_PACKAGE_NAME "gmkvextractgui")
set(GMKV_DISPLAY_NAME "gMKVExtractGUI")
set(GMKV_PACKAGE_DESCRIPTION
    "gMKVExtractGUI is a native C++/Qt interface for extracting tracks, chapters, tags, attachments, cues, and timecodes from Matroska files with MKVToolNix command-line tools."
)

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

set(CPACK_PACKAGE_NAME "${GMKV_PACKAGE_NAME}")
set(CPACK_PACKAGE_VENDOR "dibin666")
set(CPACK_PACKAGE_CONTACT "dibin666")
set(CPACK_PACKAGE_VERSION "${GMKV_PACKAGE_VERSION}")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "${PROJECT_DESCRIPTION}")
set(CPACK_PACKAGE_DESCRIPTION "${GMKV_PACKAGE_DESCRIPTION}")
set(CPACK_PACKAGE_HOMEPAGE_URL "https://github.com/dibin666/gMKVExtractGUI")
set(CPACK_PACKAGE_INSTALL_DIRECTORY "${GMKV_DISPLAY_NAME}")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
set(CPACK_PACKAGE_CHECKSUM SHA256)

if(UNIX AND NOT APPLE)
    set(CPACK_GENERATOR "DEB;RPM")
    set(CPACK_STRIP_FILES TRUE)

    set(CPACK_DEBIAN_PACKAGE_NAME "${GMKV_PACKAGE_NAME}")
    set(CPACK_DEBIAN_PACKAGE_MAINTAINER "dibin666")
    set(CPACK_DEBIAN_PACKAGE_SECTION "video")
    set(CPACK_DEBIAN_PACKAGE_PRIORITY "optional")
    set(CPACK_DEBIAN_PACKAGE_DESCRIPTION "${GMKV_PACKAGE_DESCRIPTION}")
    set(CPACK_DEBIAN_PACKAGE_DEPENDS "mkvtoolnix")
    set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)
    set(CPACK_DEBIAN_FILE_NAME DEB-DEFAULT)

    set(CPACK_RPM_PACKAGE_NAME "${GMKV_PACKAGE_NAME}")
    set(CPACK_RPM_PACKAGE_LICENSE "Unlicense")
    set(CPACK_RPM_PACKAGE_GROUP "Applications/Multimedia")
    set(CPACK_RPM_PACKAGE_DESCRIPTION "${GMKV_PACKAGE_DESCRIPTION}")
    set(CPACK_RPM_PACKAGE_REQUIRES "mkvtoolnix")
    set(CPACK_RPM_PACKAGE_AUTOREQ ON)
    set(CPACK_RPM_FILE_NAME RPM-DEFAULT)
elseif(WIN32)
    set(CPACK_GENERATOR "ZIP")
else()
    set(CPACK_GENERATOR "TGZ")
endif()

include(CPack)
