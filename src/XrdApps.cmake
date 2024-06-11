
include( XRootDCommon )

#-------------------------------------------------------------------------------
# Modules
#-------------------------------------------------------------------------------
set( LIB_XRDCL_PROXY_PLUGIN XrdClProxyPlugin-${PLUGIN_VERSION} )

#-------------------------------------------------------------------------------
# Shared library version
#-------------------------------------------------------------------------------
set( XRD_APP_UTILS_VERSION   1.0.0 )
set( XRD_APP_UTILS_SOVERSION 1 )

#-------------------------------------------------------------------------------
# This part is NOT built for XrdCl only builds
#-------------------------------------------------------------------------------
if( NOT XRDCL_ONLY )

  #-----------------------------------------------------------------------------
  # xrdadler32
  #-----------------------------------------------------------------------------
  add_executable(
    xrdadler32
    XrdApps/Xrdadler32.cc )

  target_link_libraries(
    xrdadler32
    XrdPosix
    XrdUtils
    pthread
    ${ZLIB_LIBRARY} )

  #-----------------------------------------------------------------------------
  # cconfig
  #-----------------------------------------------------------------------------
  add_executable(
    cconfig
    XrdApps/XrdAppsCconfig.cc )

  target_link_libraries(
    cconfig
    XrdUtils )

  #-----------------------------------------------------------------------------
  # mpxstats
  #-----------------------------------------------------------------------------
  add_executable(
    mpxstats
    XrdApps/XrdMpxStats.cc )

  target_link_libraries(
    mpxstats
    XrdAppUtils
    XrdUtils
    ${EXTRA_LIBS}
    pthread
    ${SOCKET_LIBRARY} )

  #-----------------------------------------------------------------------------
  # wait41
  #-----------------------------------------------------------------------------
  add_executable(
    wait41
    XrdApps/XrdWait41.cc )

  target_link_libraries(
    wait41
    XrdUtils
    pthread
    ${EXTRA_LIBS} )

  #-----------------------------------------------------------------------------
  # xrdacctest
  #-----------------------------------------------------------------------------
  add_executable(
    xrdacctest
    XrdApps/XrdAccTest.cc)

  target_link_libraries(
    xrdacctest
    XrdServer
    XrdUtils
    ${EXTRA_LIBS} )

  #-------------------------------------------------------------------------------
  # xrdmapc
  #-------------------------------------------------------------------------------
  add_executable(
    xrdmapc
    XrdApps/XrdMapCluster.cc )

  target_link_libraries(
    xrdmapc
    XrdCl
    XrdUtils )

  #-------------------------------------------------------------------------------
  # xrdqstats
  #-------------------------------------------------------------------------------
  add_executable(
    xrdqstats
    XrdApps/XrdQStats.cc )

  target_link_libraries(
    xrdqstats
    XrdCl
    XrdAppUtils
    XrdUtils
    ${EXTRA_LIBS} )

  #-------------------------------------------------------------------------------
  # xrdCp
  #-------------------------------------------------------------------------------
  add_executable(
    xrdcp-old
    XrdApps/XrdCpy.cc
    XrdClient/XrdcpXtremeRead.cc         XrdClient/XrdcpXtremeRead.hh
    XrdClient/XrdCpMthrQueue.cc          XrdClient/XrdCpMthrQueue.hh
    XrdClient/XrdCpWorkLst.cc            XrdClient/XrdCpWorkLst.hh )

  target_link_libraries(
    xrdcp-old
    XrdClient
    XrdUtils
    XrdAppUtils
    ${CMAKE_DL_LIBS}
    pthread
    ${EXTRA_LIBS} )
endif()

#-------------------------------------------------------------------------------
# AppUtils
#-------------------------------------------------------------------------------
add_library(
  XrdAppUtils
  SHARED
  XrdApps/XrdCpConfig.cc          XrdApps/XrdCpConfig.hh
  XrdApps/XrdCpFile.cc            XrdApps/XrdCpFile.hh
  XrdApps/XrdMpxXml.cc            XrdApps/XrdMpxXml.hh )

add_library(
  XrdAppUtils_static
  STATIC
  XrdApps/XrdCpConfig.cc          XrdApps/XrdCpConfig.hh
  XrdApps/XrdCpFile.cc            XrdApps/XrdCpFile.hh
  XrdApps/XrdMpxXml.cc            XrdApps/XrdMpxXml.hh )

target_link_libraries(
  XrdAppUtils
  XrdUtils )

set_target_properties(
  XrdAppUtils
  PROPERTIES
  VERSION   ${XRD_APP_UTILS_VERSION}
  SOVERSION ${XRD_APP_UTILS_SOVERSION} )

#-------------------------------------------------------------------------------
# XrdClProxyPlugin library
#-------------------------------------------------------------------------------
add_library(
  ${LIB_XRDCL_PROXY_PLUGIN}
  MODULE
  XrdApps/XrdClProxyPlugin/ProxyPrefixPlugin.cc
  XrdApps/XrdClProxyPlugin/ProxyPrefixFile.cc)

target_link_libraries(${LIB_XRDCL_PROXY_PLUGIN} XrdCl)

set_target_properties(
  ${LIB_XRDCL_PROXY_PLUGIN}
  PROPERTIES
  INTERFACE_LINK_LIBRARIES ""
  LINK_INTERFACE_LIBRARIES "" )

#-------------------------------------------------------------------------------
# Install
#-------------------------------------------------------------------------------
install(
  TARGETS XrdAppUtils XrdAppUtils_static ${LIB_XRDCL_PROXY_PLUGIN}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR} )

if( NOT XRDCL_ONLY )
  install(
    TARGETS xrdacctest xrdadler32 xrdcp-old cconfig mpxstats wait41 xrdmapc
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR} )
endif()

install(
  FILES
  ${PROJECT_SOURCE_DIR}/docs/man/xrdadler32.1
  ${PROJECT_SOURCE_DIR}/docs/man/xrdcp-old.1
  DESTINATION ${CMAKE_INSTALL_MANDIR}/man1 )

install(
  FILES
  ${PROJECT_SOURCE_DIR}/docs/man/mpxstats.8
  DESTINATION ${CMAKE_INSTALL_MANDIR}/man8 )
