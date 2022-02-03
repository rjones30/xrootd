//------------------------------------------------------------------------------
// Copyright (c) 2011-2017 by European Organization for Nuclear Research (CERN)
// Author: Michal Simon <michal.simon@cern.ch>
//------------------------------------------------------------------------------
// This file is part of the XRootD software suite.
//
// XRootD is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// XRootD is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with XRootD.  If not, see <http://www.gnu.org/licenses/>.
//
// In applying this licence, CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.
//------------------------------------------------------------------------------

#include "XrdCl/XrdClOperations.hh"
#include "XrdCl/XrdClUtils.hh"
#include "XrdCl/XrdClFileOperations.hh"
#include "XrdSys/XrdSysPthread.hh"

#include <fstream>
#include <vector>
#include <tuple>
#include <unordered_map>
#include <chrono>
#include <iostream>
#include <thread>
#include <chrono>

namespace XrdCl
{

inline uint64_t get_time()
{
  return std::chrono::duration_cast<std::chrono::seconds>(
           std::chrono::system_clock::now().time_since_epoch() ).count();
}

class ActionStatistics
{
  public:
    static ActionStatistics& Instance()
    {
      static ActionStatistics instance;
      return instance;
    }

    void UpdateOrg( const std::string &action, uint64_t duration )
    {
      std::unique_lock<std::mutex> lck( mtx );
      auto &tpl = orgstats[action];
      std::get<0>( tpl ) += duration;
      ++std::get<1>( tpl );
    }

    void UpdateAct( const std::string &action, uint64_t duration )
    {
      std::unique_lock<std::mutex> lck( mtx );
      auto &tpl = actstats[action];
      std::get<0>( tpl ) += duration;
      ++std::get<1>( tpl );
    }

    void Print()
    {
      std::cout << "Reference average durations per action:\n";
      for( auto &p : orgstats )
      {
        auto &tpl = p.second;
        double avg = double( std::get<0>( tpl ) ) / double( std::get<1>( tpl ) );
        std::cout << '\t' << p.first << "\t: " << avg << "s\n";
      }
      std::cout << "Average durations per action:\n";
      for( auto &p : actstats )
      {
        auto &tpl = p.second;
        double avg = double( std::get<0>( tpl ) ) / double( std::get<1>( tpl ) );
        std::cout << '\t' << p.first << "\t: " << avg << "s\n";
      }
    }

  private:

    ActionStatistics(){};

    ActionStatistics( ActionStatistics&& ) = delete;
    ActionStatistics( const ActionStatistics& ) = delete;

    ActionStatistics& operator=( ActionStatistics&& ) = delete;
    ActionStatistics& operator=( const ActionStatistics& ) = delete;

    std::mutex mtx;
    std::unordered_map<std::string, std::tuple<uint64_t, size_t>> orgstats;
    std::unordered_map<std::string, std::tuple<uint64_t, size_t>> actstats;
};


//------------------------------------------------------------------------------
//! Timer helper class
//------------------------------------------------------------------------------
class mytimer_t
{
  public:
    //--------------------------------------------------------------------------
    //! Constructor (record start time)
    //--------------------------------------------------------------------------
    mytimer_t() : start( clock_t::now() ){ }

    //--------------------------------------------------------------------------
    //! Reset the start time
    //--------------------------------------------------------------------------
    void reset(){ start = clock_t::now(); }

    //--------------------------------------------------------------------------
    //! @return : get time elapsed from start
    //--------------------------------------------------------------------------
    uint64_t elapsed() const
    {
      return std::chrono::duration_cast<std::chrono::seconds>( clock_t::now() - start ).count();
    }

  private:
    using clock_t = std::chrono::high_resolution_clock;
    std::chrono::time_point<clock_t> start; //< registered start time
};

//------------------------------------------------------------------------------
//! Barrier for synchronizing the asynchronous execution of actions
//! It is actually a wrapper around semaphore.
//------------------------------------------------------------------------------
class barrier_t
{
  public:
    //------------------------------------------------------------------------
    //! Constructor
    //! @param sem : the semaphore
    //------------------------------------------------------------------------
    barrier_t( XrdSysSemaphore &sem ) : sem( sem ) { }

    //------------------------------------------------------------------------
    //! Destructor
    //------------------------------------------------------------------------
    ~barrier_t()
    {
      sem.Post();
    }

    inline XrdSysSemaphore& get()
    {
      return sem;
    }

