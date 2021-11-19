if(${CMAKE_VERSION} VERSION_LESS "3.10")
	execute_process(COMMAND bash "-c" "sed -i 's/VERSION 3.10/VERSION 3.3/' \"${CMAKE_SOURCE_DIR}/libs/spdlog/CMakeLists.txt\"")
endif()

option(SPDLOG_FMT_EXTERNAL "Use external fmt library instead of bundled" ON)
add_subdirectory("libs/spdlog")