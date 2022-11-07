include(CPM)
CPMAddPackage(
  NAME spdlog
  VERSION 1.11.0
  OPTIONS
    "SPDLOG_FMT_EXTERNAL ON"
  URL https://github.com/gabime/spdlog/archive/refs/tags/v1.11.0.tar.gz
  URL_HASH MD5=287c6492c25044fd2da9947ab120b2bd
  OVERRIDE_FIND_PACKAGE
)

find_package(spdlog)