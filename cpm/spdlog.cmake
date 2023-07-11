include(CPM)
CPMAddPackage(
  NAME spdlog
  VERSION 1.12.0
  OPTIONS
    "SPDLOG_FMT_EXTERNAL ON"
  GIT_REPOSITORY "https://github.com/gabime/spdlog.git"
  GIT_TAG 7e635fca68d014934b4af8a1cf874f63989352b7
  OVERRIDE_FIND_PACKAGE
)

find_package(spdlog)