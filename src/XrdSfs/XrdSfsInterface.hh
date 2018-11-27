/* (c) 2018 by the Board of Trustees of the Leland Stanford, Jr., University  */
#define Prep_EVICT 1024
struct XrdSfsFSctl //!< SFS_FSCTL_PLUGIN/PLUGIO parms
 const char            *Arg1;      //!< PLUGIO, PLUGIN
 const char            *Arg2;      //!< PLUGIN  opaque string
struct XrdSfsFACtl;
/*                       X r d S f s D i r e c t o r y                        */

//------------------------------------------------------------------------------
//! The XrdSfsDirectory object is returned by XrdSfsFileSystem::newFile() when
//! the caller wants to be able to perform directory oriented operations.
//------------------------------------------------------------------------------
class XrdSfsDirectory
{
public:

//! The error object is used to return details whenever something other than
//! SFS_OK is returned from the methods in this class, when noted.
//-----------------------------------------------------------------------------

        XrdOucErrInfo &error;

//-----------------------------------------------------------------------------
//! Open a directory.
//! @param  path   - Pointer to the path of the directory to be opened.
//! @param  client - Client's identify (see common description).
//! @param  opaque - path's CGI information (see common description).
//! @return One of SFS_OK, SFS_ERROR, SFS_REDIRECT, ir SFS_STALL
//-----------------------------------------------------------------------------

virtual int         open(const char              *path,
                         const XrdSecEntity      *client = 0,
                         const char              *opaque = 0) = 0;

//-----------------------------------------------------------------------------
//! Get the next directory entry.
//! @return A null terminated string with the directory name. Normally, "."
//!         ".." are not returned. If a null pointer is returned then if this
//!         is due to an error, error.code should contain errno. Otherwise,
//!         error.code should contain zero to indicate that no more entries
//!         exist (i.e. end of list).
virtual const char *nextEntry() = 0;
//! Close the directory.
//! @return One of SFS_OK or SFS_ERROR
//-----------------------------------------------------------------------------

virtual int         close() = 0;

//-----------------------------------------------------------------------------
//! Get the directory path.
//! @return Null terminated string of the path used in open().
virtual const char *FName() = 0;
//! Set the stat() buffer where stat information is to be placed corresponding
//! to the directory entry returned by nextEntry().
//!
//! @return If supported, SFS_OK should be returned. If not supported, then
//!         SFS_ERROR should be returned with error.code set to ENOTSUP.
//-----------------------------------------------------------------------------

virtual int         autoStat(struct stat *buf)
                            {(void)buf;
                             error.setErrInfo(ENOTSUP, "Not supported.");
                             return SFS_ERROR;
                            }

//-----------------------------------------------------------------------------
//! Constructor (user and MonID are the ones passed to newDir()!). This
//! constructor should only be used by base plugins. Plugins that wrap an
//! SfsDirectory should use the second version of the constructor shown below.
                    XrdSfsDirectory(const char *user=0, int MonID=0)
                                   : error(*(new XrdOucErrInfo(user, MonID)))
                                   {lclEI = &error;}
//! Constructor for plugins that wrap another SfsDirectory. This constructor
//! inherits the error object from a wrapped SfsDirectory object so that only
//! one identical error object exists for all directory objects in the chain.
//! @param  wrapD  - Reference to the directory object being wrapped.
//-----------------------------------------------------------------------------

                    XrdSfsDirectory(XrdSfsDirectory &wrapD)
                                   : error(wrapD.error), lclEI(0) {}

//-----------------------------------------------------------------------------
//! Constructor for base plugins that predefined an error object. This is a
//! convenience constructor for base plugins only.
//! @param  eInfo  - Reference to the error object to use.
                    XrdSfsDirectory(XrdOucErrInfo &eInfo)
                                   : error(eInfo), lclEI(0) {}
//-----------------------------------------------------------------------------
//! Destructor
//-----------------------------------------------------------------------------

virtual            ~XrdSfsDirectory() {if (lclEI) delete lclEI;}

private:
XrdOucErrInfo* lclEI;

}; // class XrdSfsDirectory

/******************************************************************************/
/*                            X r d S f s F i l e                             */
/******************************************************************************/

//------------------------------------------------------------------------------
//! The XrdSfsFile object is returned by XrdSfsFileSystem::newFile() when
//! the caller wants to be able to perform file oriented operations.
//------------------------------------------------------------------------------

