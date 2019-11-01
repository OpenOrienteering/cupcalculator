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

