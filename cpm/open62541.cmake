include(CPM)
CPMAddPackage(
  NAME open62541
  VERSION 1.4.6
  URL https://github.com/open62541/open62541/archive/refs/tags/v1.4.6.zip
  URL_HASH MD5=c73a678e6c0b9fe63235d0ba1ae69494
  OVERRIDE_FIND_PACKAGE
  OPTIONS
  	"UA_MULTITHREADING 100"
	  "UA_ENABLE_DISCOVERY ON"
	  "UA_ENABLE_ENCRYPTION OPENSSL"
)

find_package(open62541)