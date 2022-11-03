include(CPM)
CPMAddPackage(
  NAME optional
  VERSION 1.0.0
  OPTIONS
    "OPTIONAL_ENABLE_TESTS OFF"
  URL https://github.com/TartanLlama/optional/archive/refs/tags/v1.0.0.tar.gz
  URL_HASH MD5=b654db416d19931bd3362f26ea9433b3
  OVERRIDE_FIND_PACKAGE
)

find_package(optional)