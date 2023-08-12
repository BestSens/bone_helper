include(CPM)
CPMAddPackage(
  NAME fmt
  VERSION 10.1.0
  URL https://github.com/fmtlib/fmt/releases/download/10.1.0/fmt-10.1.0.zip
  URL_HASH MD5=56eaffa1e2a2afe29e9cb2348a446e63
  OVERRIDE_FIND_PACKAGE
)

find_package(fmt)