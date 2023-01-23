include(CPM)
CPMAddPackage(
  NAME catch
  VERSION 3.3.0
  URL https://github.com/catchorg/Catch2/archive/refs/tags/v3.3.0.tar.gz
  URL_HASH MD5=fc7a0ea4b83ceed9e4d1be672d529a73
  OVERRIDE_FIND_PACKAGE
)

FetchContent_MakeAvailable(catch)