cmake_minimum_required(VERSION 3.24)
find_package(Boost REQUIRED)

add_executable(run_test_bone_helper
	src/test_circular_buffer.cpp 
	src/test_loopTimer.cpp 
	src/test_stopwatch.cpp
	src/test_jsonHelper.cpp
)

target_include_directories(run_test_bone_helper PRIVATE
	${PROJECT_SOURCE_DIR}/include
	${Boost_INCLUDE_DIRS}
	data
)

if(ENABLE_WCONVERSION)
	target_compile_options(run_test_bone_helper PRIVATE -Wconversion)
endif()

if(CRITICAL_WARNINGS)
	target_compile_options(run_test_bone_helper PRIVATE -Werror)
endif()

target_compile_options(run_test_bone_helper PRIVATE -fsanitize=address -fsanitize=undefined -Wall -Wextra -Wpedantic -Wtype-limits)
target_link_options(run_test_bone_helper PRIVATE -fsanitize=address -fsanitize=undefined)

find_package(Threads REQUIRED)

target_link_libraries(run_test_bone_helper PRIVATE
	Catch2::Catch2WithMain
	nlohmann_json::nlohmann_json
	ssl
	crypto
	pthread
	${Boost_LIBRARIES}
)

add_test(NAME test COMMAND run_test_bone_helper)

enable_testing()

# Automatically run tests for release builds
if(CMAKE_BUILD_TYPE STREQUAL Release AND AUTORUN_TESTS)
	add_custom_command(
		TARGET run_test_bone_helper
		COMMENT "Run tests"
		POST_BUILD 
		WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
		COMMAND run_test_bone_helper
	)
endif()