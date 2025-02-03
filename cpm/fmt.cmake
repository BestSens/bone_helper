include(CPM)
CPMAddPackage(
  NAME fmt
  VERSION 11.1.3
  URL https://github.com/fmtlib/fmt/releases/download/11.1.3/fmt-11.1.3.zip
  URL_HASH MD5=49b7bd807c42b4ce34e0b795a3ed1e9a
  OVERRIDE_FIND_PACKAGE
)

find_package(fmt)