  private:
    XrdSysSemaphore &sem; //< the semaphore to be posted
};

//------------------------------------------------------------------------------
//! Executes an action registered in the csv file
//------------------------------------------------------------------------------
class ActionExecutor
{
  using buffer_t = std::shared_ptr<std::vector<char>>; //< data buffer

  public:

    //--------------------------------------------------------------------------
    //! Constructor
    //! @param file     : the file that should be the context of the action
    //! @param action   : the action to be executed
    //! @param args     : arguments for the action
    //! @param orgststr : original status
    //! @param resp     : original response
    //--------------------------------------------------------------------------
    ActionExecutor( File              &file,
                    const std::string &action,
                    const std::string &args,
                    const std::string &orgststr,
                    const std::string &resp,
                    uint64_t           start,
                    uint64_t           stop ) :
      file( file ),
      action( action ),
      args( args ),
      orgststr( orgststr ),
      orgstart( start ),
      orgstop( stop )
    {
    }

    //--------------------------------------------------------------------------
    //! Execute the action
    //! @param ending  : synchronization object for ending the execution
    //--------------------------------------------------------------------------
    void Execute( std::shared_ptr<barrier_t> &ending,
                  std::shared_ptr<barrier_t> &closing )
    {
      uint64_t start = get_time();
      if( action == "Open" ) // open action
      {
        std::string      url;
        OpenFlags::Flags flags;
        Access::Mode     mode;
        uint16_t         timeout;
        std::tie( url, flags, mode, timeout ) = GetOpenArgs();
        std::string tmp( orgststr );
        WaitFor( Open( file, url, flags, mode, timeout ) >>
                 [tmp, ending, closing, start]( XRootDStatus &s ) mutable
                 {
                   uint64_t duration = get_time() - start;
                   ActionStatistics::Instance().UpdateAct( "Open", duration );
                   HandleStatus( s, tmp );
                   ending.reset();
                   closing.reset();
                 } );

      }
      else if( action == "Close" ) // close action
      {
        uint16_t timeout = GetCloseArgs();
        std::string tmp( orgststr );
        if( closing )
        {
          auto &sem = closing->get();
          closing.reset();
          sem.Wait();
        }
        Async( Close( file, timeout ) >>
               [tmp, ending, start]( XRootDStatus &s ) mutable
               {
                 uint64_t duration = get_time() - start;
                 ActionStatistics::Instance().UpdateAct( "Close", duration );
                 HandleStatus( s, tmp );
                 ending.reset();
               } );
      }
      else if( action == "Stat" ) // stat action
      {
        bool force;
        uint16_t timeout;
        std::tie( force, timeout ) = GetStatArgs();
        std::string tmp( orgststr );
        Async( Stat( file, force, timeout ) >>
               [tmp, ending, closing, start]( XRootDStatus &s, StatInfo &r ) mutable
               {
                 uint64_t duration = get_time() - start;
                 ActionStatistics::Instance().UpdateAct( "Stat", duration );
                 HandleStatus( s, tmp );
                 ending.reset();
                 closing.reset();
               } );

      }
      else if( action == "Read" ) // read action
      {
        uint64_t offset;
        buffer_t buffer;
        uint16_t timeout;
        std::tie( offset, buffer, timeout ) = GetReadArgs();
        Async( Read( file, offset, buffer->size(), buffer->data(), timeout ) >>
               [buffer, orgststr{ orgststr }, ending, closing, start]( XRootDStatus &s, ChunkInfo &r ) mutable
               {
                 uint64_t duration = get_time() - start;
                 ActionStatistics::Instance().UpdateAct( "Read", duration );
                 HandleStatus( s, orgststr );
                 buffer.reset();
                 ending.reset();
                 closing.reset();
               } );
      }
      else if( action == "PgRead" ) // pgread action
      {
        uint64_t offset;
        buffer_t buffer;
        uint16_t timeout;
        std::tie( offset, buffer, timeout ) = GetPgReadArgs();
        Async( PgRead( file, offset, buffer->size(), buffer->data(), timeout ) >>
               [buffer, orgststr{ orgststr }, ending, closing, start]( XRootDStatus &s, PageInfo &r ) mutable
               {
                 uint64_t duration = get_time() - start;
                 ActionStatistics::Instance().UpdateAct( "PgRead", duration );
                 HandleStatus( s, orgststr );
                 buffer.reset();
                 ending.reset();
                 closing.reset();
               } );
      }
      else if( action == "Write" ) // write action
      {
        uint64_t offset;
        buffer_t buffer;
        uint16_t timeout;
        std::tie( offset, buffer, timeout ) = GetWriteArgs();
        Async( Write( file, offset, buffer->size(), buffer->data(), timeout ) >>
               [buffer, orgststr{ orgststr }, ending, closing, start]( XRootDStatus &s ) mutable
               {
                 uint64_t duration = get_time() - start;
                 ActionStatistics::Instance().UpdateAct( "Write", duration );
                 HandleStatus( s, orgststr );
                 buffer.reset();
                 ending.reset();
                 closing.reset();
               } );
      }
      else if( action == "PgWrite" ) // pgwrite action
      {
        uint64_t offset;
        buffer_t buffer;
        uint16_t timeout;
        std::tie( offset, buffer, timeout ) = GetPgWriteArgs();
        Async( PgWrite( file, offset, buffer->size(), buffer->data(), timeout ) >>
               [buffer, orgststr{ orgststr }, ending, closing, start]( XRootDStatus &s ) mutable
               {
                 uint64_t duration = get_time() - start;
                 ActionStatistics::Instance().UpdateAct( "PgWrite", duration );
                 HandleStatus( s, orgststr );
                 buffer.reset();
                 ending.reset();
                 closing.reset();
               } );
      }
      else if( action == "Sync" ) // sync action
      {
        uint16_t timeout = GetSyncArgs();
        std::string tmp( orgststr );
        Async( Sync( file, timeout ) >>
               [tmp, ending, closing, start]( XRootDStatus &s ) mutable
               {
                 uint64_t duration = get_time() - start;
                 ActionStatistics::Instance().UpdateAct( "Sync", duration );
                 HandleStatus( s, tmp );
                 ending.reset();
                 closing.reset();
               } );
      }
      else if( action == "Truncate" ) // truncate action
      {
        uint64_t size;
        uint16_t timeout;
        std::tie( size, timeout ) = GetTruncateArgs();
        std::string tmp( orgststr );
        Async( Truncate( file, size, timeout ) >>
               [tmp, ending, closing, start]( XRootDStatus &s ) mutable
               {
                 uint64_t duration = get_time() - start;
                 ActionStatistics::Instance().UpdateAct( "Truncate", duration );
                 HandleStatus( s, tmp );
                 ending.reset();
                 closing.reset();
               } );
      }
      else if( action == "VectorRead" ) // vector read action
      {
        ChunkList chunks;
        uint16_t  timeout;
        std::tie( chunks, timeout ) = GetVectorReadArgs();
        std::string tmp( orgststr );
        Async( VectorRead( file, chunks, timeout ) >>
               [chunks, tmp, ending, closing, start]( XRootDStatus &s, VectorReadInfo &r ) mutable
               {
                 uint64_t duration = get_time() - start;
                 ActionStatistics::Instance().UpdateAct( "VectorRead", duration );
                 HandleStatus( s, tmp );
                 for( auto &ch : chunks )
                   delete[] (char*)ch.buffer;
                 ending.reset();
                 closing.reset();
               } );

      }
      else if( action == "VectorWrite" ) // vector write
      {
        ChunkList chunks;
        uint16_t  timeout;
        std::tie( chunks, timeout ) = GetVectorWriteArgs();
        std::string tmp( orgststr );
        Async( VectorWrite( file, chunks, timeout ) >>
               [chunks, tmp, ending, closing, start]( XRootDStatus &s ) mutable
               {
                 uint64_t duration = get_time() - start;
                 ActionStatistics::Instance().UpdateAct( "VectorWrite", duration );
                 HandleStatus( s, tmp );
                 for( auto &ch : chunks )
                   delete[] (char*)ch.buffer;
                 ending.reset();
                 closing.reset();
               } );
      }
      else
      {
        DefaultEnv::GetLog()->Warning( AppMsg, "Cannot replyt %s action.", action.c_str() );
      }
    }

