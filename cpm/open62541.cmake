include(CPM)
CPMAddPackage(
  NAME open62541
  VERSION 1.3.9
  URL https://github.com/open62541/open62541/archive/refs/tags/v1.3.9.zip
  URL_HASH MD5=5778a4c3fbca52e8bdc94a7f3c6d0f63
  OVERRIDE_FIND_PACKAGE
  OPTIONS
  	"UA_MULTITHREADING 100"
	  "UA_ENABLE_DISCOVERY ON"
	  "UA_ENABLE_ENCRYPTION OPENSSL"
)

find_package(open62541)