set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED True)

add_library(common_compile_options INTERFACE)

target_compile_options(common_compile_options INTERFACE -Wall -Wextra -Wpedantic -Wtype-limits)
target_compile_options(common_compile_options INTERFACE "$<$<CONFIG:RELEASE>:-O3;-DNDEBUG>")
target_compile_options(common_compile_options INTERFACE "$<$<CONFIG:DEBUG>:-Og;-DDEBUG;-g;-funwind-tables;-fno-inline>")

target_link_libraries(common_compile_options INTERFACE ${CMAKE_DL_LIBS})

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
	target_compile_options(common_compile_options INTERFACE "$<$<CONFIG:DEBUG>:-rdynamic>")

	if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS 5.0)
		target_compile_options(common_compile_options INTERFACE -Wno-pragmas -Wno-missing-field-initializers)
	endif()
endif()

set(CMAKE_INCLUDE_CURRENT_DIR ON)

option(ENABLE_SYSTEMD "enable linking of systemd" ON)
option(BUILD_TESTS "enable building of tests" ON)
option(AUTORUN_TESTS "enable automatic runs of tests when building Release builds" ON)
option(ENABLE_CCACHE "enables ccache if available" ON)

if(CMAKE_CROSSCOMPILING)
	set(BUILD_TESTS OFF)
endif()

if(BUILD_TESTS)
	message(STATUS "building tests enabled")
endif()

if(ENABLE_CCACHE)
	find_program(CCACHE_FOUND ccache)
	if(CCACHE_FOUND)
		message(STATUS "ccache enabled")
		set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE ccache)
		set_property(GLOBAL PROPERTY RULE_LAUNCH_LINK ccache)
	endif(CCACHE_FOUND)
endif()

if(NOT GIT_BRANCH)
	# Get the current working branch
	execute_process(
		COMMAND git rev-parse --abbrev-ref HEAD
		WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
		OUTPUT_VARIABLE GIT_BRANCH
		OUTPUT_STRIP_TRAILING_WHITESPACE
	)
endif()

if(NOT GIT_COMMIT_HASH)
	# Get the latest abbreviated commit hash of the working branch
	execute_process(
		COMMAND git rev-parse --verify --short=8 HEAD
		WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
		OUTPUT_VARIABLE GIT_COMMIT_HASH
		OUTPUT_STRIP_TRAILING_WHITESPACE
	)
endif()

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# depend on deleted version_info.hpp.temp to force rebuiling every time
add_custom_target(version_header ALL DEPENDS 
	${CMAKE_CURRENT_BINARY_DIR}/version_info.hpp
	${CMAKE_CURRENT_BINARY_DIR}/version_info.hpp.temp
)

add_custom_command(OUTPUT
	${CMAKE_CURRENT_BINARY_DIR}/version_info.hpp
	${CMAKE_CURRENT_BINARY_DIR}/version_info.hpp.temp
	COMMAND ${CMAKE_COMMAND}
	-DCMAKE_PROJECT_VERSION_MAJOR=${${CMAKE_PROJECT_NAME}_VERSION_MAJOR}
	-DCMAKE_PROJECT_VERSION_MINOR=${${CMAKE_PROJECT_NAME}_VERSION_MINOR}
	-DCMAKE_PROJECT_VERSION_PATCH=${${CMAKE_PROJECT_NAME}_VERSION_PATCH}
	-DGIT_BRANCH=${GIT_BRANCH}
	-DGIT_COMMIT_HASH=${GIT_COMMIT_HASH}
	-P ${CMAKE_CURRENT_LIST_DIR}/create_version_info.cmake)

set_source_files_properties(${CMAKE_CURRENT_BINARY_DIR}/version_info.hpp
	PROPERTIES GENERATED TRUE
	HEADER_FILE_ONLY TRUE)

set(THREADS_PREFER_PTHREAD_FLAG ON)
if(CMAKE_CROSSCOMPILING)
	set(THREADS_PTHREAD_ARG "2" CACHE STRING "Forcibly set by CMakeLists.txt." FORCE)
endif()

find_package(Threads REQUIRED)