  private:

    //--------------------------------------------------------------------------
    //! Handle response status
    //--------------------------------------------------------------------------
    static void HandleStatus( XRootDStatus &response, const std::string &orgstr )
    {
      std::string rspstr = response.ToString();
      if( rspstr == orgstr )
      {
        DefaultEnv::GetLog()->Warning( AppMsg, "We were expecting status: %s, but "
                                       "received: %s", orgstr.c_str(), rspstr.c_str() );
      }
    }

    //--------------------------------------------------------------------------
    //! Parse arguments for open
    //--------------------------------------------------------------------------
    std::tuple<std::string, OpenFlags::Flags, Access::Mode, uint16_t> GetOpenArgs()
    {
      std::vector<std::string> tokens;
      Utils::splitString( tokens, args, ";" );
      if( tokens.size() != 4 )
        throw std::invalid_argument( "Failed to parse open arguments." );
      std::string url = tokens[0];
      OpenFlags::Flags flags = static_cast<OpenFlags::Flags>( std::stoul( tokens[1] ) );
      Access::Mode mode = static_cast<Access::Mode>( std::stoul( tokens[2] ) );
      uint16_t timeout = static_cast<uint16_t>( std::stoul( tokens[3] ) );
      return std::make_tuple( url, flags, mode, timeout );
    }

