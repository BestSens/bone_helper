include(CPM)
CPMAddPackage(
  NAME fmt
  VERSION 9.1.0
  URL https://github.com/fmtlib/fmt/releases/download/9.1.0/fmt-9.1.0.zip
  URL_HASH MD5=16dcd48ecc166f10162450bb28aabc87
  OVERRIDE_FIND_PACKAGE
)

find_package(fmt)