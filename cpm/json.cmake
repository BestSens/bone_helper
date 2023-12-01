include(CPM)
CPMAddPackage(
  NAME json
  VERSION 3.11.3
  URL https://github.com/nlohmann/json/archive/refs/tags/v3.11.3.tar.gz
  OPTIONS
    "JSON_MultipleHeaders ON"
    "JSON_BuildTests OFF"
  URL_HASH MD5=39da39c312501a041def772380e30d1ae1837065
  OVERRIDE_FIND_PACKAGE
)

find_package(json)