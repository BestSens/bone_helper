include(CPM)
CPMAddPackage(
  NAME fmt
  VERSION 11.0.1
  URL https://github.com/fmtlib/fmt/releases/download/11.0.1/fmt-11.0.1.zip
  URL_HASH MD5=5f3915e2eff60e7f70c558120592100d
  OVERRIDE_FIND_PACKAGE
)

find_package(fmt)