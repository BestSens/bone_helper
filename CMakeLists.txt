project(bone_helper)

cmake_minimum_required(VERSION 3.24)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

add_library(bone_helper STATIC EXCLUDE_FROM_ALL
	src/system_helper.cpp
	src/netHelper.cpp
	src/strnatcmp.cpp
	src/fsHelper.cpp
)

target_compile_features(bone_helper PUBLIC cxx_std_23)

if(POLICY CMP0167)
	cmake_policy(SET CMP0167 NEW)
endif()

find_package(Boost REQUIRED filesystem)

target_include_directories(bone_helper PUBLIC include ${Boost_INCLUDE_DIRS})
target_include_directories(bone_helper PRIVATE libs/tinydir)

target_link_libraries(bone_helper PRIVATE
	nlohmann_json::nlohmann_json
	fmt
	spdlog
)

target_link_libraries(bone_helper PUBLIC
	ssl
	crypto
	atomic
	${Boost_LIBRARIES}
)

if(NOT STATIC_LINK_BINARY)
	target_link_libraries(bone_helper PUBLIC ${CMAKE_DL_LIBS})
endif()

add_library(uff_writer STATIC EXCLUDE_FROM_ALL
	src/uffWriter.cpp
)

target_include_directories(uff_writer PUBLIC include)
target_link_libraries(uff_writer PRIVATE bone_helper fmt)

find_package(PkgConfig)
pkg_check_modules(SYSTEMD "libsystemd")
if(SYSTEMD_FOUND AND ENABLE_SYSTEMD)
	target_compile_definitions(bone_helper PRIVATE ENABLE_SYSTEMD_STATUS)
	target_compile_definitions(bone_helper INTERFACE ENABLE_SYSTEMD_STATUS)
	target_link_libraries(bone_helper PRIVATE systemd)
	message(STATUS "systemd enabled")
endif()

if(BUILD_TESTS)
	add_subdirectory(test)
endif()