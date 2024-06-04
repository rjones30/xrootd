
include( XRootDCommon )

#-------------------------------------------------------------------------------
# Modules
#-------------------------------------------------------------------------------
set( LIB_XRD_SSI        XrdSsi-${PLUGIN_VERSION} )
set( LIB_XRD_SSILOG     XrdSsiLog-${PLUGIN_VERSION} )

#-------------------------------------------------------------------------------
# Shared library version
#-------------------------------------------------------------------------------
set( XRD_SSI_LIB_VERSION   1.0.0 )
set( XRD_SSI_LIB_SOVERSION 1 )
set( XRD_SSI_SHMAP_VERSION   1.0.0 )
set( XRD_SSI_SHMAP_SOVERSION 1 )

#-------------------------------------------------------------------------------
# The XrdSsiLib library
#-------------------------------------------------------------------------------
add_library(
  XrdSsiLib
  SHARED
XrdSsi/XrdSsiAlert.cc                  XrdSsi/XrdSsiAlert.hh
                                       XrdSsi/XrdSsiAtomics.hh
                                       XrdSsi/XrdSsiBVec.hh
XrdSsi/XrdSsiClient.cc
                                       XrdSsi/XrdSsiCluster.hh
XrdSsi/XrdSsiCms.cc                    XrdSsi/XrdSsiCms.hh
                                       XrdSsi/XrdSsiErrInfo.hh
XrdSsi/XrdSsiEvent.cc                  XrdSsi/XrdSsiEvent.hh
XrdSsi/XrdSsiFileResource.cc           XrdSsi/XrdSsiFileResource.hh
XrdSsi/XrdSsiLogger.cc                 XrdSsi/XrdSsiLogger.hh
XrdSsi/XrdSsiPacer.cc                  XrdSsi/XrdSsiPacer.hh
                                       XrdSsi/XrdSsiProvider.hh
                                       XrdSsi/XrdSsiRRAgent.hh
                                       XrdSsi/XrdSsiRRInfo.hh
                                       XrdSsi/XrdSsiRRTable.hh
XrdSsi/XrdSsiRequest.cc                XrdSsi/XrdSsiRequest.hh
XrdSsi/XrdSsiResponder.cc              XrdSsi/XrdSsiResponder.hh
                                       XrdSsi/XrdSsiResource.hh
                                       XrdSsi/XrdSsiScale.hh
XrdSsi/XrdSsiServReal.cc               XrdSsi/XrdSsiServReal.hh
XrdSsi/XrdSsiService.cc                XrdSsi/XrdSsiService.hh
XrdSsi/XrdSsiSessReal.cc               XrdSsi/XrdSsiSessReal.hh
XrdSsi/XrdSsiStats.cc                  XrdSsi/XrdSsiStats.hh
                                       XrdSsi/XrdSsiStream.hh
XrdSsi/XrdSsiTaskReal.cc               XrdSsi/XrdSsiTaskReal.hh
                                       XrdSsi/XrdSsiTrace.hh
XrdSsi/XrdSsiUtils.cc                  XrdSsi/XrdSsiUtils.hh)

add_library(
  XrdSsiLib_static
  STATIC
XrdSsi/XrdSsiAlert.cc                  XrdSsi/XrdSsiAlert.hh
                                       XrdSsi/XrdSsiAtomics.hh
                                       XrdSsi/XrdSsiBVec.hh
XrdSsi/XrdSsiClient.cc
                                       XrdSsi/XrdSsiCluster.hh
XrdSsi/XrdSsiCms.cc                    XrdSsi/XrdSsiCms.hh
                                       XrdSsi/XrdSsiErrInfo.hh
XrdSsi/XrdSsiEvent.cc                  XrdSsi/XrdSsiEvent.hh
XrdSsi/XrdSsiFileResource.cc           XrdSsi/XrdSsiFileResource.hh
XrdSsi/XrdSsiLogger.cc                 XrdSsi/XrdSsiLogger.hh
XrdSsi/XrdSsiPacer.cc                  XrdSsi/XrdSsiPacer.hh
                                       XrdSsi/XrdSsiProvider.hh
                                       XrdSsi/XrdSsiRRAgent.hh
                                       XrdSsi/XrdSsiRRInfo.hh
                                       XrdSsi/XrdSsiRRTable.hh
XrdSsi/XrdSsiRequest.cc                XrdSsi/XrdSsiRequest.hh
XrdSsi/XrdSsiResponder.cc              XrdSsi/XrdSsiResponder.hh
                                       XrdSsi/XrdSsiResource.hh
                                       XrdSsi/XrdSsiScale.hh
XrdSsi/XrdSsiServReal.cc               XrdSsi/XrdSsiServReal.hh
XrdSsi/XrdSsiService.cc                XrdSsi/XrdSsiService.hh
XrdSsi/XrdSsiSessReal.cc               XrdSsi/XrdSsiSessReal.hh
XrdSsi/XrdSsiStats.cc                  XrdSsi/XrdSsiStats.hh
                                       XrdSsi/XrdSsiStream.hh
XrdSsi/XrdSsiTaskReal.cc               XrdSsi/XrdSsiTaskReal.hh
                                       XrdSsi/XrdSsiTrace.hh
XrdSsi/XrdSsiUtils.cc                  XrdSsi/XrdSsiUtils.hh)