    //--------------------------------------------------------------------------
    //! Parse arguments for close
    //--------------------------------------------------------------------------
    uint16_t GetCloseArgs()
    {
      return static_cast<uint16_t>( std::stoul( args ) );
    }

    std::tuple<bool, uint16_t> GetStatArgs()
    {
      std::vector<std::string> tokens;
      Utils::splitString( tokens, args, ";" );
      if( tokens.size() != 2 )
        throw std::invalid_argument( "Failed to parse stat arguments." );
      bool force = ( tokens[0] == "true" );
      uint16_t timeout = static_cast<uint16_t>( std::stoul( tokens[1] ) );
      return std::make_tuple( force, timeout );
    }

    //--------------------------------------------------------------------------
    //! Parse arguments for read
    //--------------------------------------------------------------------------
    std::tuple<uint64_t, buffer_t, uint16_t> GetReadArgs()
    {
      std::vector<std::string> tokens;
      Utils::splitString( tokens, args, ";" );
      if( tokens.size() != 3 )
        throw std::invalid_argument( "Failed to parse read arguments." );
      uint64_t offset = std::stoull( tokens[0] );
      uint32_t length = std::stoul( tokens[1] );
      auto buffer = std::make_shared<std::vector<char>>( length, 'A' );
      uint16_t timeout = static_cast<uint16_t>( std::stoul( tokens[2] ) );
      return std::make_tuple( offset, buffer, timeout );
    }

    //--------------------------------------------------------------------------
    //! Parse arguments for pgread
    //--------------------------------------------------------------------------
    inline std::tuple<uint64_t, buffer_t, uint16_t> GetPgReadArgs()
    {
      return GetReadArgs();
    }

    //--------------------------------------------------------------------------
    //! Parse arguments for write
    //--------------------------------------------------------------------------
    inline std::tuple<uint64_t, buffer_t, uint16_t> GetWriteArgs()
    {
      return GetReadArgs();
    }

    //--------------------------------------------------------------------------
    //! Parse arguments for pgwrite
    //--------------------------------------------------------------------------
    inline std::tuple<uint64_t, buffer_t, uint16_t> GetPgWriteArgs()
    {
      return GetReadArgs();
    }

    //--------------------------------------------------------------------------
    //! Parse arguments for sync
    //--------------------------------------------------------------------------
    uint16_t GetSyncArgs()
    {
      return static_cast<uint16_t>( std::stoul( args ) );
    }

    //--------------------------------------------------------------------------
    //! Parse arguments for truncate
    //--------------------------------------------------------------------------
    std::tuple<uint64_t, uint16_t> GetTruncateArgs()
    {
      std::vector<std::string> tokens;
      Utils::splitString( tokens, args, ";" );
      if( tokens.size() != 2 )
        throw std::invalid_argument( "Failed to parse truncate arguments." );
      uint64_t size = std::stoull( tokens[0] );
      uint16_t timeout = static_cast<uint16_t>( std::stoul( tokens[1] ) );
      return std::make_tuple( size, timeout );
    }

    //--------------------------------------------------------------------------
    //! Parse arguments for vector read
    //--------------------------------------------------------------------------
    std::tuple<ChunkList, uint16_t> GetVectorReadArgs()
    {
      std::vector<std::string> tokens;
      Utils::splitString( tokens, args, ";" );
      ChunkList chunks;
      for( size_t i = 0; i < tokens.size() - 1; i += 2 )
      {
        uint64_t offset = std::stoull( tokens[i] );
        uint32_t length = std::stoul( tokens[i+1] );
        char*    buffer = new char[length];
        memset( buffer, 'A', length );
        chunks.emplace_back( offset, length, buffer );
      }
      uint16_t timeout = static_cast<uint16_t>( std::stoul( tokens.back() ) );
      return std::make_tuple( std::move( chunks ), timeout );
    }

