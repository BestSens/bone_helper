include(CPM)
CPMAddPackage(
  NAME openssl
  VERSION 1.1.1n
  URL https://github.com/janbar/openssl-cmake/archive/refs/tags/1.1.1n-20220327.tar.gz
  URL_HASH MD5=66e1540ee127ee15feb3aaa85ea4673b
  OVERRIDE_FIND_PACKAGE
)

find_package(openssl)