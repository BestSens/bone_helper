include(CPM)
CPMAddPackage(
  NAME fmt
  VERSION 9.1.0
  URL https://github.com/fmtlib/fmt/releases/download/9.1.0/fmt-9.1.0.zip
  URL_HASH MD5=6133244fe8ef6f75c5601e8069b37b04
  OVERRIDE_FIND_PACKAGE
)

find_package(fmt)