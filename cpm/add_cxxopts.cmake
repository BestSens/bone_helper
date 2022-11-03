include(CPM)
CPMAddPackage(
  NAME cxxopts
  VERSION 3.0.0
  OPTIONS
    "CXXOPTS_BUILD_EXAMPLES OFF"
    "CXXOPTS_BUILD_TESTS OFF"
  URL https://github.com/jarro2783/cxxopts/archive/refs/tags/v3.0.0.tar.gz
  URL_HASH MD5=4c4cb6e2f252157d096fe18451ab451e
  OVERRIDE_FIND_PACKAGE
)

find_package(cxxopts)