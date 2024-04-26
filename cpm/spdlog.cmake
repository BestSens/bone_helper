include(CPM)
CPMAddPackage(
  NAME spdlog
  VERSION 1.14.0
  OPTIONS
    "SPDLOG_FMT_EXTERNAL ON"
  URL https://github.com/gabime/spdlog/archive/refs/tags/v1.14.0.tar.gz
  URL_HASH MD5=fa062afc1e88ee3bad712dcd2d5a270a
  OVERRIDE_FIND_PACKAGE
)

find_package(spdlog)