include(CPM)
CPMAddPackage(
  NAME json
  VERSION 3.12.0
  URL https://github.com/nlohmann/json/archive/refs/tags/v3.12.0.tar.gz
  URL_HASH SHA256=4b92eb0c06d10683f7447ce9406cb97cd4b453be18d7279320f7b2f025c10187
  OPTIONS
    "JSON_MultipleHeaders ON"
    "JSON_BuildTests OFF"
)
