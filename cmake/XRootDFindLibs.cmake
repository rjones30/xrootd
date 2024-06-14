#-------------------------------------------------------------------------------
# Find the required libraries
#-------------------------------------------------------------------------------
find_package( ZLIB REQUIRED)

find_package( LibUuid REQUIRED )

if( ENABLE_READLINE )
  find_package( Readline )
  if( READLINE_FOUND )
    add_definitions( -DHAVE_READLINE )
  else()
    set( READLINE_LIBRARY "" )
    set( NCURSES_LIBRARY "" )
  endif()
endif()

if( ZLIB_FOUND )
  add_definitions( -DHAVE_LIBZ )
endif()

find_package( LibXml2 )
if( LIBXML2_FOUND )
  add_definitions( -DHAVE_XML2 )
endif()

find_package( Systemd )
if( SYSTEMD_FOUND )
  add_definitions( -DHAVE_SYSTEMD )
endif()

find_package( CURL )
if("${CURL_LIBRARIES}" STREQUAL "")
  find_library(CURL_LIBRARIES curl_static
               HINTS "${CMAKE_INSTALL_PREFIX}/lib" "${CMAKE_INSTALL_PREFIX}/lib64")
  if(NOT EXISTS ${CURL_LIBRARIES})
    find_library(CURL_LIBRARIES curl
                 HINTS "${CMAKE_INSTALL_PREFIX}/lib" "${CMAKE_INSTALL_PREFIX}/lib64")
  endif()
endif()

if(APPLE)
  find_library(COREFOUNDATION_FRAMEWORK CoreFoundation)
  find_library(SYSTEMCONFIGURATION_FRAMEWORK SystemConfiguration)
  find_library(SECURITY_FRAMEWORK Security)
  find_library(SSH2_LIBRARY ssh2)
  find_library(ZLIB_LIBRARY z)
  set(CURL_LIBRARIES ${COREFOUNDATION_FRAMEWORK}
	             ${SYSTEMCONFIGURATION_FRAMEWORK}
		     ${SECURITY_FRAMEWORK}
		     ${CURL_LIBRARIES}
		     ${SSH2_LIBRARY}
		     ${ZLIB_LIBRARY})
endif()

message("CURL_LIBRARIES=${CURL_LIBRARIES}")

execute_process(COMMAND xcode-select --print-path
                ECHO_OUTPUT_VARIABLE XCODE_PATH
		ECHO_ERROR_VARIABLE XCODE_PATH_ERROR)
message("XCODE_PATH is now ${XCODE_PATH}")
find_path(STDIO_H_PATH stdio.h
          HINTS "${XCODE_PATH}/usr/include")
message("STDIO_H_PATH is ${STDIO_H_PATH}")

if( ENABLE_CRYPTO )
  find_package( OpenSSL )
  if( OPENSSL_FOUND )
    add_definitions( -DHAVE_XRDCRYPTO )
    add_definitions( -DHAVE_SSL )
    set( BUILD_CRYPTO TRUE )
  else()
    set( BUILD_CRYPTO FALSE )
  endif()
endif()

if( ENABLE_KRB5 )
  find_package( Kerberos5 )
  if( KERBEROS5_FOUND )
    set( BUILD_KRB5 TRUE )
  else()
    set( BUILD_KRB5 FALSE )
  endif()
endif()

# mac fuse not supported
if( ENABLE_FUSE AND Linux )
  find_package( fuse )
  if( FUSE_FOUND )
    add_definitions( -DHAVE_FUSE )
    set( BUILD_FUSE TRUE )
  else()
    set( BUILD_FUSE FALSE )
  endif()
endif()

if( ENABLE_TESTS )
  find_package( CPPUnit )
  if( CPPUNIT_FOUND )
    set( BUILD_TESTS TRUE )
  else()
    set( BUILD_TESTS FALSE )
  endif()
endif()

if( ENABLE_HTTP )
  if( OPENSSL_FOUND AND BUILD_CRYPTO )
    set( BUILD_HTTP TRUE )
    if( CURL_FOUND )
      set( BUILD_TPC TRUE )
    else()
      set( BUILD_TPC FALSE )
    endif()
  else()
    set( BUILD_HTTP FALSE )
    set( BUILD_TPC FALSE )
  endif()
endif()

if( BUILD_TPC )
set ( CMAKE_REQUIRED_LIBRARIES ${CURL_LIBRARIES} )
check_function_exists( curl_multi_wait HAVE_CURL_MULTI_WAIT )
compiler_define_if_found( HAVE_CURL_MULTI_WAIT HAVE_CURL_MULTI_WAIT )
endif()
unset ( CMAKE_REQUIRED_LIBRARIES )

find_package( Macaroons )
include (FindPkgConfig)
pkg_check_modules(JSON json-c)
pkg_check_modules(UUID uuid)

if( MACAROONS_FOUND AND JSON_FOUND AND UUID_FOUND )
  set( BUILD_MACAROONS TRUE )
else()
  set( BUILD_MACAROONS FALSE )
endif()

if( ENABLE_PYTHON AND (Linux OR APPLE) )
  find_package( PythonInterp ${XRD_PYTHON_REQ_VERSION} )
  find_package( PythonLibs ${XRD_PYTHON_REQ_VERSION} )
  if( PYTHONINTERP_FOUND AND PYTHONLIBS_FOUND )
    set( BUILD_PYTHON TRUE )
    set( PYTHON_FOUND TRUE )
  else()
    set( BUILD_PYTHON FALSE )
  endif()
endif()

if( ENABLE_VOMS AND Linux )
  find_package( VOMS )
  if( VOMS_FOUND )
    set( BUILD_VOMS TRUE )
  else()
    set( BUILD_VOMS FALSE )
  endif()
endif()
