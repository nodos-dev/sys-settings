# Copyright MediaZ Teknoloji A.S. All Rights Reserved.
cmake_minimum_required(VERSION 3.24.2)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set(CMAKE_CXX_STANDARD 20)

if (NOT WITH_NODOS_WORKSPACE)
    message(FATAL_ERROR "This repo currently does not support builds without Nodos workspace. "
    "Place this repo under nodos-workspace/Module folder and run cmake -S ./Toolchain/CMake -B Build from workspace root.")
endif()

set(USE_NOS_PLUGIN_SDK_VERSION "41.0")
add_compile_definitions(NOS_DISABLE_DEPRECATED)
nos_find_plugin_sdk(${USE_NOS_PLUGIN_SDK_VERSION} NOS_PLUGIN_SDK_TARGET NOS_SDK_DIR)
if (NOT DEFINED NOS_SDK_DIR)
    message(FATAL_ERROR "Nodos SDK with version ${USE_NOS_SDK_VERSION} not found, please either install it or choose different version")
endif()

nos_generate_flatbuffers("${CMAKE_CURRENT_SOURCE_DIR}/Types" "${CMAKE_CURRENT_SOURCE_DIR}/Include/nosSettingsSubsystem" "cpp" "${NOS_SDK_DIR}/Types" nosSettingsSubsystem_generated)
list(APPEND MODULE_DEPENDENCIES_TARGETS ${NOS_PLUGIN_SDK_TARGET} nosSettingsSubsystem_generated)

list(APPEND INCLUDE_FOLDERS ${CMAKE_CURRENT_SOURCE_DIR} "${CMAKE_CURRENT_SOURCE_DIR}/Include")
nos_add_plugin("nosSettingsSubsystem" "${MODULE_DEPENDENCIES_TARGETS}" "${INCLUDE_FOLDERS}")

#Helpers need C++20
set_target_properties("nosSettingsSubsystem" PROPERTIES CXX_STANDARD 20)

nos_group_targets("nosSettingsSubsystem" "NOS Subsystems")