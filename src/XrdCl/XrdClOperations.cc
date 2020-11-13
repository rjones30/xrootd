//------------------------------------------------------------------------------
// Copyright (c) 2011-2017 by European Organization for Nuclear Research (CERN)
// Author: Krzysztof Jamrog <krzysztof.piotr.jamrog@cern.ch>,
//         Michal Simon <michal.simon@cern.ch>
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

#include <stdexcept>
#include <string>
#include "XrdCl/XrdClOperations.hh"
#include "XrdCl/XrdClLog.hh"
#include "XrdCl/XrdClDefaultEnv.hh"
#include "XrdCl/XrdClConstants.hh"

namespace XrdCl
{

  //----------------------------------------------------------------------------
  // OperationHandler Constructor.
  //----------------------------------------------------------------------------
  PipelineHandler::PipelineHandler( ResponseHandler  *handler,
                                    rcvry_func      &&recovery ) :
      responseHandler( handler ),
      recovery( std::move( recovery ) )
  {
  }

  //----------------------------------------------------------------------------
  // OperationHandler::AddOperation
  //----------------------------------------------------------------------------
  void PipelineHandler::AddOperation( Operation<true> *operation )
  {
    if( nextOperation )
    {
      nextOperation->AddOperation( operation );
    }
    else
    {
      nextOperation.reset( operation );
    }
  }

  //----------------------------------------------------------------------------
  // OperationHandler::HandleResponseImpl
  //----------------------------------------------------------------------------
  void PipelineHandler::HandleResponseImpl( XRootDStatus *status,
      AnyObject *response, HostList *hostList )
  {
    std::unique_ptr<PipelineHandler> myself( this );

    // We need to copy status as original status object is destroyed in
    // HandleResponse function
    XRootDStatus st( *status );
    if( responseHandler )
    {
      try
      {
        responseHandler->HandleResponseWithHosts( status, response, hostList );
      }
      catch( const StopPipeline &stop )
      {
        if( final ) final( stop.status );
        prms.set_value( stop.status );
        return;
      }
      catch( const RepeatOpeation &repeat )
      {
        Operation<true> *opr = currentOperation.release();
        opr->handler.reset( myself.release() );
        opr->Run( timeout, std::move( prms ), std::move( final ) );
        return;
      }
    }
    else
      dealloc( status, response, hostList );

    if( !st.IsOK() && recovery )
    {
      try
      {
        std::unique_ptr<Operation<true>> op( recovery( st ) );
        op->AddOperation( nextOperation.release() );
        op->Run( timeout, std::move( prms ), std::move( final ) );
        return;
      }
      catch( const std::exception &ex )
      {
        // just proceed as if there would be no recovery routine at all
      }
    }

    if( !st.IsOK() || !nextOperation )
    {
      if( final ) final( st );
      prms.set_value( st );
      return;
    }

    Operation<true> *opr = nextOperation.release();
    opr->Run( timeout, std::move( prms ), std::move( final ) );
  }

  //----------------------------------------------------------------------------
  // OperationHandler::HandleResponseWithHosts
  //----------------------------------------------------------------------------
  void PipelineHandler::HandleResponseWithHosts( XRootDStatus *status,
      AnyObject *response, HostList *hostList )
  {
    HandleResponseImpl( status, response, hostList );
  }

  //----------------------------------------------------------------------------
  // OperationHandler::HandleResponse
  //----------------------------------------------------------------------------
  void PipelineHandler::HandleResponse( XRootDStatus *status,
      AnyObject *response )
  {
    HandleResponseImpl( status, response );
  }

  //----------------------------------------------------------------------------
  // OperationHandler::AssignToWorkflow
  //----------------------------------------------------------------------------
  void PipelineHandler::Assign( const Timeout                            &t,
                                std::promise<XRootDStatus>                p,
                                std::function<void(const XRootDStatus&)>  f,
                                Operation<true>                          *opr )
  {
    timeout = t;
    prms    = std::move( p );
    final   = std::move( f );
    currentOperation.reset( opr );
  }

}

