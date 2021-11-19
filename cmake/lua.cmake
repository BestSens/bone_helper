cmake_minimum_required(VERSION 3.3)
include(ExternalProject)

if(DEFINED ENV{CC})
   set(LUA_CC $ENV{CC})
   message("using CC from environment: $ENV{CC}")
else()
   set(LUA_CC ${CMAKE_C_COMPILER})
endif()

if(DEFINED ENV{AR})
   set(LUA_AR $ENV{AR})
   message("using AR from environment: $ENV{AR}")
else()
   set(LUA_AR ${CMAKE_AR})
endif()

if(DEFINED ENV{RANLIB})
   set(LUA_RANLIB $ENV{RANLIB})
   message("using RANLIB from environment: $ENV{RANLIB}")
else()
   set(LUA_RANLIB ${CMAKE_C_COMPILER_RANLIB})
endif()

ExternalProject_Add(lua
   URL "https://www.lua.org/ftp/lua-5.3.6.tar.gz"
   URL_HASH SHA256=fc5fd69bb8736323f026672b1b7235da613d7177e72558893a0bdcd320466d60
   PATCH_COMMAND
      COMMAND sed -i "/^CC=/c\ CC=${LUA_CC} -std=gnu99" src/Makefile
      COMMAND sed -i "/^AR=/c\ AR=${LUA_AR} rcu" src/Makefile
      COMMAND sed -i "/^RANLIB=/c\ RANLIB=${LUA_RANLIB}" src/Makefile
   CONFIGURE_COMMAND ""
   BUILD_COMMAND 
      COMMAND $(MAKE) linux
   BUILD_ALWAYS true
   BUILD_IN_SOURCE true
   INSTALL_COMMAND ""
)
add_library(liblua STATIC IMPORTED)
ExternalProject_Get_property(lua SOURCE_DIR)
include_directories(${SOURCE_DIR})

set(lua_INCLUDE_DIRS "${SOURCE_DIR}/src")
set(lua_LIBRARIES "${SOURCE_DIR}/src/liblua.a")