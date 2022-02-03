project(bone_helper)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

option(BONE_HELPER_SSL "enable SSL support" ON)

add_library(bone_helper STATIC src/loopTimer.cpp src/stdlib_backports.cpp src/system_helper.cpp src/netHelper.cpp)

target_include_directories(bone_helper PRIVATE include)
target_include_directories(bone_helper INTERFACE include)
target_include_directories(bone_helper PRIVATE libs/tinydir)

target_link_libraries(bone_helper PRIVATE nlohmann_json::nlohmann_json fmt spdlog)

find_package(MbedTLS REQUIRED)
target_include_directories(bone_helper PRIVATE ${MBEDTLS_INCLUDE_DIRS})
target_link_libraries(bone_helper INTERFACE mbedtls)

if(NOT BONE_HELPER_SSL)
	target_compile_definitions(bone_helper PRIVATE BONE_HELPER_NO_SSL)
	target_compile_definitions(bone_helper INTERFACE BONE_HELPER_NO_SSL)
	message(STATUS "building without ssl support")
endif()

find_package(PkgConfig)
pkg_check_modules(SYSTEMD QUIET "libsystemd")
if(SYSTEMD_FOUND AND ENABLE_SYSTEMD)
	target_compile_definitions(bone_helper PRIVATE ENABLE_SYSTEMD_STATUS)
	target_compile_definitions(bone_helper INTERFACE ENABLE_SYSTEMD_STATUS)
	target_link_libraries(bone_helper PRIVATE systemd)
	message(STATUS "systemd enabled")
endif()