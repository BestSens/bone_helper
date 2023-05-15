include(CPM)
CPMAddPackage(
  NAME spdlog
  VERSION 1.11.0
  OPTIONS
    "SPDLOG_FMT_EXTERNAL ON"
  GIT_REPOSITORY "https://github.com/gabime/spdlog.git"
  GIT_TAG 57a9fd0841f00e92b478a07fef62636d7be612a8
  OVERRIDE_FIND_PACKAGE
)

find_package(spdlog)