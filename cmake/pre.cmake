set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED True)

add_compile_options(-Wall -Wextra -Wpedantic -Wtype-limits)
add_compile_options("$<$<CONFIG:RELEASE>:-O3;-DNDEBUG>")
add_compile_options("$<$<CONFIG:DEBUG>:-Og;-DDEBUG;-g;-funwind-tables;-fno-inline>")

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
	add_compile_options("$<$<CONFIG:DEBUG>:-rdynamic>")

	if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS 5.0)
		add_compile_options(-Wno-pragmas -Wno-missing-field-initializers)
	endif()
endif()

set(CMAKE_INCLUDE_CURRENT_DIR ON)

option(ENABLE_SYSTEMD "enable linking of systemd" ON)

find_program(CCACHE_FOUND ccache)
if(CCACHE_FOUND)
	message(STATUS "ccache enabled")
    set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE ccache)
    set_property(GLOBAL PROPERTY RULE_LAUNCH_LINK ccache)
endif(CCACHE_FOUND)

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

configure_file(version_info.hpp.in version_info.hpp)

set(THREADS_PREFER_PTHREAD_FLAG ON)
if(${CMAKE_CROSSCOMPILING})
	set(THREADS_PTHREAD_ARG "2" CACHE STRING "Forcibly set by CMakeLists.txt." FORCE)
endif()

find_package(Threads REQUIRED)
