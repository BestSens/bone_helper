include(CPM)
CPMAddPackage(
  NAME open62541
  VERSION 1.4.8
  URL https://github.com/open62541/open62541/archive/refs/tags/v1.4.8.zip
  URL_HASH MD5=d781f5bf16ffb8fc8eb06705f99b7641
  OVERRIDE_FIND_PACKAGE
  OPTIONS
  	"UA_MULTITHREADING 100"
	  "UA_ENABLE_DISCOVERY ON"
	  "UA_ENABLE_ENCRYPTION OPENSSL"
)

find_package(open62541)