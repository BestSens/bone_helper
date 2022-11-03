include(FetchContent)
FetchContent_Declare(
  mbedtls
  URL https://github.com/Mbed-TLS/mbedtls/archive/refs/tags/v2.28.1.tar.gz
  URL_HASH MD5=c546ba363a39ed7baec698759fcbc199
  OVERRIDE_FIND_PACKAGE
)

set(ENABLE_TESTING OFF CACHE INTERNAL "Build mbed TLS tests.")
set(ENABLE_PROGRAMS OFF CACHE INTERNAL "Build mbed TLS programs.")
set(MBEDTLS_FATAL_WARNINGS OFF CACHE INTERNAL "Compiler warnings treated as errors")

FetchContent_MakeAvailable(mbedtls)