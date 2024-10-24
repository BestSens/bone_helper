include(CPM)
CPMAddPackage(
  NAME catch
  VERSION 3.7.1
  URL https://github.com/catchorg/Catch2/archive/refs/tags/v3.7.1.tar.gz
  URL_HASH MD5=9fcbec1dc95edcb31c6a0d6c5320e098
  OVERRIDE_FIND_PACKAGE
)

FetchContent_MakeAvailable(catch)