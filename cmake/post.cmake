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