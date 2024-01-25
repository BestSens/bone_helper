include(CPM)
CPMAddPackage(
  NAME spdlog
  VERSION 1.13.0
  OPTIONS
    "SPDLOG_FMT_EXTERNAL ON"
  GIT_REPOSITORY "https://github.com/gabime/spdlog.git"
  GIT_TAG 7c02e204c92545f869e2f04edaab1f19fe8b19fd
  OVERRIDE_FIND_PACKAGE
)

find_package(spdlog)