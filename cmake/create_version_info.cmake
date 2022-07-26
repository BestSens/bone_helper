string(TIMESTAMP TODAY UTC)

execute_process(COMMAND git rev-parse --verify --short=8 HEAD
		OUTPUT_VARIABLE GIT_REV ERROR_QUIET
		OUTPUT_STRIP_TRAILING_WHITESPACE
)

# Check whether we got any revision (which isn't
# always the case, e.g. when someone downloaded a zip
# file from Github instead of a checkout)
if ("${GIT_REV}" STREQUAL "")
	set(GIT_REV "")
	set(GIT_DIFF "")
	set(GIT_TAG "")

	if(NOT GIT_BRANCH)
		set(GIT_BRANCH "")
	endif()
else()
	execute_process(
		COMMAND bash -c "git diff --quiet --exit-code || echo -dirty"
		OUTPUT_VARIABLE GIT_DIFF
		OUTPUT_STRIP_TRAILING_WHITESPACE
	)

	execute_process(
		COMMAND git describe --exact-match --tags
		OUTPUT_VARIABLE GIT_TAG ERROR_QUIET
		OUTPUT_STRIP_TRAILING_WHITESPACE
	)

	if(NOT GIT_BRANCH)
		execute_process(
			COMMAND git rev-parse --abbrev-ref HEAD
			OUTPUT_VARIABLE GIT_BRANCH
			OUTPUT_STRIP_TRAILING_WHITESPACE
		)
	endif()
endif()

if(NOT GIT_COMMIT_HASH)
	set(GIT_COMMIT_HASH "${GIT_REV}${GIT_DIFF}")
endif()

set(VERSION_HPP "\
#ifndef VERSION_INFO_HPP_\n\
#define VERSION_INFO_HPP_\n\
\n\
namespace {\n\
	constexpr auto app_version_major = ${CMAKE_PROJECT_VERSION_MAJOR};\n\
	constexpr auto app_version_minor = ${CMAKE_PROJECT_VERSION_MINOR};\n\
	constexpr auto app_version_patch = ${CMAKE_PROJECT_VERSION_PATCH};\n\
	constexpr auto app_version_gitrev = \"${GIT_COMMIT_HASH}\";\n\
	constexpr auto app_version_branch = \"${GIT_BRANCH}\";\n\
	constexpr auto app_version_tag = \"${GIT_TAG}\";\n\
	constexpr auto timestamp = \"${TODAY}\";\n\
} // namespace\n\
\n\
#endif /* VERSION_INFO_HPP_ */\n\
")

if(EXISTS ${CMAKE_CURRENT_BINARY_DIR}/version_info.hpp)
    file(READ ${CMAKE_CURRENT_BINARY_DIR}/version_info.hpp VERSION_HPP_)
else()
    set(VERSION_HPP_ "")
endif()

if (NOT "${VERSION_HPP}" STREQUAL "${VERSION_HPP_}")
    file(WRITE ${CMAKE_CURRENT_SOURCE_DIR}/version_info.hpp "${VERSION_HPP}")
endif()