include(CPM)
CPMAddPackage(
  NAME catch
  VERSION 3.8.1
  URL https://github.com/catchorg/Catch2/archive/refs/tags/v3.8.1.tar.gz
  URL_HASH SHA256=18b3f70ac80fccc340d8c6ff0f339b2ae64944782f8d2fca2bd705cf47cadb79
  OVERRIDE_FIND_PACKAGE
)

FetchContent_MakeAvailable(catch)