    //--------------------------------------------------------------------------
    //! Parse arguments for vector write
    //--------------------------------------------------------------------------
    inline std::tuple<ChunkList, uint16_t> GetVectorWriteArgs()
    {
      return GetVectorReadArgs();
    }

    File              &file;     //< the file object
    const std::string  action;   //< the action to be executed
    const std::string  args;     //< arguments for the operation
    std::string        orgststr; //< the original response status of the action
    uint64_t           orgstart; //< original start time
    uint64_t           orgstop;  //< original stop time
};

//------------------------------------------------------------------------------
//! List of actions: start time - action
//------------------------------------------------------------------------------
using action_list = std::multimap<uint64_t, ActionExecutor>;

//------------------------------------------------------------------------------
//! Parse input file
//! @param path : path to the input csv file
//------------------------------------------------------------------------------
std::unordered_map<File*, action_list> ParseInput( const std::string &path )
{
  std::unordered_map<File*, action_list> result;
  std::ifstream input( path, std::ifstream::in );
  std::string line;
  std::unordered_map<uint64_t, File*> files;

  while( input.good() )
  {
    std::getline( input, line );
    if( line.empty() )
      continue;
    std::vector<std::string> tokens; tokens.reserve( 7 );
    XrdCl::Utils::splitString( tokens, line, "," );
    if( tokens.size() == 6 )
      tokens.emplace_back();
    if( tokens.size() != 7 )
    {
      throw std::invalid_argument( "Invalid input file format." );
    }

    uint64_t    id     = std::stoull( tokens[0] ); // file object ID
    std::string action = tokens[1];                // action name (e.g. Open)
    uint64_t    start  = std::stoull( tokens[2] ); // start time
    std::string args   = tokens[3];                // operation arguments
    uint64_t    stop   = std::stoull( tokens[4] ); // stop time
    std::string status = tokens[5];                // operation status
    std::string resp   = tokens[6];                // server response

    // update statistics
    ActionStatistics::Instance().UpdateOrg( action, stop - start );

    if( !files.count( id ) )
    {
      files[id] = new File( false );
      files[id]->SetProperty( "BundledClose", "true" );
    }
    result[files[id]].emplace( start, ActionExecutor( *files[id], action, args, status, resp, start, stop ) );
  }

  return result;
}

//------------------------------------------------------------------------------
//! Execute list of actions against given file
//! @param file    : the file object
//! @param actions : list of actions to be executed
//! @return        : thread that will executed the list of actions
//------------------------------------------------------------------------------
std::thread ExecuteActions( std::unique_ptr<File> file, action_list &&actions )
{
  std::thread t( [file{ std::move( file ) }, actions{ std::move( actions ) }]() mutable
      {
        XrdSysSemaphore endsem( 0 );
        XrdSysSemaphore closesem( 0 );
        auto ending = std::make_shared<barrier_t>( endsem );
        auto closing = std::make_shared<barrier_t>( closesem );
        auto prevstop = actions.begin()->first;
        for( auto &p : actions )
        {
          if( p.first > prevstop )
            std::this_thread::sleep_for( std::chrono::seconds( p.first - prevstop ) );
          prevstop = p.first;
          auto &action = p.second;
          mytimer_t timer;
          action.Execute( ending, closing );
          uint64_t duration = timer.elapsed();
          prevstop += duration;
        }
        ending.reset();
        closing.reset();
        endsem.Wait();
        file.reset();
      } );
  return t;
}

}

int main( int argc, char **argv )
{
  if( argc != 2 )
  {
    std::cout << "Error: wrong number of arguments.\n";
    std::cout << "\nUsage:   xrdreplay <file>\n";
    return 1;
  }

  std::string path( argv[1] );
  try
  {
    auto actions = XrdCl::ParseInput( path ); // parse the input file
    std::vector<std::thread> threads;
    threads.reserve( actions.size() );
    for( auto &action : actions )
    {
      // execute list of actions against file object
      threads.emplace_back( ExecuteActions( std::unique_ptr<XrdCl::File>( action.first ), std::move( action.second ) ) );
    }
    for( auto &t : threads ) // wait until we are done
      t.join();

    XrdCl::ActionStatistics::Instance().Print();
  }
  catch( const std::invalid_argument &ex )
  {
    std::cout << ex.what() << std::endl; // print parsing errors
    return 1;
  }

  return 0;
}



