cmake_minimum_required(VERSION 3.21)
cmake_policy(SET CMP0069 NEW) 
set(CMAKE_POLICY_DEFAULT_CMP0069 NEW)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

macro(set_build_dir_default TARGET_NAME_)
	set_target_properties(${TARGET_NAME_}
		PROPERTIES
		ARCHIVE_OUTPUT_DIRECTORY "Binary"
		LIBRARY_OUTPUT_DIRECTORY "Library"
		RUNTIME_OUTPUT_DIRECTORY "Binary"
	)
endmacro()

project(ConsoleArrayVisualizer LANGUAGES C)

if (CMAKE_C_COMPILER_FRONTEND_VARIANT STREQUAL "GNU")
	set(MINGW true)
endif()

set(ConsoleArrayVisualizer_HEADER_FILES
	"${ConsoleArrayVisualizer_SOURCE_DIR}/Include/RunSorts.h"
	"${ConsoleArrayVisualizer_SOURCE_DIR}/Include/Utils/Common.h"
	"${ConsoleArrayVisualizer_SOURCE_DIR}/Include/Utils/GuardedMalloc.h"
	"${ConsoleArrayVisualizer_SOURCE_DIR}/Include/Utils/LinkedList.h"
	"${ConsoleArrayVisualizer_SOURCE_DIR}/Include/Utils/Machine.h"
	"${ConsoleArrayVisualizer_SOURCE_DIR}/Include/Utils/MemoryPool.h"
	"${ConsoleArrayVisualizer_SOURCE_DIR}/Include/Utils/Random.h"
	"${ConsoleArrayVisualizer_SOURCE_DIR}/Include/Utils/SharedLock.h"
	"${ConsoleArrayVisualizer_SOURCE_DIR}/Include/Utils/SpinLock.h"
	"${ConsoleArrayVisualizer_SOURCE_DIR}/Include/Utils/Time.h"
	"${ConsoleArrayVisualizer_SOURCE_DIR}/Include/Visualizer.h"
)
set(ConsoleArrayVisualizer_SOURCE_FILES
	"${ConsoleArrayVisualizer_SOURCE_DIR}/Source/Main.c"
	"${ConsoleArrayVisualizer_SOURCE_DIR}/Source/RunSorts.c"
	"${ConsoleArrayVisualizer_SOURCE_DIR}/Source/Sorts/FlashSort.c"
	"${ConsoleArrayVisualizer_SOURCE_DIR}/Source/Sorts/HeapSort.c"
	"${ConsoleArrayVisualizer_SOURCE_DIR}/Source/Sorts/InsertionSort.c"
	"${ConsoleArrayVisualizer_SOURCE_DIR}/Source/Sorts/MergeSort.c"
	"${ConsoleArrayVisualizer_SOURCE_DIR}/Source/Sorts/PigeonholeSort.c"
	"${ConsoleArrayVisualizer_SOURCE_DIR}/Source/Sorts/QuickSort.c"
	"${ConsoleArrayVisualizer_SOURCE_DIR}/Source/Sorts/ShellSort.c"
	"${ConsoleArrayVisualizer_SOURCE_DIR}/Source/Utils/Machine.c"
	"${ConsoleArrayVisualizer_SOURCE_DIR}/Source/Utils/MemoryPool.c"
	"${ConsoleArrayVisualizer_SOURCE_DIR}/Source/Utils/Random.c"
	"${ConsoleArrayVisualizer_SOURCE_DIR}/Source/Utils/Time.c"
	"${ConsoleArrayVisualizer_SOURCE_DIR}/Source/Visualizer/VisualizerCommon.c"
	"${ConsoleArrayVisualizer_SOURCE_DIR}/Source/Visualizer/Renderer/ColumnWindowsConsole.c"
)
set(ConsoleArrayVisualizer_INCLUDE_DIRECTORIES
	"${ConsoleArrayVisualizer_SOURCE_DIR}/Include"
)
if (MINGW)
	list(APPEND ConsoleArrayVisualizer_HEADER_FILES
		"${ConsoleArrayVisualizer_SOURCE_DIR}/Include/CRT/threads.h"
	)
	list(APPEND ConsoleArrayVisualizer_SOURCE_FILES
		"${ConsoleArrayVisualizer_SOURCE_DIR}/Source/CRT/c11threads_win32.c"
	)
	list(APPEND ConsoleArrayVisualizer_INCLUDE_DIRECTORIES
		"${ConsoleArrayVisualizer_SOURCE_DIR}/Include/CRT"
	)
endif()

add_executable(ConsoleArrayVisualizer
	${ConsoleArrayVisualizer_HEADER_FILES}
	${ConsoleArrayVisualizer_SOURCE_FILES}
)
target_include_directories(ConsoleArrayVisualizer PRIVATE
	${ConsoleArrayVisualizer_INCLUDE_DIRECTORIES}
)
set_target_properties(ConsoleArrayVisualizer PROPERTIES
	INTERPROCEDURAL_OPTIMIZATION TRUE
	C_STANDARD 23
)
target_link_libraries(ConsoleArrayVisualizer ntdll)
if (MINGW)
	target_link_options(ConsoleArrayVisualizer PRIVATE -Wl,--pdb=)
endif()

set_build_dir_default(ConsoleArrayVisualizer)
