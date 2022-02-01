target_link_libraries(${MAIN_EXECUTABLE} PRIVATE ${CMAKE_DL_LIBS})

find_package(PkgConfig)
pkg_check_modules(SYSTEMD QUIET "libsystemd")
if(SYSTEMD_FOUND AND ENABLE_SYSTEMD)
	target_compile_definitions(${MAIN_EXECUTABLE} PRIVATE ENABLE_SYSTEMD_STATUS)
	target_link_libraries(${MAIN_EXECUTABLE} PRIVATE systemd)
	message(STATUS "systemd enabled")
endif()

# "Use" NOSTRIP variable to get rid of warning
set(ignoreMe "${NOSTRIP}")

# Strip binary release builds
if(NOT DEFINED NOSTRIP AND CMAKE_BUILD_TYPE STREQUAL Release)
	if(DEFINED ENV{STRIP})
		set(CMAKE_STRIP $ENV{STRIP})
	endif()

	add_custom_command(TARGET ${MAIN_EXECUTABLE} POST_BUILD
			COMMAND ${CMAKE_STRIP} ${MAIN_EXECUTABLE})
endif()