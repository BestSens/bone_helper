include(CPM)
CPMAddPackage(
  NAME fmt
  VERSION 8.1.1
  URL https://github.com/fmtlib/fmt/releases/download/8.1.1/fmt-8.1.1.zip
  URL_HASH MD5=16dcd48ecc166f10162450bb28aabc87
  OVERRIDE_FIND_PACKAGE
)

find_package(fmt)