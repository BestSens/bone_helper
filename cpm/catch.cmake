include(CPM)
CPMAddPackage(
  NAME catch
  VERSION 3.5.2
  URL https://github.com/catchorg/Catch2/archive/refs/tags/v3.5.2.tar.gz
  URL_HASH MD5=c22bc17311b08ae8338d80f096487765
  OVERRIDE_FIND_PACKAGE
)

FetchContent_MakeAvailable(catch)