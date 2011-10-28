project(OResults)
cmake_minimum_required(VERSION 2.6)
find_package(Qt4 REQUIRED)

enable_testing()

#set(CMAKE_BUILD_TYPE Release)
set(CMAKE_BUILD_TYPE Debug)

add_definitions(-ansi -pedantic)
if (CMAKE_COMPILER_IS_GNUCXX)
	add_definitions(-Wno-long-long -Wno-deprecated  -Wall)
endif (CMAKE_COMPILER_IS_GNUCXX)
set(CMAKE_CXX_FLAGS_DEBUG "-g3 -O0 -DDEBUG")
set(CMAKE_CXX_FLAGS_RELEASE "-g0 -O2 -s")

include_directories(${QT_INCLUDES} ${CMAKE_CURRENT_BINARY_DIR})

set(Common_SRCS
 src/mainWindow.cpp
 
 src/location.cpp
 
 src/club.cpp
 src/clubDialog.cpp
 src/runner.cpp
 
 src/scoring.cpp
 src/scoringDialog.cpp

 src/seriesScoring.cpp
 src/seriesScoringDialog.cpp

 src/layout.cpp
 src/presentScoring.cpp
 
 src/event.cpp
 src/resultList.cpp
 
 src/util.cpp
 src/problemWidget.cpp
 src/comboBoxDelegate.cpp
 src/csvFile.cpp
 src/resultCsvImport.cpp
)
set(OResults_SRCS ${Common_SRCS} src/main.cpp)
qt4_automoc(${OResults_SRCS} src/main.cpp)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin)
add_executable(OResults ${OResults_SRCS})
target_link_libraries(OResults
 ${QT_QTCORE_LIBRARY}
 ${QT_QTGUI_LIBRARY}
)

# Testing
set(OResultsTest_SRCS ${Common_SRCS} src/test.cpp)
qt4_automoc(src/test.cpp)
add_executable(OResultsTest ${OResultsTest_SRCS})
add_test(NAME ScoringToolTest COMMAND $<TARGET_FILE:OResultsTest>)

target_link_libraries(OResultsTest
 ${QT_QTCORE_LIBRARY}
 ${QT_QTGUI_LIBRARY}
 ${QT_QTTEST_LIBRARY}
)
