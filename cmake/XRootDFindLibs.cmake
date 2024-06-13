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
    set( READLINE_LIBRARY "" CACHE STRING "location of the readline library")
    set( NCURSES_LIBRARY "" CACHE STRING "location of the ncurses library")
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
               HINTS "${CMAKE_BINARY_PREFIX}/lib" "${CMAKE_BINARY_PREFIX}/lib64")
  if("${CURL_LIBRARIES}" STREQUAL "")
    find_library(CURL_LIBRARIES curl
                 HINTS "${CMAKE_BINARY_PREFIX}/lib" "${CMAKE_BINARY_PREFIX}/lib64")
  endif()
endif()
message("find_package(CURL) returned CURL_LIBRARIES=${CURL_LIBRARIES}")

if( ENABLE_CRYPTO )
  find_package( OpenSSL )
  if( OPENSSL_FOUND )
    add_definitions( -DHAVE_XRDCRYPTO )
    add_definitions( -DHAVE_SSL )
    set( BUILD_CRYPTO TRUE CACHE BOOL "build with crypto library")
  else()
    set( BUILD_CRYPTO FALSE CACHE BOOL "build with crypto library")
  endif()
endif()

if( ENABLE_KRB5 )
  find_package( Kerberos5 )
  if( KERBEROS5_FOUND )
    set( BUILD_KRB5 TRUE CACHE BOOL "build with kerberos5")
  else()
    set( BUILD_KRB5 FALSE CACHE BOOL "build with kerberos5")
  endif()
endif()

# mac fuse not supported
if( ENABLE_FUSE AND Linux )
  find_package( fuse )
  if( FUSE_FOUND )
    add_definitions( -DHAVE_FUSE )
    set( BUILD_FUSE TRUE CACHE BOOL "build with fuse library")
  else()
    set( BUILD_FUSE FALSE CACHE BOOL "build with fuse library")
  endif()
endif()

if( ENABLE_TESTS )
  find_package( CPPUnit )
  if( CPPUNIT_FOUND )
    set( BUILD_TESTS TRUE CACHE BOOL "build tests")
  else()
    set( BUILD_TESTS FALSE CACHE BOOL "build tests")
  endif()
endif()

if( ENABLE_HTTP )
  if( OPENSSL_FOUND AND BUILD_CRYPTO )
    set( BUILD_HTTP TRUE CACHE BOOL "build with http support")
    if( CURL_FOUND )
      set( BUILD_TPC TRUE CACHE BOOL "build with 3rd party copy support")
    else()
      set( BUILD_TPC FALSE CACHE BOOL "build with 3rd party copy support")
    endif()
  else()
    set( BUILD_HTTP FALSE CACHE BOOL "build with http support")
    set( BUILD_TPC FALSE CACHE BOOL "build with 3rd party copy support")
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
  set( BUILD_MACAROONS TRUE CACHE BOOL "build with macaroons support")
else()
  set( BUILD_MACAROONS FALSE CACHE BOOL "build with macaroons suppoert")
endif()

if( ENABLE_PYTHON AND (Linux OR APPLE) )
  find_package( PythonInterp ${XRD_PYTHON_REQ_VERSION} )
  find_package( PythonLibs ${XRD_PYTHON_REQ_VERSION} )
  if( PYTHONINTERP_FOUND AND PYTHONLIBS_FOUND )
    set( BUILD_PYTHON TRUE CACHE BOOL "build with python support")
    set( PYTHON_FOUND TRUE CACHE BOOL "has python installed")
  else()
    set( BUILD_PYTHON FALSE CACHE BOOL "build with python support")
  endif()
endif()

if( ENABLE_VOMS AND Linux )
  find_package( VOMS )
  if( VOMS_FOUND )
    set( BUILD_VOMS TRUE CACHE BOOL "build with voms support")
  else()
    set( BUILD_VOMS FALSE CACHE BOOL "build with voms support")
  endif()
endif()
