set(ENABLE_TESTING OFF CACHE INTERNAL "Build mbed TLS tests.")
set(ENABLE_PROGRAMS OFF CACHE INTERNAL "Build mbed TLS programs.")
add_subdirectory(libs/mbedtls)

set(MBEDTLS_INCLUDE_DIRS "${CMAKE_SOURCE_DIR}/libs/mbedtls/include")
set(MBEDTLS_LIBRARY "${CMAKE_BINARY_DIR}/libs/mbedtls/library/libmbedtls.a")
set(MBEDX509_LIBRARY "${CMAKE_BINARY_DIR}/libs/mbedtls/library/libmbedcrypto.a")
set(MBEDCRYPTO_LIBRARY "${CMAKE_BINARY_DIR}/libs/mbedtls/library/libmbedx509.a")