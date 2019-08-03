#   Copyright 2019 Weston Schmidt
#
#   Licensed under the Apache License, Version 2.0 (the "License");
#   you may not use this file except in compliance with the License.
#   You may obtain a copy of the License at
#
#       http://www.apache.org/licenses/LICENSE-2.0
#
#   Unless required by applicable law or agreed to in writing, software
#   distributed under the License is distributed on an "AS IS" BASIS,
#   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#   See the License for the specific language governing permissions and
#   limitations under the License.

cmake_minimum_required(VERSION 2.8.7)

project(metriks)
include(ExternalProject)
include(CTest)

# Compile options/flags
#-------------------------------------------------------------------------------
add_definitions(-std=c99)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c99 -g -Werror -Wall")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c99 -g -Werror -Wall")

if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
set(CMAKE_MACOSX_RPATH 1)
set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -undefined dynamic_lookup")
endif()

set(INSTALL_DIR ${CMAKE_CURRENT_BINARY_DIR}/_install)
set(PREFIX_DIR ${CMAKE_CURRENT_BINARY_DIR}/_prefix)
set(INCLUDE_DIR ${INSTALL_DIR}/include)
set(LIBRARY_DIR ${INSTALL_DIR}/lib${LIB_SUFFIX})
set(LIBRARY_DIR64 ${INSTALL_DIR}/lib64)
set(COMMON_LIBRARY_DIR ${INSTALL_DIR}/lib/${CMAKE_LIBRARY_ARCHITECTURE})

# Includes
#--------------------------------------------------------------------------------
include_directories(${INCLUDE_DIR})

include_directories(${CURL_INCLUDE_DIRS})
include_directories (SYSTEM /usr/include)


# pthread external dependency
#-------------------------------------------------------------------------------
find_package (Threads)

enable_testing()

link_directories ( ${LIBRARY_DIR} ${COMMON_LIBRARY_DIR} ${LIBRARY_DIR64} )
add_subdirectory(src)
add_subdirectory(tests)
