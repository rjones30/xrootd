#-------------------------------------------------------------------------------
# Define the OS variables
#-------------------------------------------------------------------------------

include( CheckCXXSourceRuns )

set( Linux    FALSE CACHE BOOL "no default target platform")
set( MacOSX   FALSE CACHE BOOL "no default target platform")
set( Solaris  FALSE CACHE BOOL "no default target platform")

set( XrdClPipelines FALSE CACHE BOOL "disable use of xrootd client pipelines")

add_definitions( -D_LARGEFILE_SOURCE -D_LARGEFILE64_SOURCE -D_FILE_OFFSET_BITS=64 )
set( LIBRARY_PATH_PREFIX "lib" CACHE STRING "Library path prefix")

if( NOT DEFINED USE_LIBC_SEMAPHORE )
  set(USE_LIBC_SEMAPHORE 1 CACHE BOOL "use semaphores provided by libc")
endif()
add_definitions( -DUSE_LIBC_SEMAPHORE=${USE_LIBC_SEMAPHORE} )

#-------------------------------------------------------------------------------
# Enable c++0x / c++11
#-------------------------------------------------------------------------------
set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x -DOPENSSL_NO_FILENAMES" )

#-------------------------------------------------------------------------------
# GCC
#-------------------------------------------------------------------------------
if( CMAKE_COMPILER_IS_GNUCXX )
  set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x" )
  set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra" )
  #-----------------------------------------------------------------------------
  # Set -Werror only for Debug (or undefined) build type or if we have been
  # explicitly asked to do so
  #-----------------------------------------------------------------------------
  if( ( CMAKE_BUILD_TYPE STREQUAL "Debug" OR "${CMAKE_BUILD_TYPE}" STREQUAL ""
        OR FORCE_WERROR ) )
    set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Werror" )
  endif()
  set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-unused-parameter" )
  # gcc 4.1 is retarded
  execute_process( COMMAND ${CMAKE_C_COMPILER} -dumpversion
                   OUTPUT_VARIABLE GCC_VERSION )
  if( (GCC_VERSION VERSION_GREATER 4.1 OR GCC_VERSION VERSION_EQUAL 4.1)
      AND GCC_VERSION VERSION_LESS 4.2 )
    set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-strict-aliasing" )
  endif()

  # for 4.9.3 or greater the 'omit-frame-pointer' 
  # interfears  with custom semaphore implementation
  if( (GCC_VERSION VERSION_GREATER 4.9.2) AND (USE_LIBC_SEMAPHORE EQUAL 0) )
    set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-omit-frame-pointer" )
  endif()
  
  if( GCC_VERSION VERSION_GREATER 4.8.0 )
    set( XrdClPipelines TRUE CACHE BOOL "use xrootd client pipelines")
  endif()

  # gcc 6.0 is more pedantic
  if( GCC_VERSION VERSION_GREATER 6.0 OR GCC_VERSION VERSION_EQUAL 6.0 )
    set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-error=misleading-indentation" )
  endif()
  
  # gcc 9.0 is even more pedantic
  if( GCC_VERSION VERSION_GREATER 9.0 OR GCC_VERSION VERSION_EQUAL 9.0 )
      set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-error=address-of-packed-member" )
      #-------------------------------------------------------------------------
      # Althought new compilers are obliged to do copy elision on return 
      # statement old ones are not, so we turn off the warning. 
      #-------------------------------------------------------------------------
      set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-error=pessimizing-move" )
  endif()
  
endif()

#-------------------------------------------------------------------------------
# Linux
#-------------------------------------------------------------------------------
if( ${CMAKE_SYSTEM_NAME} STREQUAL "Linux" )
  set( Linux TRUE CACHE BOOL "target platform is Linux")
  include( GNUInstallDirs )
  add_definitions( -D__linux__=1 )
  set( EXTRA_LIBS rt CACHE STRING "Extra platform-specific libraries for linker")
endif()