class XrdSfsAio;
class XrdSfsDio;
class XrdSfsXio;
  
class XrdSfsFile
public:
//! The error object is used to return details whenever something other than
//! SFS_OK is returned from the methods in this class, when noted.
//-----------------------------------------------------------------------------

        XrdOucErrInfo  &error;

//-----------------------------------------------------------------------------
//! Open a file.
//! @param  path   - Pointer to the path of the file to be opened.
//! @param  oMode  - Flags indicating how the open is to be handled.
//!                  SFS_O_CREAT   create the file
//!                  SFS_O_MKPTH   Make directory path if missing
//!                  SFS_O_NOWAIT  do not impose operational delays
//!                  SFS_O_POSC    persist only on successful close
//!                  SFS_O_RAWIO   allow client-side decompression
//!                  SFS_O_RDONLY  open read/only
//!                  SFS_O_RDWR    open read/write
//!                  SFS_O_REPLICA Open for replication
//!                  SFS_O_RESET   Reset any cached information
//!                  SFS_O_TRUNC   truncate existing file to zero length
//!                  SFS_O_WRONLY  open write/only
//! @param  cMode  - The file's mode if it will be created.
//! @param  opaque - path's CGI information (see common description).
//! @return One of SFS_OK, SFS_ERROR, SFS_REDIRECT, SFS_STALL, or SFS_STARTED
virtual int            open(const char                *fileName,
                                  XrdSfsFileOpenMode   openMode,
                                  mode_t               createMode,
                            const XrdSecEntity        *client = 0,
                            const char                *opaque = 0) = 0;
//! Close the file.
//! @return One of SFS_OK or SFS_ERROR.
virtual int            close() = 0;
//! Execute a special operation on the file (version 1)
//! @param  cmd   - The operation to be performed (see below).
//!                 SFS_FCTL_GETFD    Return file descriptor if possible
//!                 SFS_FCTL_STATV    Reserved for future use.
//! @param  args  - specific arguments to cmd
//!                 SFS_FCTL_GETFD    Set to zero.
//! @param  eInfo  - The object where error info or results are to be returned.
//!                  This is legacy and the error onject may be used as well.
//!
//! @return If an error occurs or the operation is not support, SFS_ERROR
//!         should be returned with error.code set to errno. Otherwise,
//!         SFS_FCTL_GETFD  error.code holds the real file descriptor number
//!                         If the value is negative, sendfile() is not used.
//!                         If the value is SFS_SFIO_FDVAL then the SendData()
//!                         method is used for future read requests.
virtual int            fctl(const int               cmd,
                            const char             *args,
                                  XrdOucErrInfo    &eInfo) = 0;
//! Execute a special operation on the file (version 2)
//!                  SFS_FCTL_SPEC1    Perform implementation defined action
//! @param  alen   - Length of data pointed to by args.
//! @param  args   - Data sent with request, zero if alen is zero.
virtual int            fctl(const int               cmd,
                                  int               alen,
                                  const char       *args,
                            const XrdSecEntity     *client = 0)
  (void)cmd; (void)alen; (void)args; (void)client;
//! Get the file path.
//! @return Null terminated string of the path used in open().
//-----------------------------------------------------------------------------

virtual const char    *FName() = 0;

//-----------------------------------------------------------------------------
//! Get file's memory mapping if one exists (memory mapped files only).
//! @param  addr   - Place where the starting memory address is returned.
//! @param  size   - Place where the file's size is returned.
//!
//! @return SFS_OK when the file is memory mapped or any other code otherwise.
virtual int            getMmap(void **Addr, off_t &Size) = 0;
//! Preread file blocks into the file system cache.
//! @param  offset  - The offset where the read is to start.
//! @param  size    - The number of bytes to pre-read.
//! @return >= 0      The number of bytes that will be pre-read.
//! @return SFS_ERROR File could not be preread, error holds the reason.
virtual XrdSfsXferSize read(XrdSfsFileOffset   offset,
                            XrdSfsXferSize     size) = 0;
