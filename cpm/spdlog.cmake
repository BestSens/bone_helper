include(CPM)
CPMAddPackage(
  NAME spdlog
  VERSION 1.15.2
  OPTIONS
    "SPDLOG_FMT_EXTERNAL ON"
  URL https://github.com/gabime/spdlog/archive/refs/tags/v1.15.2.tar.gz
  URL_HASH SHA256=7a80896357f3e8e920e85e92633b14ba0f229c506e6f978578bdc35ba09e9a5d
  OVERRIDE_FIND_PACKAGE
)

find_package(spdlog)