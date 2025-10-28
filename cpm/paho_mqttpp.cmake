include(CPM)
CPMAddPackage(
	NAME eclipse-paho-mqtt-c
	VERSION 1.3.15
	URL https://github.com/eclipse/paho.mqtt.c/archive/refs/tags/v1.3.15.tar.gz
	URL_HASH SHA256=60ce2cfdc146fcb81c621cb8b45874d2eb1d4693105d048f60e31b8f3468be90
	OPTIONS
		"PAHO_WITH_SSL ON"
		"PAHO_BUILD_SHARED OFF"
		"PAHO_BUILD_STATIC ON"
		"PAHO_ENABLE_TESTING OFF"
		"PAHO_BUILD_SAMPLES OFF"
		"PAHO_BUILD_DOCUMENTATION OFF"
		"OPENSSL_CRYPTO_LIBRARY ${OPENSSL_CRYPTO_LIBRARY}"
		"OPENSSL_SSL_LIBRARY ${OPENSSL_SSL_LIBRARY}"
)

if(TARGET paho-mqtt3as-static)
    export(TARGETS paho-mqtt3as-static NAMESPACE eclipse-paho-mqtt-c:: FILE "${eclipse-paho-mqtt-c_BINARY_DIR}/PahoMqttCStaticTargets.cmake")
endif()

CPMAddPackage(
	NAME paho-mqttpp
	VERSION 1.5.3
	URL https://github.com/eclipse/paho.mqtt.cpp/archive/refs/tags/v1.5.3.tar.gz
	URL_HASH SHA256=8aab7761bcb43e2d65dbf266c8623d345f7612411363a97aa66370fb9822d0b9
	OPTIONS
		"PAHO_WITH_SSL ON"
		"PAHO_BUILD_STATIC ON"
		"PAHO_BUILD_SHARED OFF"
		"PAHO_ENABLE_TESTING OFF"
		"PAHO_BUILD_SAMPLES OFF"
		"PAHO_BUILD_DOCUMENTATION OFF"
)

# set(paho_mqttpp_LIBRARIES "${paho_mqttpp_BINARY_DIR}/src/libpaho-mqttpp3.a")
# set(paho_mqttpp_INCLUDE_DIRS "${paho_mqttpp_SOURCE_DIR}/include")
