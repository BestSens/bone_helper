include(CPM)
CPMAddPackage(
  NAME openssl
  VERSION 1.1.1n
  URL https://github.com/janbar/openssl-cmake.git
  OVERRIDE_FIND_PACKAGE
)

find_package(openssl)