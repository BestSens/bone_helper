if(NOT SSL_DIR)
	message(FATAL_ERROR "If you do not use system ssl SSL_DIR needs to be specified" )
endif()

add_library(ssl STATIC IMPORTED)
set_property(TARGET ssl PROPERTY IMPORTED_LOCATION ${SSL_DIR}/lib/libssl.a)
target_include_directories(ssl INTERFACE ${SSL_DIR}/include)

add_library(crypto STATIC IMPORTED)
set_property(TARGET crypto PROPERTY IMPORTED_LOCATION ${SSL_DIR}/lib/libcrypto.a)
target_include_directories(crypto INTERFACE ${SSL_DIR}/include)