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
add_library(maestro_device SHARED maestro_device.cpp)
target_link_libraries(maestro_device PRIVATE qdmi::qdmi qdmi::project_warnings)
generate_prefixed_qdmi_headers(${QDMI_PREFIX})
target_include_directories(maestro_device PUBLIC ${CMAKE_CURRENT_BINARY_DIR}/include)
if(NOT CXX_DEVICE)
  # set c++ standard
  target_compile_features(maestro_device PRIVATE cxx_std_17)
endif()
add_library(qdmi::maestro_device ALIAS maestro_device)
