cmake_minimum_required(VERSION 3.10)

project(DocBuilderAddIn CXX)

set(CMAKE_CXX_STANDARD 17)
set(TARGET DocBuilderAddIn)

option(CASE_INSENSITIVE "Case insensitive method names" OFF)
option(STATIC_CRT "Static CRT linkage" OFF)
option(OUT_PARAMS "Support output parameters" OFF)

if (WIN32)
    add_definitions(-DUNICODE -DWIN32 -D_USRDLL -D_CRT_SECURE_NO_WARNINGS -D_CRT_SECURE_NO_DEPRECATE -DADDINCPP_EXPORTS)
endif ()
if (UNIX AND NOT APPLE)
    add_definitions(-D__linux__)
endif ()

list(APPEND SOURCES
        src/addin.def
        src/stdafx.h
        src/dllmain.cpp
        src/exports.cpp
        src/Component.cpp
        src/Component.h
        src/DocBuilderAddIn.cpp
        src/DocBuilderAddIn.h)

list(APPEND H_SOURCES
    "src/"
    "include/"
)

if (UNIX AND NOT APPLE)
    list(APPEND H_SOURCES
        "docbuilder_linux/include"
    )
endif()
if(WIN32)
    list(APPEND H_SOURCES
        "docbuilder_win/include"
    )
endif()

list(APPEND SOURCES
    ${H_SOURCES}
)



if (UNIX AND NOT APPLE)
    set(LIBRARY_DIRS
    "${CMAKE_SOURCE_DIR}/docbuilder_linux"
)
endif()
if(WIN32)
    set(LIBRARY_DIRS
        "C:/Windows/System32"
        "${CMAKE_SOURCE_DIR}/docbuilder_win"
        "C:/Windows/System32/downlevel"
    )
endif()

link_directories(${LIBRARY_DIRS})

if (ANDROID)
    list(APPEND SOURCES
            src/jnienv.cpp
            src/jnienv.h)
endif ()

if (UNIX AND NOT APPLE)
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,-rpath='${CMAKE_SOURCE_DIR}/docbuilder_linux'")
endif ()

add_library(${TARGET} SHARED ${SOURCES})

target_include_directories(${TARGET} PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
if(WIN32)
    target_precompile_headers(${TARGET} PUBLIC "${CMAKE_SOURCE_DIR}/src/stdafx.h")
endif()

set_target_properties( ${TARGET} PROPERTIES
       POSITION_INDEPENDENT_CODE ON 
       CXX_STANDARD 17
       CXX_STANDARD_REQUIRED ON
   )
target_include_directories(${TARGET} PRIVATE ${H_SOURCES})

if (UNIX AND NOT APPLE)
    find_library(DOCTRENDERER doctrenderer "${CMAKE_SOURCE_DIR}/docbuilder_linux/")
    message(STATUS "DOCTRENDERER_linux: ${DOCTRENDERER}")
    target_link_libraries(${TARGET} PRIVATE ${DOCTRENDERER})
endif()
if(WIN32)
    find_library(DOCTRENDERER doctrenderer "${CMAKE_SOURCE_DIR}/docbuilder_win/")
    message(STATUS "DOCTRENDERER_win: ${DOCTRENDERER}")
    target_link_libraries(${TARGET} PRIVATE ${DOCTRENDERER})
endif()

set_property(TARGET ${TARGET} PROPERTY LINK_DIRECTORIES ${LIBRARY_DIRS})


target_compile_definitions(${TARGET} PRIVATE
        UNICODE
        _UNICODE)

if (CASE_INSENSITIVE)
    target_compile_definitions(${TARGET} PRIVATE CASE_INSENSITIVE)
endif ()

if (OUT_PARAMS)
    target_compile_definitions(${TARGET} PRIVATE OUT_PARAMS)
endif ()

target_include_directories(${TARGET} PRIVATE
        include)

if (WIN32 AND NOT MSVC)
    message(FATAL_ERROR "Must be compiled with MSVC on Windows")
endif ()

if (WIN32)
    if (STATIC_CRT)
        string(REPLACE "/MD" "/MT" CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE}")
        string(REPLACE "/MD" "/MT" CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG}")
    endif ()
    target_compile_definitions(${TARGET} PRIVATE
            _WINDOWS
            _SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING)
    target_compile_options(${TARGET} PRIVATE /utf-8)
endif ()

if (UNIX)
    if (TARGET_ARCH STREQUAL "x86_64")
        set(CMAKE_C_FLAGS "-m64 ${CMAKE_C_FLAGS}")
        set(CMAKE_CXX_FLAGS "-m64 ${CMAKE_CXX_FLAGS}")
    endif ()
endif ()

if (ANDROID)
    if (CMAKE_BUILD_TYPE STREQUAL Release)
        add_custom_command(TARGET ${TARGET} POST_BUILD
                COMMAND ${CMAKE_STRIP} ${CMAKE_SHARED_LIBRARY_PREFIX}${TARGET}${CMAKE_SHARED_LIBRARY_SUFFIX})
    endif ()
endif ()