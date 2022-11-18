include(CPM)
CPMAddPackage(
  NAME sol2
  VERSION 2.20.6
  URL https://github.com/ThePhD/sol2/archive/refs/tags/v2.20.6.tar.gz
  URL_HASH MD5=f515a1e7aa65087ebcfd3c343f4b3b34
  OVERRIDE_FIND_PACKAGE
)

find_package(sol2)