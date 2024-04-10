include(CPM)
CPMAddPackage(
  NAME catch
  VERSION 3.5.4
  URL https://github.com/catchorg/Catch2/archive/refs/tags/v3.5.4.tar.gz
  URL_HASH MD5=d6e53cc0ce7fa70205e0c716aff258a8
  OVERRIDE_FIND_PACKAGE
)

FetchContent_MakeAvailable(catch)