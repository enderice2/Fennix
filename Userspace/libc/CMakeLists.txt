cmake_minimum_required(VERSION 3.10)
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)
project(FennixCLibrary VERSION 0.0.1)

if(NOT DEFINED ENV{WORKSPACE_DIR})
  set(STANDALONE_BUILD ON)
  message(STATUS "Compiling standalone")

  if(NOT DEFINED ENV{CMAKE_INSTALL_PREFIX})
    set(CMAKE_INSTALL_PREFIX "/")
    message(STATUS "Using default install prefix: /")
  else()
    message(STATUS "Using custom install prefix: $ENV{CMAKE_INSTALL_PREFIX}")
  endif()

else()
  set(STANDALONE_BUILD OFF)
  message(STATUS "Compiling within workspace")
  if(DEFINED ENV{QUIET_BUILD})
    message(STATUS "Quiet build enabled")
    set(CMAKE_INSTALL_MESSAGE NEVER)
    set(CMAKE_MESSAGE_LOG_LEVEL "WARNING")
    set(MAKEFLAGS "--no-print-directory")
    set(CMAKE_VERBOSE_MAKEFILE OFF)
    set_property(GLOBAL PROPERTY RULE_MESSAGES OFF)
  endif()

  if(NOT DEFINED ENV{CMAKE_INSTALL_PREFIX})
    set(CMAKE_INSTALL_PREFIX "$ENV{WORKSPACE_DIR}/out/")
    message(STATUS "Using default install prefix: $ENV{WORKSPACE_DIR}/out/")
  else()
    message(STATUS "Using custom install prefix: $ENV{CMAKE_INSTALL_PREFIX}")
  endif()

  try_compile(
    WORKSPACE_TEST
    ${CMAKE_BINARY_DIR}
    ${CMAKE_SOURCE_DIR}/workspace_test.c
    CMAKE_FLAGS "-DCMAKE_C_COMPILER=$ENV{CC} -DCMAKE_CXX_COMPILER=$ENV{CXX} -DCMAKE_ASM_COMPILER=$ENV{AS} -DCMAKE_AR=$ENV{AR} -DCMAKE_LINKER=$ENV{LD}"
    OUTPUT_VARIABLE OUTPUT
  )

  if(NOT WORKSPACE_TEST)
    message(FATAL_ERROR "Workspace test failed: ${OUTPUT}")
  else()
    message(STATUS "Workspace test passed")
  endif()

endif()

if(NOT DEFINED TARGET_OS)
  if("${CMAKE_SYSTEM_NAME}" STREQUAL "Linux")
    set(TARGET_OS "linux")
  elseif("${CMAKE_SYSTEM_NAME}" STREQUAL "Fennix")
    set(TARGET_OS "fennix")
  else()
    message(FATAL_ERROR "Unsupported OS: ${CMAKE_SYSTEM_NAME}")
  endif()
endif()

if(NOT DEFINED TARGET_ARCH)
  set(TARGET_ARCH ${CMAKE_SYSTEM_PROCESSOR})
endif()

message(STATUS "Building for ${TARGET_OS}/${TARGET_ARCH}")

if(DEFINED ENV{CC})
  set(CMAKE_C_COMPILER "$ENV{CC}")
endif()
if(DEFINED ENV{CXX})
  set(CMAKE_CXX_COMPILER "$ENV{CXX}")
endif()
if(DEFINED ENV{AS})
  set(CMAKE_ASM_COMPILER "$ENV{AS}")
endif()
if(DEFINED ENV{AR})
  set(CMAKE_AR "$ENV{AR}")
endif()
if(DEFINED ENV{LD})
  set(CMAKE_LINKER "$ENV{LD}")
endif()

if(DEFINED ENV{DEBUG} AND "$ENV{DEBUG}" STREQUAL "1")
  set(CMAKE_C_FLAGS "-ggdb3 -O0 -DDEBUG")
else()
  set(CMAKE_C_FLAGS "-O2")
endif()

set(SYSDEPS_DIR ${CMAKE_SOURCE_DIR}/sysdeps)
set(ABIS_DIR ${CMAKE_SOURCE_DIR}/abis)

set(SYSDEPS_GENERIC ${SYSDEPS_DIR}/${TARGET_OS}/generic)
set(SYSDEPS_PATH ${SYSDEPS_DIR}/${TARGET_OS}/${TARGET_ARCH})
set(ABIS_GENERIC ${ABIS_DIR}/${TARGET_OS}/generic)
set(ABIS_PATH ${ABIS_DIR}/${TARGET_OS}/${TARGET_ARCH})

if(NOT EXISTS ${SYSDEPS_PATH} AND NOT EXISTS ${SYSDEPS_GENERIC})
  message(FATAL_ERROR "Missing sysdeps for ${TARGET_OS}: ${SYSDEPS_PATH} or ${SYSDEPS_GENERIC}")
endif()

if(NOT EXISTS ${ABIS_PATH} AND NOT EXISTS ${ABIS_GENERIC})
  message(FATAL_ERROR "Missing abis for ${TARGET_OS}: ${ABIS_PATH} or ${ABIS_GENERIC}")
endif()

message(STATUS "Using sysdeps from: ${SYSDEPS_GENERIC} and ${SYSDEPS_PATH}")
message(STATUS "Using abis from: ${ABIS_GENERIC} and ${ABIS_PATH}")

include_directories(${ABIS_GENERIC} ${ABIS_PATH})
include_directories(${CMAKE_SOURCE_DIR}/include)

add_subdirectory(runtime)
add_subdirectory(interpreter)
add_subdirectory(src)
add_subdirectory(libs)

if(DEFINED ENV{WORKSPACE_DIR})
  set(INC_OUTPUT "inc")
else()
  set(INC_OUTPUT "include")
endif()

install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/
  DESTINATION ${CMAKE_INSTALL_PREFIX}/${INC_OUTPUT}
  FILES_MATCHING
  PATTERN "*")

install(DIRECTORY ${ABIS_GENERIC}/
  DESTINATION ${CMAKE_INSTALL_PREFIX}/${INC_OUTPUT}
  FILES_MATCHING
  PATTERN "*")

install(DIRECTORY ${ABIS_PATH}/
  DESTINATION ${CMAKE_INSTALL_PREFIX}/${INC_OUTPUT}
  FILES_MATCHING
  PATTERN "*")

if(TARGET_OS STREQUAL "Linux")
  install(CODE "execute_process(
  COMMAND ${CMAKE_COMMAND} -E create_symlink 
          /lib/ld.so 
          \"\$ENV{DESTDIR}\${CMAKE_INSTALL_PREFIX}/lib/ld-linux-x86-64.so.2\"
)")
  install(CODE "execute_process(
  COMMAND ${CMAKE_COMMAND} -E create_symlink 
          /lib/ld.so 
          \"\$ENV{DESTDIR}\${CMAKE_INSTALL_PREFIX}/lib64/ld-linux-x86-64.so.2\"
)")
  install(CODE "execute_process(
  COMMAND ${CMAKE_COMMAND} -E create_symlink 
          /lib/libc.so 
          \"\$ENV{DESTDIR}\${CMAKE_INSTALL_PREFIX}/lib/libc.so.6\"
)")
  install(CODE "execute_process(
  COMMAND ${CMAKE_COMMAND} -E create_symlink 
          /lib/libc.so 
          \"\$ENV{DESTDIR}\${CMAKE_INSTALL_PREFIX}/lib64/libc.so.6\"
)")
endif()
