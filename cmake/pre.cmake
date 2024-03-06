set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(common_compile_options INTERFACE)

option(ENABLE_WCONVERSION "enables warnings for conversions" ON)
option(CRITICAL_WARNINGS "throw error on warnings" ON)

add_compile_options(-Wno-psabi)

target_compile_options(common_compile_options INTERFACE -g -Wall -Wextra -Wpedantic -Wtype-limits)
target_compile_options(common_compile_options INTERFACE "$<$<CONFIG:RELEASE>:-O3;-DNDEBUG>")
target_compile_options(common_compile_options INTERFACE "$<$<CONFIG:DEBUG>:-Og;-DDEBUG;-funwind-tables;-fno-inline>")

if(ENABLE_WCONVERSION)
	target_compile_options(common_compile_options INTERFACE -Wconversion)
endif()

if(CRITICAL_WARNINGS)
	target_compile_options(common_compile_options INTERFACE -Werror)
endif()

if(NOT STATIC_LINK_BINARY)
	target_link_libraries(common_compile_options INTERFACE ${CMAKE_DL_LIBS})
endif()

option(USE_LTO "enable link time optimizations when available" ON)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
	target_compile_options(common_compile_options INTERFACE "$<$<CONFIG:DEBUG>:-rdynamic>")
	target_compile_options(common_compile_options INTERFACE "-fno-var-tracking")
endif()

if(USE_LTO)
	target_compile_options(common_compile_options INTERFACE -flto)

	if(${CMAKE_VERSION} VERSION_LESS "3.13.0") 
		set(CMAKE_SHARED_LINKER_FLAGS "-flto=auto")
	else()
		target_link_options(common_compile_options INTERFACE -flto=auto)
	endif()
endif()

if(STATIC_LINK_BINARY)
	set(CMAKE_FIND_LIBRARY_SUFFIXES ".a")
	set(BUILD_SHARED_LIBS OFF)
	set(CMAKE_EXE_LINKER_FLAGS "-static")
endif()

set(CMAKE_INCLUDE_CURRENT_DIR ON)

option(ENABLE_SYSTEMD "enable linking of systemd" ON)
option(BUILD_TESTS "enable building of tests" ON)
option(AUTORUN_TESTS "enable automatic runs of tests when building Release builds" ON)
option(ENABLE_CCACHE "enables ccache if available" ON)
option(ENABLE_STRIPPING "enable stripping of binary" ON)

if(CMAKE_CROSSCOMPILING)
	set(BUILD_TESTS OFF)
endif()

option(FORCE_COLORED_OUTPUT "Always produce ANSI-colored output (GNU/Clang only)." TRUE)
if(FORCE_COLORED_OUTPUT)
	if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
	   add_compile_options (-fdiagnostics-color=always)
	elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
	   add_compile_options (-fcolor-diagnostics)
	endif ()
endif ()

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

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# depend on deleted version_info.hpp.temp to force rebuiling every time
add_custom_target(version_header ALL DEPENDS 
	${CMAKE_CURRENT_BINARY_DIR}/version_info.hpp
)

add_custom_command(
	OUTPUT	${CMAKE_CURRENT_BINARY_DIR}/version_info.hpp
			${CMAKE_CURRENT_BINARY_DIR}/_version_info.hpp
	COMMAND ${CMAKE_COMMAND}
			-DCMAKE_PROJECT_VERSION_MAJOR=${${CMAKE_PROJECT_NAME}_VERSION_MAJOR}
			-DCMAKE_PROJECT_VERSION_MINOR=${${CMAKE_PROJECT_NAME}_VERSION_MINOR}
			-DCMAKE_PROJECT_VERSION_PATCH=${${CMAKE_PROJECT_NAME}_VERSION_PATCH}
			-DGIT_BRANCH=${GIT_BRANCH}
			-DGIT_COMMIT_HASH=${GIT_COMMIT_HASH}
			-P ${CMAKE_CURRENT_LIST_DIR}/create_version_info.cmake
)

set_source_files_properties(${CMAKE_CURRENT_BINARY_DIR}/version_info.hpp
	PROPERTIES GENERATED TRUE
	HEADER_FILE_ONLY TRUE)

set(THREADS_PREFER_PTHREAD_FLAG ON)
if(CMAKE_CROSSCOMPILING)
	set(THREADS_PTHREAD_ARG "2" CACHE STRING "Forcibly set by CMakeLists.txt." FORCE)
	target_compile_options(common_compile_options INTERFACE "$<$<CONFIG:RELEASE>:-fno-var-tracking-assignments>")
endif()

find_package(Threads REQUIRED)
