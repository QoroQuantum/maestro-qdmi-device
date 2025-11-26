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

include(FetchContent)
include(CMakeDependentOption)
set(FETCH_PACKAGES "")

if(TARGET qdmi::qdmi)
  message(STATUS "[qdmi][template] QDMI is already available.")
else()
  message(STATUS "[qdmi][template] QDMI will be included via FetchContent")
  # cmake-format: off
  set(QDMI_VERSION 1.0.0
          CACHE STRING "QDMI version")
  set(QDMI_REV "develop"
          CACHE STRING "QDMI identifier (tag, branch or commit hash)")
  set(QDMI_REPO_OWNER "Munich-Quantum-Software-Stack"
          CACHE STRING "QDMI repository owner (change when using a fork)")
  # cmake-format: on

  if(CMAKE_VERSION VERSION_GREATER_EQUAL 3.24)
    FetchContent_Declare(
      qdmi
      GIT_REPOSITORY https://github.com/${QDMI_REPO_OWNER}/qdmi.git
      GIT_TAG ${QDMI_REV}
      FIND_PACKAGE_ARGS ${QDMI_VERSION})
    list(APPEND FETCH_PACKAGES qdmi)
  else()
    find_package(qdmi ${QDMI_VERSION} QUIET)
    if(NOT qdmi_FOUND)
      FetchContent_Declare(
        qdmi
        GIT_REPOSITORY https://github.com/${QDMI_REPO_OWNER}/qdmi.git
        GIT_TAG ${QDMI_REV})
      list(APPEND FETCH_PACKAGES qdmi)
    endif()
  endif()
  message(
    STATUS "[qdmi][template] QDMI will be included via FetchContent - done")
endif()

if(BUILD_MAESTRO_DEVICE_TESTS)
  set(gtest_force_shared_crt
      ON
      CACHE BOOL "" FORCE)
  set(GTEST_VERSION
      1.14.0
      CACHE STRING "Google Test version")
  set(GTEST_URL
      https://github.com/google/googletest/archive/refs/tags/v${GTEST_VERSION}.tar.gz
  )
  if(CMAKE_VERSION VERSION_GREATER_EQUAL 3.24)
    FetchContent_Declare(googletest URL ${GTEST_URL} FIND_PACKAGE_ARGS
                                        ${GTEST_VERSION} NAMES GTest)
    list(APPEND FETCH_PACKAGES googletest)
  else()
    find_package(googletest ${GTEST_VERSION} QUIET NAMES GTest)
    if(NOT googletest_FOUND)
      FetchContent_Declare(googletest URL ${GTEST_URL})
      list(APPEND FETCH_PACKAGES googletest)
    endif()
  endif()
endif()

# Make all declared dependencies available.
FetchContent_MakeAvailable(${FETCH_PACKAGES})