#-------------------------------------------------------------------------------
# MacOSX
#-------------------------------------------------------------------------------
if( APPLE )
  set( MacOSX TRUE CACHE BOOL "target platform is MacOS")
  
  if( NOT DEFINED CMAKE_MACOSX_RPATH )
    set( CMAKE_MACOSX_RPATH 1 )
  endif()

  # this is here because of Apple deprecating openssl and krb5
  set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-deprecated-declarations" )

  add_definitions( -D__macos__=1 )
  add_definitions( -DLT_MODULE_EXT=".dylib" )
  set( CMAKE_INSTALL_LIBDIR "lib" )
  set( CMAKE_INSTALL_BINDIR "bin" )
  set( CMAKE_INSTALL_MANDIR "share/man" )
  set( CMAKE_INSTALL_INCLUDEDIR "include" )
  set( CMAKE_INSTALL_DATADIR "share" )
endif()

#-------------------------------------------------------------------------------
# FreeBSD
#-------------------------------------------------------------------------------
if( ${CMAKE_SYSTEM_NAME} STREQUAL "FreeBSD" )
  set( CMAKE_INSTALL_LIBDIR "lib" )
  set( CMAKE_INSTALL_BINDIR "bin" )
  set( CMAKE_INSTALL_MANDIR "man" )
  set( CMAKE_INSTALL_INCLUDEDIR "include" )
  set( CMAKE_INSTALL_DATADIR "share" )
endif()

#-------------------------------------------------------------------------------
# Solaris
#-------------------------------------------------------------------------------
if( ${CMAKE_SYSTEM_NAME} STREQUAL "SunOS" )
  define_default( FORCE_32BITS FALSE )
  set( CMAKE_INSTALL_LIBDIR "lib" )
  set( CMAKE_INSTALL_BINDIR "bin" )
  set( CMAKE_INSTALL_MANDIR "man" )
  set( CMAKE_INSTALL_INCLUDEDIR "include" )
  set( CMAKE_INSTALL_DATADIR "share" )
  set( Solaris TRUE CACHE BOOL "target platform is Solaris")
  add_definitions( -D__solaris__=1 )
  add_definitions( -DSUNCC -D_REENTRANT -D_POSIX_PTHREAD_SEMANTICS )
  set( EXTRA_LIBS rt Crun Cstd CACHE STRING "Extra platform-specific libraries for linker")

  set( CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -fast" )
  set( CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} -fast" )

  define_solaris_flavor()

  #-----------------------------------------------------------------------------
  # Define solaris version
  #-----------------------------------------------------------------------------
  execute_process( COMMAND uname -r
                   OUTPUT_VARIABLE SOLARIS_VER )
  string( REPLACE "." ";" SOLARIS_VER_LIST ${SOLARIS_VER} )
  list( GET SOLARIS_VER_LIST 1 SOLARIS_VERSION )
  string( REPLACE "\n" "" SOLARIS_VERSION ${SOLARIS_VERSION} )
  add_definitions( -DSOLARIS_VERSION=${SOLARIS_VERSION} )

  #-----------------------------------------------------------------------------
  # AMD64 (opteron)
  #-----------------------------------------------------------------------------
  if( ${SOLARIS_VERSION} STREQUAL "10" AND SOLARIS_AMD64 AND NOT FORCE_32BITS )
    set( CMAKE_CXX_FLAGS " -m64 -xtarget=opteron -xs ${CMAKE_CXX_FLAGS} " )
    set( CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -G" )
    set( CMAKE_LIBRARY_PATH "/lib/64;/usr/lib/64" )
    add_definitions( -DSUNX86 )
    set( LIB_SEARCH_OPTIONS NO_DEFAULT_PATH CACHE STRING "library search options")
    set( LIBRARY_PATH_PREFIX "lib/64" CACHE STRING "Library path prefix")
  endif()

  #-----------------------------------------------------------------------------
  # Check if the SunCC compiler can do optimizations
  #-----------------------------------------------------------------------------
  check_cxx_source_runs(
  "
    int main()
    {
      #if __SUNPRO_CC > 0x5100
      return 0;
      #else
      return 1;
      #endif
    }
  "
  SUNCC_CAN_DO_OPTS )

endif()
