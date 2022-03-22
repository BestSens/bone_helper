# Strip binary release builds
if(ENABLE_STRIPPING)
	if(DEFINED ENV{STRIP})
		set(CMAKE_STRIP $ENV{STRIP})
	endif()

	add_custom_command(TARGET ${MAIN_EXECUTABLE} POST_BUILD
			COMMAND ${CMAKE_OBJCOPY} --only-keep-debug "${MAIN_EXECUTABLE}" "${MAIN_EXECUTABLE}.dbg"
			COMMAND ${CMAKE_STRIP} "${MAIN_EXECUTABLE}"
			COMMAND ${CMAKE_OBJCOPY} --add-gnu-debuglink="${MAIN_EXECUTABLE}.dbg" "${MAIN_EXECUTABLE}")
endif()