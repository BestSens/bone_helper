include(CPM)
CPMAddPackage(
  NAME json
  VERSION 3.11.2
  URL https://github.com/nlohmann/json/archive/refs/tags/v3.11.2.tar.gz
  OPTIONS
    "JSON_MultipleHeaders ON"
    "JSON_BuildTests OFF"
  URL_HASH MD5=e8d56bc54621037842ee9f0aeae27746
  OVERRIDE_FIND_PACKAGE
)

find_package(json)