//! Read file bytes into a buffer.
//! @param  offset  - The offset where the read is to start.
//! @param  buffer  - pointer to buffer where the bytes are to be placed.
//! @param  size    - The number of bytes to read.
//!
//! @return >= 0      The number of bytes that placed in buffer.
//! @return SFS_ERROR File could not be read, error holds the reason.
virtual XrdSfsXferSize read(XrdSfsFileOffset   offset,
                            char              *buffer,
                            XrdSfsXferSize     size) = 0;
//! Read file bytes using asynchrnous I/O.
//! @param  aioparm - Pointer to async I/O object controlling the I/O.
//! @return SFS_OK    Request accepted and will be scheduled.
//! @return SFS_ERROR File could not be read, error holds the reason.
virtual XrdSfsXferSize read(XrdSfsAio *aioparm) = 0;
//! Given an array of read requests (size rdvCnt), read them from the file
//! and place the contents consecutively in the provided buffer. A dumb default
//! implementation is supplied but should be replaced to increase performance.
//! @param  readV     pointer to the array of read requests.
//! @param  rdvcnt    the number of elements in readV.
//! @return >=0       The numbe of bytes placed into the buffer.
//! @return SFS_ERROR File could not be read, error holds the reason.
virtual XrdSfsXferSize readv(XrdOucIOVec      *readV,
                             int               rdvCnt)
                            {XrdSfsXferSize rdsz, totbytes = 0;
                             for (int i = 0; i < rdvCnt; i++)
                                 {rdsz = read(readV[i].offset,
                                              readV[i].data, readV[i].size);
                                  if (rdsz != readV[i].size)
                                     {if (rdsz < 0) return rdsz;
                                      error.setErrInfo(ESPIPE,"read past eof");
                                      return SFS_ERROR;
                                     }
                                  totbytes += rdsz;
                                 }
                             return totbytes;
                            }
//! Send file bytes via a XrdSfsDio sendfile object to a client (optional).
//! @param  sfDio   - Pointer to the sendfile object for data transfer.
//! @param  offset  - The offset where the read is to start.
//! @param  size    - The number of bytes to read and send.
//! @return SFS_ERROR File not read, error object has reason.
//! @return SFS_OK    Either data has been successfully sent via sfDio or no
//!                   data has been sent and a normal read() should be issued.
virtual int            SendData(XrdSfsDio         *sfDio,
                                XrdSfsFileOffset   offset,
                                XrdSfsXferSize     size)
{
  (void)sfDio; (void)offset; (void)size;
  return SFS_OK;
}
//! Write file bytes from a buffer.
//! @param  offset  - The offset where the write is to start.
//! @param  buffer  - pointer to buffer where the bytes reside.
//! @param  size    - The number of bytes to write.
//! @return >= 0      The number of bytes that were written.
//! @return SFS_ERROR File could not be written, error holds the reason.
virtual XrdSfsXferSize write(XrdSfsFileOffset  offset,
                             const char       *buffer,
                             XrdSfsXferSize    size) = 0;
//! Write file bytes using asynchrnous I/O.
//! @param  aioparm - Pointer to async I/O object controlling the I/O.
//! @return  0       Request accepted and will be scheduled.
//! @return !0       Request not accepted, returned value is errno.
virtual int            write(XrdSfsAio *aioparm) = 0;
//! Given an array of write requests (size wdvcnt), write them to the file
//! from the provided associated buffer. A dumb default implementation is
//! supplied but should be replaced to increase performance.
//! @param  writeV    pointer to the array of write requests.
//! @param  wdvcnt    the number of elements in writeV.
//! @return >=0       The total number of bytes written to the file.
//! @return SFS_ERROR File could not be written, error holds the reason.
virtual XrdSfsXferSize writev(XrdOucIOVec      *writeV,
                              int               wdvCnt)
                             {XrdSfsXferSize wrsz, totbytes = 0;
                              for (int i = 0; i < wdvCnt; i++)
                                  {wrsz = write(writeV[i].offset,
                                                writeV[i].data, writeV[i].size);
                                   if (wrsz != writeV[i].size)
                                      {if (wrsz < 0) return wrsz;
                                      error.setErrInfo(ESPIPE,"write past eof");
                                      return SFS_ERROR;
                                     }
                                  totbytes += wrsz;
                                 }
                             return totbytes;
                            }
