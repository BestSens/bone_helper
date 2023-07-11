include(CPM)
CPMAddPackage(
  NAME cxxopts
  VERSION 3.1.1
  OPTIONS
    "CXXOPTS_BUILD_EXAMPLES OFF"
    "CXXOPTS_BUILD_TESTS OFF"
  URL https://github.com/jarro2783/cxxopts/archive/refs/tags/v3.1.1.tar.gz
  URL_HASH MD5=61b8bf0d8ab97fd55d67c7c25687b26d
  OVERRIDE_FIND_PACKAGE
)

find_package(cxxopts)