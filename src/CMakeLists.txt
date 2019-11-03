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


find_package(Qt5Widgets REQUIRED)
find_package(Qt5Test REQUIRED)

include("bundle_helper")

set(CMAKE_AUTOMOC ON)

add_library(CupCalculator_Common STATIC
	mainWindow.cpp
	
	location.cpp
	
	club.cpp
	clubDialog.cpp
	runner.cpp
	
	scoring.cpp
	scoringDialog.cpp
   
	seriesScoring.cpp
	seriesScoringDialog.cpp
   
	layout.cpp
	presentScoring.cpp
	
	event.cpp
	resultList.cpp
	
	util.cpp
	problemWidget.cpp
	comboBoxDelegate.cpp
	csvFile.cpp
	resultCsvImport.cpp
)
target_link_libraries(CupCalculator_Common
  PUBLIC Qt5::Widgets
)

add_executable(CupCalculator WIN32 main.cpp)
target_link_libraries(CupCalculator
  PUBLIC
    CupCalculator_Common
    Qt5::Widgets
)
install(TARGETS CupCalculator
  RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
)
if(MAKE_BUNDLE)
	bundle_helper_postprocess(CupCalculator Qt5Widgets Qt5Gui)
endif()

add_executable(CupCalculatorTest test.cpp)
target_link_libraries(CupCalculatorTest
  PUBLIC
    CupCalculator_Common
    Qt5::Widgets
    Qt5::Test
)

