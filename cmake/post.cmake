# Strip binary release builds
if(ENABLE_STRIPPING)
	if(DEFINED ENV{STRIP})
		set(CMAKE_STRIP $ENV{STRIP})
	endif()

	add_custom_command(TARGET ${MAIN_EXECUTABLE} POST_BUILD
			COMMAND ${CMAKE_STRIP} ${MAIN_EXECUTABLE})
endif()