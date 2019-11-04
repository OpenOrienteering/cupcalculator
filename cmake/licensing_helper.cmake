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


# Setup a licensing helper script to be run at install time.

find_file(LICENSING_COPYRIGHT_DIR
  NAMES copyright
  PATH_SUFFIXES share/doc
  DOC "Path to directory with 3rd-party component copyright files."
)

find_file(LICENSING_COMMON_DIR
  NAMES common-licenses
  PATHS "${LICENSING_COPYRIGHT_DIR}"
  NO_DEFAULT_PATH
  DOC "Path to directory with common license files."
)

set(LICENSING_OPTIONAL_ITEMS "" CACHE STRING
  "List of items which may be deployed even when licensing documentation is absent"
)


# Deploy 3rd-party copyright, terms etc. 
# The app name can be followed by the filenames of its explicit license texts as
# installed to the documenation directory. They will be checked for duplicates
# in the common license text. If an identical file is found there, the named
# file is removed.

function(LICENSING_HELPER_POSTPROCESS app)
	if(NOT LICENSING_COPYRIGHT_DIR OR NOT LICENSING_COMMON_DIR)
		message(STATUS "LICENSING_COPYRIGHT_DIR: ${LICENSING_COPYRIGHT_DIR}")
		message(STATUS "LICENSING_COMMON_DIR:    ${LICENSING_COMMON_DIR}")
		message(FATAL_ERROR "Both LICENSING_COPYRIGHT_DIR and LICENSING_COMMON_DIR must be set")
	endif()	
	
	set(BUNDLE_EXECUTABLE "${app}")
	set(BUNDLE_COPYRIGHT "${ARGN}")
	set(script_name "licensing_helper_${app}.cmake")
	configure_file("${PROJECT_SOURCE_DIR}/cmake/licensing_helper_install.cmake.in" "${script_name}" @ONLY)
	install(CODE "include(\"${CMAKE_CURRENT_BINARY_DIR}/${script_name}\")")
endfunction()
