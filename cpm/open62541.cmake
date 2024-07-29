include(CPM)
CPMAddPackage(
  NAME open62541
  VERSION 1.4.3
  URL https://github.com/open62541/open62541/archive/refs/tags/v1.4.3.zip
  URL_HASH MD5=26ecfb9bee7ade3ecf6d958cbaa61d73
  OVERRIDE_FIND_PACKAGE
  OPTIONS
  	"UA_MULTITHREADING 100"
	  "UA_ENABLE_DISCOVERY ON"
	  "UA_ENABLE_ENCRYPTION OPENSSL"
)

find_package(open62541)