# ------------------------------------------------------------------------------
# Copyright 2024 Munich Quantum Software Stack Project
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <https://www.gnu.org/licenses/>.
# ------------------------------------------------------------------------------

# set required cmake version
cmake_minimum_required(VERSION 3.19...3.30)

option(CXX_DEVICE "Build a C++ device" ON)

set(QDMI_PREFIX "MAESTRO")

project(
  MaestroDevice
  LANGUAGES C
  VERSION 0.0.1
  DESCRIPTION "Maestro QDMI Device Implementation")

if(CXX_DEVICE)
  enable_language(CXX)
endif()

# make scripts available to cmake
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

# check if this is the master project or used via add_subdirectory
if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
  set(MAESTRO_DEVICE_MASTER_PROJECT ON)
else()
  set(MAESTRO_DEVICE_MASTER_PROJECT OFF)
endif()

option(BUILD_MAESTRO_DEVICE_TESTS "Build tests for MaestroDevice"
       ${MAESTRO_DEVICE_MASTER_PROJECT})

include(cmake/ExternalDependencies.cmake)
include(cmake/MaestroDependencies.cmake)

# add main library code
add_subdirectory(src)

# enable testing
if(BUILD_MAESTRO_DEVICE_TESTS)
  enable_testing()
  include(GoogleTest)
  add_subdirectory(test)
endif()