//! Return state information on the file.
//! @return One of SFS_OK, SFS_ERROR, SFS_REDIRECT, or SFS_STALL. When SFS_OK
//!         is returned, buf must hold stat information.
virtual int            stat(struct stat *buf) = 0;
//! Make sure all outstanding data is actually written to the file (sync).
virtual int            sync() = 0;
//! Make sure all outstanding data is actually written to the file (async).
//!
//! @return SFS_OK    Request accepted and will be scheduled.
//! @return SFS_ERROR Request could not be accepted, return error has reason.
//-----------------------------------------------------------------------------

virtual int            sync(XrdSfsAio *aiop) = 0;

//-----------------------------------------------------------------------------
//! Truncate the file.
virtual int            truncate(XrdSfsFileOffset fsize) = 0;
//! Get compression information for the file.
//!
//! @param  cxtype - Place where the compression algorithm name is to be placed
//! @param  cxrsz  - Place where the compression page size is to be returned
//!
//! @return One of the valid SFS return codes described above. If the file
//!         is not compressed or an error is returned, cxrsz must be set to 0.
virtual int            getCXinfo(char cxtype[4], int &cxrsz) = 0;
//! Enable exchange buffer I/O for write calls.
//!
//! @param  - Pointer to the XrdSfsXio object to be used for buffer exchanges.
virtual void           setXio(XrdSfsXio *xioP) { (void)xioP; }
//! Constructor (user and MonID are the ones passed to newFile()!). This
//! constructor should only be used by base plugins. Plugins that wrap an
//! SfsFile should use the second version of the constructor shown below.
//!
//! @param  user   - Text identifying the client responsible for this call.
//!                  The pointer may be null if identification is missing.
//! @param  MonID  - The monitoring identifier assigned to this and all
//!                  future requests using the returned object.
                       XrdSfsFile(const char *user=0, int MonID=0)
                                 : error(*(new XrdOucErrInfo(user, MonID)))
                                 {lclEI = &error;}
//! Constructor for plugins that wrap another SFS plugin. This constructor
//! inherits the error object from a wrapped XrdSfsFile object so that only
//! one identical error object exists for all file objects in the chain.
//! @param  wrapF  - Reference to the file object being wrapped.
                       XrdSfsFile(XrdSfsFile &wrapF)
                                 : error(wrapF.error), lclEI(0) {}
//! Constructor for base plugins that predefined an error object. This is a
//! convenience constructor for base plugins only.
//! @param  eInfo  - Reference to the error object to use.
                       XrdSfsFile(XrdOucErrInfo &eInfo)
                                 : error(eInfo), lclEI(0) {}
//! Destructor
virtual               ~XrdSfsFile() {if (lclEI) delete lclEI;}
private:
XrdOucErrInfo* lclEI;

}; // class XrdSfsFile

/******************************************************************************/
/*                      X r d S f s F i l e S y s t e m                       */
/******************************************************************************/
  
//! Common parameters: Many of the methods have certain common parameters.
//! These are documented here to avoid lengthy duplicate descriptions.
//! @param  eInfo  - The object where error info or results are to be returned.
//!                  For errors, you should return information as follows:
//!                  SFS_OK       eInfo may contain results, as described in
//!                               specified method description that follows.
//!                  SFS_ERROR    eInfo.code    - errno number
//!                               eInfo.message - error message text
//!                  SFS_REDIRECT eInfo.code    - target port number
//!                               eInfo.message - target host address/name
//!                  SFS_STALL    eInfo.code    - expected seconds to stall
//!                               eInfo.message - reason for the delay
//!                  SFS_STARTED  eInfo.code    - expected seconds to completion
//!                               eInfo.message - reason for the delay
//!                  SFS_DATA     eInfo.code    - length of data in message
//!                               eInfo.message - the request data
//! @param  client - Pointer to the client's identity information or nil if
//!                  the identity is not known.
//!
//! @param  opaque - Pointer to the CGI information associated with Path or
//!                  nil if there is no opaque information.
class XrdSfsFileSystem
public:
//! Obtain a new director object to be used for future directory requests.
//!
//! @param  user   - Text identifying the client responsible for this call.
//!                  The pointer may be null if identification is missing.
//! @param  MonID  - The monitoring identifier assigned to this and all
//!                  future requests using the returned object.
//! @return pointer- Pointer to an XrdSfsDirectory object.
//! @return nil    - Insufficient memory to allocate an object.
virtual XrdSfsDirectory *newDir(char *user=0, int MonID=0)  = 0;
//! Obtain a new wrapped directory object to be used for future requests.
//! @param  eInfo  - Reference to the error object to be used by the new
//!                  directory object. Note that an implementation is supplied
//!                  for compatability purposes that results in the directory
//!                  object to be larger than needed because it would also
//!                  supply an errinfo object. You can override this method
//!                  in order to return a much smaller object w/o errinfo.
//! @return pointer- Pointer to an XrdSfsDirectory object.
//! @return nil    - Insufficient memory to allocate an object.
virtual XrdSfsDirectory *newDir(XrdOucErrInfo &eInfo)
                               {XrdSfsDirectory *dP = newDir();
                                if (dP) dP->error = eInfo;
                                return dP;
                               }
