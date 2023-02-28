include(CPM)
CPMAddPackage(
	NAME paho_mqtt_c
	VERSION 1.3.1
	URL https://github.com/eclipse/paho.mqtt.c/archive/refs/tags/v1.3.12.tar.gz
	OPTIONS
		"PAHO_WITH_SSL ON"
		"PAHO_BUILD_SHARED OFF"
		"PAHO_BUILD_STATIC ON"
		"PAHO_ENABLE_TESTING OFF"
		"PAHO_BUILD_SAMPLES OFF"
		"PAHO_BUILD_DOCUMENTATION OFF"
		"OPENSSL_CRYPTO_LIBRARY ${OPENSSL_CRYPTO_LIBRARY}"
		"OPENSSL_SSL_LIBRARY ${OPENSSL_SSL_LIBRARY}"
		"CMAKE_CXX_STANDARD 11"
	URL_HASH MD5=b654db416d19931bd3362f26ea9433b3
)

set(paho_mqtt_c_LIBRARIES "${paho_mqtt_c_BINARY_DIR}/src/libpaho-mqtt3as.a")
set(paho_mqtt_c_INCLUDE_DIRS "${paho_mqtt_c_SOURCE_DIR}/src/")

CPMAddPackage(
	NAME paho_mqttpp
	VERSION 1.2.0
	URL https://github.com/eclipse/paho.mqtt.cpp/archive/refs/tags/v1.2.0.tar.gz
	OPTIONS
		"PAHO_WITH_SSL ON"
		"PAHO_BUILD_STATIC ON"
		"PAHO_BUILD_SHARED OFF"
		"PAHO_ENABLE_TESTING OFF"
		"PAHO_BUILD_SAMPLES OFF"
		"PAHO_BUILD_DOCUMENTATION OFF"
		"PAHO_MQTT_C_LIBRARIES ${paho_mqtt_c_LIBRARIES}"
		"PAHO_MQTT_C_INCLUDE_DIRS ${paho_mqtt_c_INCLUDE_DIRS}"
		"CMAKE_CXX_STANDARD 11"
	URL_HASH MD5=b654db416d19931bd3362f26ea9433b3
)

set(paho_mqttpp_LIBRARIES "${paho_mqttpp_BINARY_DIR}/src/libpaho-mqttpp3.a")
set(paho_mqttpp_INCLUDE_DIRS "${paho_mqttpp_SOURCE_DIR}/src/")
