set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR ARM)

if(MINGW OR CYGWIN OR WIN32)
    set(UTIL_SEARCH_CMD where)
elseif(UNIX OR APPLE)
    set(UTIL_SEARCH_CMD which)
endif()

set(TOOLCHAIN_PREFIX arm-poky-linux-gnueabi-)

execute_process(
  COMMAND ${UTIL_SEARCH_CMD} ${TOOLCHAIN_PREFIX}gcc
  OUTPUT_VARIABLE BINUTILS_PATH
  OUTPUT_STRIP_TRAILING_WHITESPACE
)

set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

set(CMAKE_C_COMPILER ${TOOLCHAIN_PREFIX}gcc)
set(CMAKE_ASM_COMPILER ${CMAKE_C_COMPILER})
set(CMAKE_CXX_COMPILER ${TOOLCHAIN_PREFIX}g++)

set(CMAKE_LINKER ${TOOLCHAIN_PREFIX}ld)

set(CMAKE_AR ${TOOLCHAIN_PREFIX}gcc-ar)
set(CMAKE_C_COMPILER_AR ${CMAKE_AR})
set(CMAKE_CXX_COMPILER_AR ${CMAKE_AR})
set(CMAKE_ASM_COMPILER_AR ${CMAKE_AR})

set(CMAKE_RANLIB ${TOOLCHAIN_PREFIX}gcc-ranlib)
set(CMAKE_C_COMPILER_RANLIB ${CMAKE_RANLIB})
set(CMAKE_CXX_COMPILER_RANLIB ${CMAKE_RANLIB})
set(CMAKE_ASM_COMPILER_RANLIB ${CMAKE_RANLIB})

set(CMAKE_NM ${TOOLCHAIN_PREFIX}gcc-nm)

set(CMAKE_OBJCOPY ${TOOLCHAIN_PREFIX}objcopy)
set(CMAKE_SIZE_UTIL ${TOOLCHAIN_PREFIX}size)

set(CMAKE_STRIP ${TOOLCHAIN_PREFIX}strip)

set(CMAKE_FIND_ROOT_PATH ${BINUTILS_PATH})
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

set(CMAKE_SYSROOT "/opt/boneos/2.0.3/sysroots/cortexa9hf-vfp-neon-poky-linux-gnueabi")
set(ENV{SDKTARGETSYSROOT} ${CMAKE_SYSROOT})
set(ENV{PKG_CONFIG_SYSROOT_DIR} ${CMAKE_SYSROOT})
set(ENV{PKG_CONFIG_PATH} ${CMAKE_SYSROOT}/usr/lib/pkgconfig)
set(CMAKE_MODULE_PATH ${CMAKE_SYSROOT}/usr/lib/cmake)

add_compile_options("-march=armv7-a;-mfloat-abi=hard;-mfpu=neon;-mtune=cortex-a9;-pthread")
add_link_options("-march=armv7-a;-mfloat-abi=hard;-mfpu=neon;-mtune=cortex-a9;-pthread")

set(SSL_DIR ${CMAKE_SYSROOT}/opt/openssl-3)