//! Obtain a new file object to be used for a future file requests.
//! @param  user   - Text identifying the client responsible for this call.
//!                  The pointer may be null if identification is missing.
//! @param  MonID  - The monitoring identifier assigned to this and all
//!                  future requests using the returned object.
//! @return pointer- Pointer to an XrdSfsFile object.
//! @return nil    - Insufficient memory to allocate an object.
virtual XrdSfsFile      *newFile(char *user=0, int MonID=0) = 0;
//! Obtain a new wrapped file object to be used for a future requests.
//! @param  eInfo  - Reference to the error object to be used by the new file
//!                  object. Note that an implementation is supplied for
//!                  compatability purposes results in the new file object
//!                  to be larger than needed because it would also supply
//!                  an errinfo object. You can override this method in order
//!                  to return a much smaller object by omitting the errinfo.
//! @return pointer- Pointer to an XrdSfsFile object.
//! @return nil    - Insufficient memory to allocate an object.
virtual XrdSfsFile      *newFile(XrdOucErrInfo &eInfo)
                                {XrdSfsFile *fP = newFile();
                                 if (fP) fP->error = eInfo;
                                 return fP;
                                }
//! Obtain checksum information for a file.
//! @param  Func   - The checksum operation to be performed:
//!                  csCalc  - (re)calculate and return the checksum value
//!                  csGet   - return the existing checksum value, if any
//!                  csSize  - return the size of the checksum value that
//!                            corresponds to csName (path may be null).
//! @param  csName - The name of the checksum value wanted.
//! @param  path   - Pointer to the path of the file in question.
//! @param  eInfo  - The object where error info or results are to be returned.
//! @param  client - Client's identify (see common description).
//! @param  opaque - Path's CGI information (see common description).
//! @return One of SFS_OK, SFS_ERROR, or SFS_REDIRECT. When SFS_OK is returned,
//!         eInfo should contain results, as follows:
//!         csCalc/csGet eInfo.message - null terminated string with the
//!                                      checksum value in ASCII hex.
//!         csSize       eInfo.code    - size of binary checksum value.
enum    csFunc {csCalc = 0, csGet, csSize};

virtual int            chksum(      csFunc            Func,
                              const char             *csName,
                              const char             *path,
                                    XrdOucErrInfo    &eInfo,
                              const XrdSecEntity     *client = 0,
                              const char             *opaque = 0)
{
  (void)Func; (void)csName; (void)path; (void)eInfo; (void)client;
  (void)opaque;
  eInfo.setErrInfo(ENOTSUP, "Not supported.");
  return SFS_ERROR;
}
//! Change file mode settings.
//! @param  path   - Pointer to the path of the file in question.
//! @param  mode   - The new file mode setting.
//! @param  eInfo  - The object where error info or results are to be returned.
//! @param  client - Client's identify (see common description).
//! @param  opaque - Path's CGI information (see common description).
//! @return One of SFS_OK, SFS_ERROR, SFS_REDIRECT or SFS_STALL
virtual int            chmod(const char             *path,
                                   XrdSfsMode        mode,
                                   XrdOucErrInfo    &eInfo,
                             const XrdSecEntity     *client = 0,
                             const char             *opaque = 0) = 0;
//! Notify filesystem that a client has disconnected.
//! @param  client - Client's identify (see common description).
//-----------------------------------------------------------------------------

virtual void           Disc(const XrdSecEntity     *client = 0)
{
  (void)client;
}

