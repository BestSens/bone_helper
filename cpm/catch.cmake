include(CPM)
CPMAddPackage(
  NAME catch
  VERSION 3.3.1
  URL https://github.com/catchorg/Catch2/archive/refs/tags/v3.3.1.tar.gz
  URL_HASH MD5=5cdc99f93e0b709936eb5af973df2a5c
  OVERRIDE_FIND_PACKAGE
)

FetchContent_MakeAvailable(catch)