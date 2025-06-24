include(CPM)
CPMAddPackage(
	NAME paho_mqtt_c
	VERSION 1.3.14
	URL https://github.com/eclipse/paho.mqtt.c/archive/refs/tags/v1.3.14.tar.gz
	URL_HASH SHA256=7af7d906e60a696a80f1b7c2bd7d6eb164aaad908ff4c40c3332ac2006d07346
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

set(paho_mqtt_c_LIBRARIES "${paho_mqtt_c_BINARY_DIR}/src/libpaho-mqtt3as.a")
set(paho_mqtt_c_INCLUDE_DIRS "${paho_mqtt_c_SOURCE_DIR}/src")

CPMAddPackage(
	NAME paho_mqttpp
	VERSION 1.3.2
	GIT_REPOSITORY https://github.com/eclipse/paho.mqtt.cpp.git
	GIT_TAG v1.3.2
	OPTIONS
		"PAHO_WITH_SSL ON"
		"PAHO_BUILD_STATIC ON"
		"PAHO_BUILD_SHARED OFF"
		"PAHO_ENABLE_TESTING OFF"
		"PAHO_BUILD_SAMPLES OFF"
		"PAHO_BUILD_DOCUMENTATION OFF"
		"PAHO_MQTT_C_LIBRARIES ${paho_mqtt_c_LIBRARIES}"
		"PAHO_MQTT_C_INCLUDE_DIRS ${paho_mqtt_c_INCLUDE_DIRS}"
)

set(paho_mqttpp_LIBRARIES "${paho_mqttpp_BINARY_DIR}/src/libpaho-mqttpp3.a")
set(paho_mqttpp_INCLUDE_DIRS "${paho_mqttpp_SOURCE_DIR}/src")
