include(CPM)
CPMAddPackage(
  NAME fmt
  VERSION 10.1.1
  URL https://github.com/fmtlib/fmt/releases/download/10.1.1/fmt-10.1.1.zip
  URL_HASH MD5=5b74fd3cfa02058855379da416940efe
  OVERRIDE_FIND_PACKAGE
)

find_package(fmt)