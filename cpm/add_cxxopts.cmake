include(CPM)
CPMAddPackage(
  NAME cxxopts
  VERSION 3.2.0
  OPTIONS
    "CXXOPTS_BUILD_EXAMPLES OFF"
    "CXXOPTS_BUILD_TESTS OFF"
  URL https://github.com/jarro2783/cxxopts/archive/refs/tags/v3.2.0.tar.gz
  URL_HASH SHA256=9f43fa972532e5df6c5fd5ad0f5bac606cdec541ccaf1732463d8070bbb7f03b
)
