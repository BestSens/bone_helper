project(bone_helper)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

add_library(bone_helper STATIC src/loopTimer.cpp src/stdlib_backports.cpp src/system_helper.cpp src/netHelper.cpp)
find_package(MbedTLS REQUIRED)

target_include_directories(bone_helper PRIVATE include)
target_include_directories(bone_helper PRIVATE libs/tinydir ${MBEDTLS_INCLUDE_DIRS})
target_include_directories(bone_helper INTERFACE include)

target_link_libraries(bone_helper PRIVATE nlohmann_json::nlohmann_json fmt spdlog mbedtls)

find_package(PkgConfig)
pkg_check_modules(SYSTEMD QUIET "libsystemd")
if(SYSTEMD_FOUND AND ENABLE_SYSTEMD)
	add_definitions(-DENABLE_SYSTEMD_STATUS)
	target_link_libraries(bone_helper PRIVATE systemd)
	message(STATUS "systemd enabled")
endif()