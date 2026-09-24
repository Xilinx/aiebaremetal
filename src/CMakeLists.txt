# Copyright (C) 2025 Advanced Micro Devices, Inc.. All rights reserved.
# SPDX-License-Identifier: MIT

cmake_minimum_required(VERSION 3.15)  

project(aiebaremetal LANGUAGES C CXX ASM)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)  
set(CMAKE_CXX_STANDARD_REQUIRED ON)  

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS TRUE)

file(GLOB BAREMETAL_SRCS_CC  
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cc"  
)  
file(GLOB BAREMETAL_SRCS_CPP  
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp" 
    "${CMAKE_CURRENT_SOURCE_DIR}/common_layer/*.cpp" 
    "${CMAKE_CURRENT_SOURCE_DIR}/common_layer/aegothers/*.cpp"
) 

set(SOURCES ${BAREMETAL_SRCS_CC} ${BAREMETAL_SRCS_CPP} ) 
 

file(GLOB  HEADERS  "${CMAKE_CURRENT_SOURCE_DIR}/include/aeg_baremetal_api.h"  )
find_package(common)


file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/include/aeg_baremetal_api.h DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/)

include_directories(  
    "${CMAKE_CURRENT_SOURCE_DIR}"  
    "${CMAKE_CURRENT_SOURCE_DIR}/include"  
    "${CMAKE_CURRENT_SOURCE_DIR}/include/common_layer"  
    "${CMAKE_CURRENT_SOURCE_DIR}/include/common_layer/aegothers"   
)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/../build")  
  
# Create the "release" library (libaiebaremetal.a in Makefile) 

add_library(aiebaremetal STATIC ${SOURCES})  
set_target_properties(aiebaremetal PROPERTIES OUTPUT_NAME "aiebaremetal")  

target_compile_options(aiebaremetal PRIVATE -DXAIE_DEBUG  
    -g  
    -Wall  
    -Wextra  
    -DNO_JSON  
    -Dversal  
    -DARMA72_EL3  
    -fno-tree-loop-distribute-patterns  )

if(("${CMAKE_SYSTEM_PROCESSOR}" STREQUAL "psv_cortexr5")
    OR ("${CMAKE_SYSTEM_PROCESSOR}" STREQUAL "cortexr52"))
   
    target_compile_options(aiebaremetal PRIVATE -DARMR5)

endif()

install(TARGETS aiebaremetal LIBRARY DESTINATION ${CMAKE_LIBRARY_PATH} ARCHIVE DESTINATION ${CMAKE_LIBRARY_PATH})
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/include/aeg_baremetal_api.h DESTINATION ${CMAKE_INCLUDE_PATH})