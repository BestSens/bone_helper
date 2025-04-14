include(CPM)
CPMAddPackage(
  NAME inja
  VERSION 3.4
  OPTIONS
    "INJA_USE_EMBEDDED_JSON OFF"
    "BUILD_TESTING OFF"
    "BUILD_BENCHMARK OFF"
  URL https://github.com/pantor/inja/archive/refs/tags/v3.4.0.tar.gz
  URL_HASH MD5=ccb2e98630334f94faa6781d4c54a1ff
)
