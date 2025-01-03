# CMakeLists.txt --- CMake project settings
##############################################################################

# CMake minimum version
cmake_minimum_required(VERSION 3.6)

# project name and languages
project(FFedge CXX RC)

# Architecture option
option(ARCH "Choose architecture (x86, x64 or arm64)" OFF)
if(${ARCH} STREQUAL x64)
    set(ARCH x64)
elseif(${ARCH} STREQUAL arm64)
    set(ARCH ARM64)
elseif(${ARCH} STREQUAL x86)
    set(ARCH x86)
else()
    set(ARCH x86)
endif()
message(STATUS "ARCH = ${ARCH}")

# webview2
if(MSVC)
    message(FATAL_ERROR "Please use the solution file!")
else()
    set(WEBVIEW2_INCLUDE ${CMAKE_CURRENT_SOURCE_DIR}/webview2)
    set(WEBVIEW2_LIBS ${CMAKE_CURRENT_SOURCE_DIR}/${ARCH}/WebView2Loader.dll)
endif()

# JAPAN or not?
option(JAPAN "Enable Japanese support" ON)
if(JAPAN)
    add_definitions(-DJAPAN)
endif()
message(STATUS "JAPAN = ${JAPAN}")

# FFedge.exe
add_executable(FFedge FFedge.cpp FFedge_res.rc)
target_compile_definitions(FFedge PRIVATE UNICODE _UNICODE)
target_include_directories(FFedge PRIVATE ${WEBVIEW2_INCLUDE})
target_link_libraries(FFedge PRIVATE comctl32 shlwapi ${WEBVIEW2_LIBS})

# Don't add default manifest
if(MSVC)
    target_link_options(FFedge PRIVATE /MANIFEST:NO)
endif()

##############################################################################
