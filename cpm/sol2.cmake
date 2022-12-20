include(CPM)
CPMAddPackage(
  NAME sol2
  VERSION 3.3.0
  URL https://github.com/ThePhD/sol2/archive/refs/tags/v3.3.0.tar.gz
  URL_HASH MD5=05021725f7a3e0b91e19250d001deb8e
  OVERRIDE_FIND_PACKAGE
)

find_package(sol2)