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


# Script mode
if(UNIX2DOS_FILE)
	file(READ "${UNIX2DOS_FILE}" content)
	string(REGEX REPLACE "\r*\n" "\r\n" content "${content}")
	file(WRITE "${UNIX2DOS_FILE}" "${content}")
	return()
endif()


set(UNIX2DOS_ENABLED   "${WIN32}" CACHE BOOL "Convert line endings for Windows")


if(COMMAND unix2dos)
	# include guard
	return()
elseif(NOT UNIX2DOS_ENABLED)
	function(UNIX2DOS)
	endfunction()
	function(UNIX2DOS_INSTALLED)
	endfunction()
	return()
endif()


# Windows-only

set(UNIX2DOS_LIST_FILE "${CMAKE_CURRENT_LIST_FILE}")


# Convert the files matching the given pattern to Windows line endings.
# Cf. CMake's file(GLOB ...) for the pattern syntax

function(UNIX2DOS)
	file(GLOB files LIST_DIRECTORIES false ${ARGN})
	foreach(file ${files})
		execute_process(
		  COMMAND "${CMAKE_COMMAND}" "-DUNIX2DOS_FILE=${file}" -P "${UNIX2DOS_LIST_FILE}"
		  RESULT_VARIABLE result
		  ERROR_VARIABLE error
		)
		if(NOT result EQUAL 0)
			message(FATAL_ERROR "${error}")
		endif()
	endforeach()
endfunction()


# At installation time, convert the files matching the given installation path
# pattern from UNIX to Windows line endings.
# Cf. CMake's file(GLOB ...) for the pattern syntax

function(UNIX2DOS_INSTALLED)
	set(code
	  "set(UNIX2DOS_ENABLED ON CACHE BOOL \"Convert line endings for Windows\")"
	  "include(\"${UNIX2DOS_LIST_FILE}\")"
	)
	foreach(pattern ${ARGN})
		list(APPEND code "unix2dos(\"\$ENV{DESTDIR}\${CMAKE_INSTALL_PREFIX}/${pattern}\")")
	endforeach()
	string(REPLACE ";" "\n  " code "${code}")
	install(CODE "${code}")
endfunction()

