include(CPM)
CPMAddPackage(
  NAME catch
  VERSION 3.1.1
  URL https://github.com/catchorg/Catch2/archive/refs/tags/v3.1.1.tar.gz
  URL_HASH MD5=1f3e0d8c3297252f77d643ff06d058cb
  OVERRIDE_FIND_PACKAGE
)

FetchContent_MakeAvailable(catch)