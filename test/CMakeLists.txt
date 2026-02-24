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

if(NOT CXX_DEVICE)
  # add CXX language support
  enable_language(CXX)
endif()

# ------------------------------------------------------------------------------
# Non-Functional Tests
# ------------------------------------------------------------------------------

generate_device_defs_executable(${QDMI_PREFIX})

# ------------------------------------------------------------------------------
# Functional Tests
# ------------------------------------------------------------------------------

# create an executable in which the tests will be stored
add_executable(maestro_device_test test_maestro_device.cpp maestro_backend_tests.cpp)

# link the Google test infrastructure to the test executable.
target_link_libraries(maestro_device_test PRIVATE gtest_main qdmi::qdmi maestro_device
                                             qdmi::qdmi_project_warnings)

# set c++ standard
target_compile_features(maestro_device_test PRIVATE cxx_std_17)

# ensure maestro_device and maestro_device_definitions_test are built whenever
# maestro_device_test is built
add_dependencies(maestro_device_test qdmi_test_maestro_device_defs)

# only discover tests if this is the main project
if(NOT MAESTRO_DEVICE_MASTER_PROJECT)
  return()
endif()

gtest_discover_tests(
  maestro_device_test
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
             DISCOVERY_TIMEOUT 60)
