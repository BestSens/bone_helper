include(CPM)
CPMAddPackage(
  NAME fmt
  VERSION 10.2.1
  URL https://github.com/fmtlib/fmt/releases/download/10.2.1/fmt-10.2.1.zip
  URL_HASH MD5=04e266ad52659480d593486a17eed804
  OVERRIDE_FIND_PACKAGE
)

find_package(fmt)