//-----------------------------------------------------------------------------
//! Notify filesystem about implmentation dependent environment. This method
//! may be called only once, if at all, right after obtaining this object.
//! @param  envP   - Pointer to environmental information.
virtual void           EnvInfo(XrdOucEnv *envP)
  (void)envP;
//! Perform a filesystem extended attribute function.
//! @param  faReq  - pointer to the request object (see XrdSfsFAttr.hh). If the
//!                  pointer is nill, simply return whether or not extended
//!                  attributes are supported.
//! @param  eInfo  - The object where error info or results are to be returned.
//! @param  client - Client's identify (see common description).
//! @return SFS_OK   a null response is sent.
//! @return SFS_DATA error.code    length of the data to be sent.
//!                  error.message contains the data to be sent.
//! @return SFS_STARTED Operation started result will be returned via callback.
//!         o/w      one of SFS_ERROR, SFS_REDIRECT, or SFS_STALL.
virtual int            FAttr(      XrdSfsFACtl      *faReq,
                                   XrdOucErrInfo    &eInfo,
                             const XrdSecEntity     *client = 0)
                            {(void)faReq; (void)client;
                             eInfo.setErrInfo(ENOTSUP, "Not supported.");
                             return SFS_ERROR;
                            }
//! Perform a filesystem control operation (version 2)
//! @param  cmd    - The operation to be performed:
//!                  SFS_FSCTL_PLUGIN  Return Implementation Dependent Data v1
//!                  SFS_FSCTL_PLUGIO  Return Implementation Dependent Data v2
//! @param  args   - Arguments specific to cmd.
//!                  SFS_FSCTL_PLUGIN  path and opaque information.
//!                  SFS_FSCTL_PLUGIO  Unscreened argument string.
//! @param  eInfo  - The object where error info or results are to be returned.
//! @param  client - Client's identify (see common description).
//! @return SFS_OK   a null response is sent.
//!         SFS_DATA error.code    length of the data to be sent.
//!                  error.message contains the data to be sent.
//!         o/w      one of SFS_ERROR, SFS_REDIRECT, or SFS_STALL.
virtual int            FSctl(const int               cmd,
                                   XrdSfsFSctl      &args,
                                   XrdOucErrInfo    &eInfo,
                             const XrdSecEntity     *client = 0)
{
  (void)cmd; (void)args; (void)eInfo; (void)client;
  return SFS_OK;
}
//! Perform a filesystem control operation (version 1)
//! @param  cmd    - The operation to be performed:
//!                  SFS_FSCTL_LOCATE  Locate a file or file servers
//!                  SFS_FSCTL_STATCC  Return cluster config status
//!                  SFS_FSCTL_STATFS  Return physical filesystem information
//!                  SFS_FSCTL_STATLS  Return logical  filesystem information
//!                  SFS_FSCTL_STATXA  Return extended attributes
//! @param  args   - Arguments specific to cmd.
//!                  SFS_FSCTL_LOCATE  args points to the path to be located
//!                                    ""   path is the first exported path
//!                                    "*"  return all current servers
//!                                    "*/" return servers exporting path
//!                                    o/w  return servers having the path
//!                  SFS_FSCTL_STATFS  Path in the filesystem in question.
//!                  SFS_FSCTL_STATLS  Path in the filesystem in question.
//!                  SFS_FSCTL_STATXA  Path of the file whose xattr is wanted.
//! @param  eInfo  - The object where error info or results are to be returned.
//! @param  client - Client's identify (see common description).
//! @return SFS_OK   a null response is sent.
//! @return SFS_DATA error.code    length of the data to be sent.
//!                  error.message contains the data to be sent.
//! @return SFS_STARTED Operation started result will be returned via callback.
//!                  Valid only for for SFS_FSCTL_LOCATE, SFS_FSCTL_STATFS, and
//!                  SFS_FSCTL_STATXA
//!         o/w      one of SFS_ERROR, SFS_REDIRECT, or SFS_STALL.
virtual int            fsctl(const int               cmd,
                             const char             *args,
                                   XrdOucErrInfo    &eInfo,
                             const XrdSecEntity     *client = 0) = 0;
