# File generated at : 17:47:33, Sat 24 Aug 
# Converted Project : gamevoice.vcxproj
# /!\ NOTE: this CMakeLists doesn't generate a final plugin release
# /!\ It is used to CI the build and validate it ; see .travis.yml
cmake_minimum_required(VERSION 3.0.0 FATAL_ERROR)

##################### Variables ############################
# Change if you want modify path or other values           #
############################################################

# Project
get_filename_component(PROJECT_DIR "${CMAKE_CURRENT_SOURCE_DIR}" ABSOLUTE)
set(DEPENDENCIES_DIR ${PROJECT_DIR}/dependencies)
set(PROJECT_NAME gamevoice)


# Outputs
set(OUTPUT_DEBUG cmake/bin/debug)
set(OUTPUT_RELEASE cmake/bin/release)

################# CMake Project ############################
# The main options of project                              #
############################################################

project(${PROJECT_NAME} C)

# Define Release by default.
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "Release")
  message(STATUS "Build type not specified: Use Release by default.")
endif(NOT CMAKE_BUILD_TYPE)

############## Artefacts Output ############################
# Defines outputs , depending BUILD TYPE                   #
############################################################

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
  set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${PROJECT_DIR}/${OUTPUT_DEBUG}")
  set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${PROJECT_DIR}/${OUTPUT_DEBUG}")
  set(CMAKE_EXECUTABLE_OUTPUT_DIRECTORY "${PROJECT_DIR}/${OUTPUT_DEBUG}")
else()
  set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${PROJECT_DIR}/${OUTPUT_RELEASE}")
  set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${PROJECT_DIR}/${OUTPUT_RELEASE}")
  set(CMAKE_EXECUTABLE_OUTPUT_DIRECTORY "${PROJECT_DIR}/${OUTPUT_RELEASE}")
endif()

# Messages
message("${PROJECT_NAME}: MAIN PROJECT: ${CMAKE_PROJECT_NAME}")
message("${PROJECT_NAME}: CURR PROJECT: ${CMAKE_CURRENT_SOURCE_DIR}")
message("${PROJECT_NAME}: CURR BIN DIR: ${CMAKE_CURRENT_BINARY_DIR}")
message("${PROJECT_NAME}: C COMPILER ID: ${CMAKE_C_COMPILER_ID}")

############### Files & Targets ############################
# Files of project and target to build                     #
############################################################

include_directories(
	include
	)

# Source Files
set(SRC_FILES
    src/gamevoice_functions.c
    src/plugin.c
    src/usbHidCommunication.c
)
source_group("Sources" FILES ${SRC_FILES})

# Header Files
set(HEADERS_FILES
    src/stdafx.h
    src/ts3_helpers.h
    src/usbHidCommunication.h
    src/plugin.h
    src/gamevoice_functions.h
)
source_group("Headers" FILES ${HEADERS_FILES})

# Add library to build.
add_library(${PROJECT_NAME} SHARED
   ${SRC_FILES} ${HEADERS_FILES}
)

######################### Flags ############################
# Defines Flags for Windows and Linux                      #
############################################################

if(NOT MSVC)
   set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
   if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
       set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++")
   endif()
endif(NOT MSVC)

# Preprocessor definitions
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_definitions(${PROJECT_NAME} PRIVATE 
   -D_DEBUG 
   -D_WINDOWS 
   -D_USRDLL 
   -DTEST_PLUGIN_EXPORTS 
   -DWINDOWS 
    )
    if(MSVC)
		# Visual Studio C++
        target_compile_options(${PROJECT_NAME} PRIVATE  /W3 /Od /Zi /EHsc /GS /MTd /FC)
	elseif ("${CMAKE_C_COMPILER_ID}" STREQUAL "GNU")
		# gcc
		target_compile_options(${PROJECT_NAME} PRIVATE  -g )
    endif()
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Release")
    target_compile_definitions(${PROJECT_NAME} PRIVATE 
   -DWIN32 
   -DNDEBUG 
   -D_WINDOWS 
   -D_USRDLL 
   -DTEST_PLUGIN_EXPORTS 
   -DWINDOWS 
    )
    if(MSVC)
		# Visual Studio C++
        target_compile_options(${PROJECT_NAME} PRIVATE  /W3 /GL /EHsc /GS /O2 /MT /FC /WX-)
	elseif ("${CMAKE_C_COMPILER_ID}" STREQUAL "GNU")
		# gcc
		 target_compile_options(${PROJECT_NAME} PRIVATE -O2 )
    endif()
endif()

########### Link & Dependencies ############################
# Add project dependencies and Link to project             #
############################################################

target_link_libraries(${PROJECT_NAME} setupapi.lib hid.lib )