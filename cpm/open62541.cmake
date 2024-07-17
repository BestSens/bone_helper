include(CPM)
CPMAddPackage(
  NAME open62541
  VERSION 1.4.2
  URL https://github.com/open62541/open62541/archive/refs/tags/v1.4.2.zip
  URL_HASH MD5=e963b05b75c6f3b9b5f8e27a863ed079
  OVERRIDE_FIND_PACKAGE
  OPTIONS
  	"UA_MULTITHREADING 100"
	  "UA_ENABLE_DISCOVERY ON"
	  "UA_ENABLE_ENCRYPTION OPENSSL"
)

find_package(open62541)