//! Return statistical information.
//! @param  buff   - Pointer to the buffer where results are to be returned.
//!                  Statistics should be in standard XML format. If buff is
//!                  nil then only maximum size information is wanted.
//! @param  blen   - The length available in buff.
//! @return Number of bytes placed in buff. When buff is nil, the maximum
//!         number of bytes that could have been placed in buff.
virtual int            getStats(char *buff, int blen) = 0;
//! Get version string.
//!
//! @return The version string. Normally this is the XrdVERSION value.
//-----------------------------------------------------------------------------

virtual const char    *getVersion() = 0;

//-----------------------------------------------------------------------------
//! Return directory/file existence information (short stat).
//!
//! @param  path   - Pointer to the path of the file/directory in question.
//! @param  eFlag  - Where the results are to be returned.
//! @param  eInfo  - The object where error info is to be returned.
//! @param  client - Client's identify (see common description).
//! @param  opaque - Path's CGI information (see common description).
//!         When SFS_OK is returned, eFlag must be properly set, as follows:
//!         XrdSfsFileExistNo            - path does not exist
//!         XrdSfsFileExistIsFile        - path refers to an  online file
//!         XrdSfsFileExistIsDirectory   - path refers to an  online directory
//!         XrdSfsFileExistIsOffline     - path refers to an offline file
//!         XrdSfsFileExistIsOther       - path is neither a file nor directory
virtual int            exists(const char                *path,
                                    XrdSfsFileExistence &eFlag,
                                    XrdOucErrInfo       &eInfo,
                              const XrdSecEntity        *client = 0,
                              const char                *opaque = 0) = 0;
//! Create a directory.
//! @param  path   - Pointer to the path of the directory to be created.
//! @param  mode   - The directory mode setting.
//! @param  eInfo  - The object where error info is to be returned.
//! @param  client - Client's identify (see common description).
//! @param  opaque - Path's CGI information (see common description).
//!
//! @return One of SFS_OK, SFS_ERROR, SFS_REDIRECT, or SFS_STALL
virtual int            mkdir(const char              *path,
                                   XrdSfsMode         mode,
                                   XrdOucErrInfo     &eInfo,
                             const XrdSecEntity      *client = 0,
                             const char              *opaque = 0) = 0;
//! Preapre a file for future processing.
//! @param  pargs  - The preapre arguments.
//! @param  eInfo  - The object where error info is to be returned.
//! @param  client - Client's identify (see common description).
virtual int            prepare(      XrdSfsPrep      &pargs,
                                     XrdOucErrInfo   &eInfo,
                               const XrdSecEntity    *client = 0) = 0;
//! Remove a file.
//! @param  path   - Pointer to the path of the file to be removed.
//! @param  eInfo  - The object where error info is to be returned.
//! @param  client - Client's identify (see common description).
//! @param  opaque - Path's CGI information (see common description).
//! @return One of SFS_OK, SFS_ERROR, SFS_REDIRECT, or SFS_STALL
virtual int            rem(const char                *path,
                                 XrdOucErrInfo       &eInfo,
                           const XrdSecEntity        *client = 0,
                           const char                *opaque = 0) = 0;
//! Remove a directory.
//! @param  path   - Pointer to the path of the directory to be removed.
//! @param  eInfo  - The object where error info is to be returned.
//! @param  client - Client's identify (see common description).
//! @param  opaque - Path's CGI information (see common description).
//!
//! @return One of SFS_OK, SFS_ERROR, SFS_REDIRECT, or SFS_STALL
virtual int            remdir(const char             *path,
                                    XrdOucErrInfo    &eInfo,
                              const XrdSecEntity     *client = 0,
                              const char             *opaque = 0) = 0;
//! Rename a file or directory.
//! @param  oPath   - Pointer to the path to be renamed.
//! @param  nPath   - Pointer to the path oPath is to have.
//! @param  eInfo   - The object where error info is to be returned.
//! @param  client  - Client's identify (see common description).
//! @param  opaqueO - oPath's CGI information (see common description).
//! @param  opaqueN - nPath's CGI information (see common description).
//!
//! @return One of SFS_OK, SFS_ERROR, SFS_REDIRECT, or SFS_STALL
virtual int            rename(const char             *oPath,
                              const char             *nPath,
                                    XrdOucErrInfo    &eInfo,
                              const XrdSecEntity     *client = 0,
                              const char             *opaqueO = 0,
                              const char             *opaqueN = 0) = 0;
