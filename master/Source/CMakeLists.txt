if(CMAKE_BUILD_TYPE STREQUAL "Debug")
	add_definitions(-DDEBUG)
endif()

set(EXECUTABLE_OUTPUT_PATH ${BURNITNOW_BINARY_DIR})

set(BURNITNOW_SRCS
	BurnApplication.cpp
	BurnWindow.cpp
	CommandThread.cpp
	CompilationAudioView.cpp
	CompilationDataView.cpp
	CompilationImageView.cpp
	BurnItNow.rdef

# 3rd party sources
	Haiku/CommandPipe.cpp
)

add_executable(BurnItNow ${BURNITNOW_SRCS})

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wno-multichar -Woverloaded-virtual -Wno-unknown-pragmas")

include_directories(${BURNITNOW_SOURCE_DIR}/Source/Haiku)

target_link_libraries(BurnItNow "be" "tracker")
