cmake_minimum_required(VERSION 2.8)

project(BURNITNOW)

set(CMAKE_MODULE_PATH "${BURNITNOW_SOURCE_DIR}/CMakeModules")

include(UseHaiku)

set(EXECUTABLE_OUTPUT_PATH ${BURNITNOW_BINARY_DIR})

set(BURNITNOW_SRCS
	main.cpp
	AboutWindow.cpp
	AskName.cpp
	AudioView.cpp
	BurnView.cpp
	CDRWView.cpp
	CopyCDView.cpp
	DataView.cpp
	IconLabel.cpp
	LeftList.cpp
	LogView.cpp
	MakeBFS.cpp
	Prefs.cpp
	PrefsView.cpp
	RightList.cpp
	StatusWindow.cpp
	jpWindow.cpp
	AboutImage.rdef
	BurnItNow.rdef
	gfx.rdef
	)

add_executable("BurnItNow" ${BURNITNOW_SRCS})

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wno-multichar -Woverloaded-virtual -Wno-unknown-pragmas")

target_link_libraries("BurnItNow" "be" "tracker" "media" "translation")
