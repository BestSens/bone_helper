project(bone_helper)

add_library(bone_helper STATIC src/loopTimer.cpp src/stdlib_backports.cpp src/system_helper.cpp)

target_include_directories(bone_helper PRIVATE include)
target_include_directories(bone_helper INTERFACE include)
target_link_libraries(bone_helper PRIVATE nlohmann_json::nlohmann_json)

find_package(PkgConfig)
pkg_check_modules(SYSTEMD QUIET "libsystemd")
if(SYSTEMD_FOUND AND ENABLE_SYSTEMD)
	add_definitions(-DENABLE_SYSTEMD_STATUS)
	target_link_libraries(bone_helper PRIVATE systemd)
	message(STATUS "systemd enabled")
endif()