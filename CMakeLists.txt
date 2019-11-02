#
#    Copyright 2019 Kai Pastor
#    
#    This file is part of OpenOrienteering.
# 
#    OpenOrienteering is free software: you can redistribute it and/or modify
#    it under the terms of the GNU General Public License as published by
#    the Free Software Foundation, either version 3 of the License, or
#    (at your option) any later version.
# 
#    OpenOrienteering is distributed in the hope that it will be useful,
#    but WITHOUT ANY WARRANTY; without even the implied warranty of
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#    GNU General Public License for more details.
# 
#    You should have received a copy of the GNU General Public License
#    along with OpenOrienteering.  If not, see <http://www.gnu.org/licenses/>.

cmake_minimum_required(VERSION 3.1 FATAL_ERROR)

project(CupCalculator VERSION 1.1.1 LANGUAGES CXX)


# Packaging 

set(CPACK_PACKAGE_NAME "OpenOrienteering ${CMAKE_PROJECT_NAME}")
set(CPACK_PACKAGE_VENDOR "OpenOrienteering")
set(CPACK_PACKAGE_VERSION_MAJOR ${PROJECT_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${PROJECT_VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${PROJECT_VERSION_PATCH})
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Cup scoring program from OpenOrienteering")
set(CPACK_PACKAGE_FILE_NAME "OpenOrienteering-${CMAKE_PROJECT_NAME}-${PROJECT_VERSION}-${CMAKE_SYSTEM_NAME}")
set(CPACK_RESOURCE_FILE_LICENSE "${PROJECT_SOURCE_DIR}/COPYING")
set(CPACK_STRIP_FILES "TRUE")

# Focus on MinGW
set(CPACK_PACKAGE_EXECUTABLES CupCalculator "${CPACK_PACKAGE_NAME} ${PROJECT_VERSION}")
set(CPACK_PACKAGE_INSTALL_DIRECTORY "${CPACK_PACKAGE_NAME} ${PROJECT_VERSION}")
set(CPACK_GENERATOR "ZIP" CACHE STRING "The binary package generators")

include(CPack)


# Installation

include(GNUInstallDirs)

install(FILES "COPYING"
  RENAME COPYING.txt
  DESTINATION "${CMAKE_INSTALL_DOCDIR}"
)


# Actual build
	
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

if(CMAKE_COMPILER_IS_GNUCXX)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wpedantic -Wextra")
endif()

add_subdirectory(data)
add_subdirectory(src)
add_subdirectory(translations)


# Testing

include(CTest)
enable_testing()
add_test(NAME CupCalculatorTest COMMAND CupCalculatorTest)


# Auxiliary targets

set(ci
  ci/azure-pipelines.yml
  ci/build.yml
  ci/filter-stderr.sed
  ci/publish.yml
  ci/publish-coverage.yml
  ci/release-notes.yml
  ci/setup-common.yml
  ci/setup-msys2.yml
  ci/setup-ubuntu.yml
  ci/shell.sh
  ci/openorienteering-cupcalculator-ci.cmake
)

add_custom_target(CI SOURCES ${ci})

