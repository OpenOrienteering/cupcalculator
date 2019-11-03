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


# Append the Qt5 runtime directory to the given variable.

function(BUNDLE_HELPER_APPEND_QT5_RUNTIME var)
	get_target_property(qmake_command Qt5::qmake IMPORTED_LOCATION)
	if(WIN32)
		execute_process(
		  COMMAND "${qmake_command}" -query QT_INSTALL_BINS
		  OUTPUT_VARIABLE qt_lib_hint
		  OUTPUT_STRIP_TRAILING_WHITESPACE
		)
	else()
		execute_process(
		  COMMAND "${qmake_command}" -query QT_INSTALL_LIBS
		  OUTPUT_VARIABLE qt_lib_hint
		  OUTPUT_STRIP_TRAILING_WHITESPACE
		)
	endif()
	list(APPEND ${var} "${qt_lib_hint}")
	set(${var} "${${var}}" PARENT_SCOPE)
endfunction()


# Append the MinGW runtime directories to the given variable.

function(BUNDLE_HELPER_APPEND_MINGW_RUNTIME var)
	set(_env_lang $ENV{LC_ALL})
	set(ENV{LC_ALL} C)
	execute_process(
	  COMMAND ${CMAKE_C_COMPILER} --print-search-dirs
	  OUTPUT_VARIABLE mingw_search_dirs
	)
	set(ENV{LC_ALL} ${_env_lang})
	string(REGEX REPLACE ".*libraries: ?=?([^\n]*).*" \\1 mingw_search_dirs "${mingw_search_dirs}")
	string(REPLACE \; \\\; mingw_search_dirs "${mingw_search_dirs}")
	string(REPLACE : \; mingw_search_dirs "${mingw_search_dirs}")
	list(APPEND ${var} ${mingw_search_dirs})
	set(${var} "${${var}}" PARENT_SCOPE)
endfunction()


# Install the Qt5 plugins listed after app to the bundle.

function(BUNDLE_HELPER_INSTALL_QT5_PLUGINS app)
	set(blacklist
	  Qt5::QMinimalIntegrationPlugin
	  Qt5::QOffscreenIntegrationPlugin
	  Qt5::QTuioTouchPlugin
	)
	foreach(module ${ARGN})
		find_package("${module}" REQUIRED)
		foreach(plugin ${${module}_PLUGINS})
			if(plugin IN_LIST blacklist)
				continue()
			endif()
			get_target_property(qt_plugin_location "${plugin}" IMPORTED_LOCATION_RELEASE)
			get_filename_component(qt_plugin_directory "${qt_plugin_location}" DIRECTORY)
			get_filename_component(qt_plugin_subdir "${qt_plugin_directory}" NAME)
			if(APPLE)
				install(
				  FILES "${qt_plugin_location}"
				  DESTINATION "${CMAKE_INSTALL_BINDIR}/${app}.app/Contents/PlugIns/${qt_plugin_subdir}")
			else()
				install(
				  FILES "${qt_plugin_location}"
				  DESTINATION "${CMAKE_INSTALL_BINDIR}/plugins/${qt_plugin_subdir}")
			endif()
		endforeach()
	endforeach()
endfunction()


# Turn the installed app into a bundle with all required runtime libraries.
# The app name can be followed by CMake package names for Qt libraries
# (e.g. "Qt5Widgets") which will have their plugins installed to the bundle.

function(BUNDLE_HELPER_POSTPROCESS app)
	# Collect additional library paths, plugins and tools
	set(BUNDLE_LIB_HINTS )
	if(TARGET Qt5::qmake)
		bundle_helper_append_qt5_runtime(BUNDLE_LIB_HINTS)
		bundle_helper_install_qt5_plugins("${app}" ${ARGN})
	endif()
	if(MINGW)
		bundle_helper_append_mingw_runtime(BUNDLE_LIB_HINTS)
		# Grep is used (and desperately needed) to speed up objdump parsing.
		find_program(gp_grep_cmd NAMES grep)
	endif()
	
	set(BUNDLE_EXECUTABLE "${app}")
	set(script_name "bundle_helper_${app}.cmake")
	configure_file("${PROJECT_SOURCE_DIR}/cmake/bundle_helper_install.cmake.in" "${script_name}" @ONLY)
	install(CODE "include(\"${CMAKE_CURRENT_BINARY_DIR}/${script_name}\")")
endfunction()
