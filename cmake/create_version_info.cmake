string(TIMESTAMP TODAY UTC)

file(WRITE version_info.hpp.temp "\
#ifndef VERSION_INFO_HPP_\n\
#define VERSION_INFO_HPP_\n\
\n\
namespace {\n\
	constexpr auto app_version_major = ${CMAKE_PROJECT_VERSION_MAJOR};\n\
	constexpr auto app_version_minor = ${CMAKE_PROJECT_VERSION_MINOR};\n\
	constexpr auto app_version_patch = ${CMAKE_PROJECT_VERSION_PATCH};\n\
	constexpr auto app_version_gitrev = \"${GIT_COMMIT_HASH}\";\n\
	constexpr auto app_version_branch = \"${GIT_BRANCH}\";\n\
	constexpr auto timestamp = \"${TODAY}\";\n\
} // namespace\n\
\n\
#endif /* VERSION_INFO_HPP_ */\n\
")

execute_process(COMMAND ${CMAKE_COMMAND} -E copy_if_different
						version_info.hpp.temp ${CMAKE_CURRENT_BINARY_DIR}/version_info.hpp)

file(REMOVE version_info.hpp.temp)