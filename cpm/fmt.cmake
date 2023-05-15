include(CPM)
CPMAddPackage(
  NAME fmt
  VERSION 10.0.0
  URL https://github.com/fmtlib/fmt/releases/download/10.0.0/fmt-10.0.0.zip
  URL_HASH MD5=bfd28e60b354d77f8c791912863b4e1e
  OVERRIDE_FIND_PACKAGE
)

find_package(fmt)