include(CPM)
CPMAddPackage(
	NAME paho_mqttpp
	VERSION 1.5.3
	GIT_REPOSITORY https://github.com/eclipse/paho.mqtt.cpp.git
	GIT_TAG v1.5.3
	OPTIONS
		"PAHO_WITH_SSL ON"
		"PAHO_BUILD_STATIC ON"
		"PAHO_BUILD_SHARED OFF"
		"PAHO_ENABLE_TESTING OFF"
		"PAHO_BUILD_SAMPLES OFF"
		"PAHO_BUILD_DOCUMENTATION OFF"
		"PAHO_WITH_MQTT_C ON"
		"CMAKE_CXX_STANDARD 11"
)

set(paho_mqttpp_LIBRARIES "${paho_mqttpp_BINARY_DIR}/src/libpaho-mqttpp3.a")
set(paho_mqttpp_INCLUDE_DIRS "${paho_mqttpp_SOURCE_DIR}/include")
set(paho_mqtt_c_LIBRARIES "${paho_mqttpp_BINARY_DIR}/externals/paho-mqtt-c/src/libpaho-mqtt3a.a")
set(paho_mqtt_c_INCLUDE_DIRS "${paho_mqttpp_SOURCE_DIR}/externals/paho-mqtt-c/src")
