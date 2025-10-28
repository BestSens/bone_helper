include(CPM)
CPMAddPackage(
  NAME spdlog
  VERSION 1.16.0
  OPTIONS
    "SPDLOG_FMT_EXTERNAL ON"
  URL https://github.com/gabime/spdlog/archive/refs/tags/v1.16.0.tar.gz
  URL_HASH SHA256=8741753e488a78dd0d0024c980e1fb5b5c85888447e309d9cb9d949bdb52aa3e
  OVERRIDE_FIND_PACKAGE
)

find_package(spdlog)