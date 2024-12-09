﻿#
# GDK-targets.cmake : Defines library imports for the Microsoft GDK shared libraries
#
# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.

if(_GDK_TARGETS_)
  return()
endif()

if(CMAKE_SIZEOF_VOID_P EQUAL 4)
    message(FATAL_ERROR "ERROR: Microsoft GDK only supports 64-bit")
endif()

if(NOT XdkEditionTarget)
    message(FATAL_ERROR "ERROR: XdkEditionTarget must be set")
endif()

#--- Locate Microsoft GDK
if(BUILD_USING_BWOI)
    if(DEFINED ENV{ExtractedFolder})
        cmake_path(SET ExtractedFolder "$ENV{ExtractedFolder}")
    else()
        set(ExtractedFolder "d:/xtrctd.sdks/BWOIExample/")
    endif()

    if(NOT EXISTS ${ExtractedFolder})
        message(FATAL_ERROR "ERROR: BWOI requires a valid ExtractedFolder (${ExtractedFolder})")
    endif()

    set(Console_SdkRoot "${ExtractedFolder}/Microsoft GDK")
else()
    GET_FILENAME_COMPONENT(Console_SdkRoot "[HKEY_LOCAL_MACHINE\\SOFTWARE\\WOW6432Node\\Microsoft\\GDK;GRDKInstallPath]" ABSOLUTE CACHE)
endif()

if(NOT EXISTS "${Console_SdkRoot}/${XdkEditionTarget}")
    message(FATAL_ERROR "ERROR: Cannot locate Microsoft Game Development Kit (GDK) - ${XdkEditionTarget}")
endif()

#--- GameRuntime Library (for Xbox these are included in the Console_Libs variable)
if(NOT _GDK_XBOX_)
    add_library(Xbox::GameRuntime STATIC IMPORTED)
    set_target_properties(Xbox::GameRuntime PROPERTIES
        IMPORTED_LOCATION "${Console_SdkRoot}/${XdkEditionTarget}/GRDK/gameKit/Lib/amd64/xgameruntime.lib"
        MAP_IMPORTED_CONFIG_MINSIZEREL ""
        MAP_IMPORTED_CONFIG_RELWITHDEBINFO ""
        INTERFACE_INCLUDE_DIRECTORIES "${Console_SdkRoot}/${XdkEditionTarget}/GRDK/gameKit/Include"
        INTERFACE_COMPILE_FEATURES "cxx_std_11"
        IMPORTED_LINK_INTERFACE_LANGUAGES "CXX")

    if(XdkEditionTarget GREATER_EQUAL 220600)
        add_library(Xbox::GameInput STATIC IMPORTED)
        set_target_properties(Xbox::GameInput PROPERTIES
        IMPORTED_LOCATION "${Console_SdkRoot}/${XdkEditionTarget}/GRDK/gameKit/Lib/amd64/gameinput.lib"
        MAP_IMPORTED_CONFIG_MINSIZEREL ""
        MAP_IMPORTED_CONFIG_RELWITHDEBINFO ""
        INTERFACE_INCLUDE_DIRECTORIES "${Console_SdkRoot}/${XdkEditionTarget}/GRDK/gameKit/Include"
        IMPORTED_LINK_INTERFACE_LANGUAGES "CXX")
    endif()
endif()

#--- Extension Libraries
set(Console_GRDKExtLibRoot "${Console_SdkRoot}/${XdkEditionTarget}/GRDK/ExtensionLibraries")
set(ExtensionPlatformToolset 142)

set(_GDK_TARGETS_ ON)