target_link_libraries(
  XrdSsiLib
  XrdCl
  XrdUtils
  pthread )

set_target_properties(
  XrdSsiLib
  PROPERTIES
  VERSION   ${XRD_SSI_LIB_VERSION}
  SOVERSION ${XRD_SSI_LIB_SOVERSION}
  LINK_INTERFACE_LIBRARIES "" )

#-------------------------------------------------------------------------------
# The XrdSsiShMap library
#-------------------------------------------------------------------------------
add_library(
  XrdSsiShMap
  SHARED
XrdSsi/XrdSsiShMam.cc                  XrdSsi/XrdSsiShMam.hh
XrdSsi/XrdSsiShMap.icc                 XrdSsi/XrdSsiShMap.hh
XrdSsi/XrdSsiShMat.cc                  XrdSsi/XrdSsiShMat.hh)

add_library(
  XrdSsiShMap_static
  STATIC
XrdSsi/XrdSsiShMam.cc                  XrdSsi/XrdSsiShMam.hh
XrdSsi/XrdSsiShMap.icc                 XrdSsi/XrdSsiShMap.hh
XrdSsi/XrdSsiShMat.cc                  XrdSsi/XrdSsiShMat.hh)

target_link_libraries(
  XrdSsiShMap
  ${ZLIB_LIBRARY}
  pthread )

set_target_properties(
  XrdSsiShMap
  PROPERTIES
  VERSION   ${XRD_SSI_SHMAP_VERSION}
  SOVERSION ${XRD_SSI_SHMAP_SOVERSION}
  LINK_INTERFACE_LIBRARIES "" )

#-------------------------------------------------------------------------------
# The XrdSsi plugin
#-------------------------------------------------------------------------------
add_library(
  ${LIB_XRD_SSI}
  MODULE
  XrdSsi/XrdSsiDir.cc                    XrdSsi/XrdSsiDir.hh
  XrdSsi/XrdSsiFile.cc                   XrdSsi/XrdSsiFile.hh
  XrdSsi/XrdSsiFileReq.cc                XrdSsi/XrdSsiFileReq.hh
  XrdSsi/XrdSsiFileSess.cc               XrdSsi/XrdSsiFileSess.hh
  XrdSsi/XrdSsiSfs.cc                    XrdSsi/XrdSsiSfs.hh
  XrdSsi/XrdSsiSfsConfig.cc              XrdSsi/XrdSsiSfsConfig.hh
  XrdSsi/XrdSsiStat.cc
)

target_link_libraries(
  ${LIB_XRD_SSI}
  XrdSsiLib
  XrdUtils
  XrdServer )

set_target_properties(
  ${LIB_XRD_SSI}
  PROPERTIES
  INTERFACE_LINK_LIBRARIES ""
  LINK_INTERFACE_LIBRARIES "" )

#-------------------------------------------------------------------------------
# The XrdSsiLog plugin
#-------------------------------------------------------------------------------
add_library(
  ${LIB_XRD_SSILOG}
  MODULE
  XrdSsi/XrdSsiLogging.cc
)

target_link_libraries(
  ${LIB_XRD_SSILOG}
  XrdSsiLib
  XrdUtils
  XrdServer )

set_target_properties(
  ${LIB_XRD_SSILOG}
  PROPERTIES
  INTERFACE_LINK_LIBRARIES ""
  LINK_INTERFACE_LIBRARIES "" )

#-------------------------------------------------------------------------------
# Install
#-------------------------------------------------------------------------------
install(
  TARGETS XrdSsiLib XrdSsiLib_static XrdSsiShMap XrdSsiShMap_static ${LIB_XRD_SSI} ${LIB_XRD_SSILOG}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR} )
