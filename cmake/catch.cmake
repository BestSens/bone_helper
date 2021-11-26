if(${CMAKE_VERSION} VERSION_LESS "3.10")
	execute_process(COMMAND bash "-c" "sed -i 's/VERSION 3.5/VERSION 3.3/' \"${CMAKE_SOURCE_DIR}/libs/catch/CMakeLists.txt\"")
endif()

add_subdirectory("libs/catch")