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


if(COMMAND unix2dos)
	# include guard
	return()
elseif(NOT WIN32)
	# no-op if not building for Windows
	function(UNIX2DOS)
	endfunction()
	function(UNIX2DOS_INSTALLED)
	endfunction()
	return()
endif()


message(STATUS "Enabling unix2dos")

find_program(UNIX2DOS_COMMAND
  NAMES unix2dos
  DOC "Filepath of the unix2dos executable"
)
find_program(SED_COMMAND
  NAMES gsed sed
  DOC "Filepath of the sed executable"
)
if(NOT UNIX2DOS_COMMAND AND NOT SED_COMMAND)
	message(WARNING "unix2dos or sed are required to convert text files for Windows")
endif()
mark_as_advanced(UNIX2DOS_COMMAND)
mark_as_advanced(UNIX2DOS_SED_COMMAND)


# On Windows, convert the files matching the given pattern from UNIX to Windows
# line endings. Cf. CMake's file(GLOB ...) for the pattern syntax
function(UNIX2DOS)
	file(GLOB files ${ARGN} LIST_DIRECTORIES false)
	foreach(file ${files})
		if(UNIX2DOS_COMMAND)
			execute_process(COMMAND "${UNIX2DOS_COMMAND}" -ascii --quiet "${file}")
		elseif(UNIX2DOS_SED_COMMAND)
			execute_process(
			  COMMAND "${UNIX2DOS_SED_COMMAND}" -e "s,\\r*$,\\r," -i -- "${file}"
			  COMMAND "${CMAKE_COMMAND}" -E remove -f "${file}--"
			)
		endif()
	endforeach()
endfunction()


function(UNIX2DOS_INSTALLED)
	set(code
	  "list(APPEND CMAKE_MODULE_PATH \"${PROJECT_SOURCE_DIR}/cmake\")"
	  "include(\"unix2dos\")"
	)
	foreach(pattern ${ARGN})
		list(APPEND code "unix2dos(\"\$ENV{DESTDIR}\${CMAKE_INSTALL_PREFIX}/${pattern}\")")
	endforeach()
	string(REPLACE ";" "\n  " code "${code}")
	install(CODE "${code}")
endfunction()