//! Return state information on a file or directory.
//!
//! @param  path   - Pointer to the path in question.
//! @param  buf    - Pointer to the structure where info it to be returned.
//! @param  eInfo  - The object where error info is to be returned.
//! @param  client - Client's identify (see common description).
//! @param  opaque - path's CGI information (see common description).
//!
//! @return One of SFS_OK, SFS_ERROR, SFS_REDIRECT, SFS_STALL, or SFS_STARTED
//!         When SFS_OK is returned, buf must contain stat information.
virtual int            stat(const char               *Name,
                                  struct stat        *buf,
                                  XrdOucErrInfo      &eInfo,
                            const XrdSecEntity       *client = 0,
                            const char               *opaque = 0) = 0;
//! Return mode information on a file or directory.
//!
//! @param  path   - Pointer to the path in question.
//! @param  mode   - Where full mode information is to be returned.
//! @param  eInfo  - The object where error info is to be returned.
//! @param  client - Client's identify (see common description).
//! @param  opaque - path's CGI information (see common description).
//!
//! @return One of SFS_OK, SFS_ERROR, SFS_REDIRECT, SFS_STALL, or SFS_STARTED
//!         When SFS_OK is returned, mode must contain mode information. If
//!         teh mode is -1 then it is taken as an offline file.
virtual int            stat(const char               *path,
                                  mode_t             &mode,
                                  XrdOucErrInfo      &eInfo,
                            const XrdSecEntity       *client = 0,
                            const char               *opaque = 0) = 0;
//! Truncate a file.
//! @param  path   - Pointer to the path of the file to be truncated.
//! @param  fsize  - The size that the file is to have.
//! @param  eInfo  - The object where error info is to be returned.
//! @return One of SFS_OK, SFS_ERROR, SFS_REDIRECT, or SFS_STALL
virtual int            truncate(const char             *path,
                                      XrdSfsFileOffset  fsize,
                                      XrdOucErrInfo    &eInfo,
                                const XrdSecEntity     *client = 0,
                                const char             *opaque = 0) = 0;
//! Constructor and Destructor
                       XrdSfsFileSystem() {}
virtual               ~XrdSfsFileSystem() {}
};

/******************************************************************************/
/*              F i l e   S y s t e m   I n s t a n t i a t o r               */
/******************************************************************************/
/*! When building a shared library plugin, the following "C" entry point must
    exist in the library:
    @param  nativeFS - the filesystem that would have been used. You may return
                       this pointer if you wish.
    @param  Logger   - The message logging object to be used for messages.
    @param  configFN - pointer to the path of the configuration file. If nil
                       there is no configuration file.
    @return Pointer to the file system object to be used or nil if an error
            occurred.
    extern "C"
         {XrdSfsFileSystem *XrdSfsGetFileSystem(XrdSfsFileSystem *nativeFS,
                                                XrdSysLogger     *Logger,
                                                const char       *configFn);
         }
    An alternate entry point may be defined in lieu of the previous entry point.
    This normally identified by a version option in the configuration file (e.g.
    xrootd.fslib -2 <path>). It differs in that an extra parameter is passed:
    @param  envP     - Pointer to the environment containing implementation
                       specific information.
    extern "C"
         {XrdSfsFileSystem *XrdSfsGetFileSystem2(XrdSfsFileSystem *nativeFS,
                                                 XrdSysLogger     *Logger,
                                                 const char       *configFn,
                                                 XrdOucEnv        *envP);
         }
*/
typedef XrdSfsFileSystem *(*XrdSfsFileSystem_t) (XrdSfsFileSystem *nativeFS,
                                                 XrdSysLogger     *Logger,
                                                 const char       *configFn);

typedef XrdSfsFileSystem *(*XrdSfsFileSystem2_t)(XrdSfsFileSystem *nativeFS,
                                                 XrdSysLogger     *Logger,
                                                 const char       *configFn,
                                                 XrdOucEnv        *envP);
  
//------------------------------------------------------------------------------
/*! Specify the compilation version.

    Additionally, you *should* declare the xrootd version you used to compile
    your plug-in. The plugin manager automatically checks for compatability.
    Declare it as follows:
    #include "XrdVersion.hh"
    XrdVERSIONINFO(XrdSfsGetFileSystem,<name>);
    where <name> is a 1- to 15-character unquoted name identifying your plugin.
*/
//------------------------------------------------------------------------------