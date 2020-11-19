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

#ifndef __XRD_CL_OPERATIONS_HH__
#define __XRD_CL_OPERATIONS_HH__

#include <memory>
#include <stdexcept>
#include <sstream>
#include <tuple>
#include <future>
#include "XrdCl/XrdClXRootDResponses.hh"
#include "XrdCl/XrdClOperationHandlers.hh"
#include "XrdCl/XrdClArg.hh"
#include "XrdCl/XrdClOperationTimeout.hh"
#include "XrdSys/XrdSysPthread.hh"

namespace XrdCl
{

  template<bool HasHndl> class Operation;

  class Pipeline;


  //----------------------------------------------------------------------------
  //! Type of the recovery function to be provided by the user
  //----------------------------------------------------------------------------
  typedef std::function<Operation<true>*(const XRootDStatus&)>  rcvry_func;

  //----------------------------------------------------------------------------
  //! Wrapper for ResponseHandler, used only internally to run next operation
  //! after previous one is finished
  //----------------------------------------------------------------------------
  class PipelineHandler: public ResponseHandler
  {
      template<bool> friend class Operation;

    public:

      //------------------------------------------------------------------------
      //! Constructor.
      //!
      //! @param handler  : the handler of our operation
      //! @param recovery : the recovery procedure for our operation
      //------------------------------------------------------------------------
      PipelineHandler( ResponseHandler   *handler,
                       rcvry_func       &&recovery );

      //------------------------------------------------------------------------
      //! Default Constructor.
      //------------------------------------------------------------------------
      PipelineHandler( rcvry_func &&recovery ) : recovery( std::move( recovery ) )
      {
      }

      //------------------------------------------------------------------------
      //! Callback function.
      //------------------------------------------------------------------------
      void HandleResponseWithHosts( XRootDStatus *status, AnyObject *response,
          HostList *hostList );

      //------------------------------------------------------------------------
      //! Callback function.
      //------------------------------------------------------------------------
      void HandleResponse( XRootDStatus *status, AnyObject *response );

      //------------------------------------------------------------------------
      //! Destructor.
      //------------------------------------------------------------------------
      ~PipelineHandler()
      {
      }

      //------------------------------------------------------------------------
      //! Add new operation to the pipeline
      //!
      //! @param operation  :  operation to add
      //------------------------------------------------------------------------
      void AddOperation( Operation<true> *operation );

      //------------------------------------------------------------------------
      //! Set workflow to this and all next handlers. In the last handler
      //! it is used to finish workflow execution
      //!
      //! @param  prms         :  a promis that the pipeline will have a result
      //! @param  final        :  a callable that should be called at the end of
      //!                         pipeline
      //------------------------------------------------------------------------
      void Assign( const Timeout                            &timeout,
                   std::promise<XRootDStatus>                prms,
                   std::function<void(const XRootDStatus&)>  final,
                   Operation<true>                          *opr );

    private:

      //------------------------------------------------------------------------
      //! Callback function implementation;
      //------------------------------------------------------------------------
      void HandleResponseImpl( XRootDStatus *status, AnyObject *response,
          HostList *hostList = nullptr );

      inline void dealloc( XRootDStatus *status, AnyObject *response,
          HostList *hostList )
      {
        delete status;
        delete response;
        delete hostList;
      }

      //------------------------------------------------------------------------
      //! The handler of our operation
      //------------------------------------------------------------------------
      std::unique_ptr<ResponseHandler> responseHandler;

      //------------------------------------------------------------------------
      //! The operation the handler is assigned to
      //------------------------------------------------------------------------
      std::unique_ptr<Operation<true>> currentOperation;

      //------------------------------------------------------------------------
      //! Next operation in the pipeline
      //------------------------------------------------------------------------
      std::unique_ptr<Operation<true>> nextOperation;

      //------------------------------------------------------------------------
      //! Pipeline timeout
      //------------------------------------------------------------------------
      Timeout timeout;

      //------------------------------------------------------------------------
      //! The promise that there will be a result (traveling along the pipeline)
      //------------------------------------------------------------------------
      std::promise<XRootDStatus> prms;

      //------------------------------------------------------------------------
      //! The lambda/function/functor that should be called at the end of the
      //! pipeline (traveling along the pipeline)
      //------------------------------------------------------------------------
      std::function<void(const XRootDStatus&)> final;

      //------------------------------------------------------------------------
      //! The recovery routine for the respective operation
      //------------------------------------------------------------------------
      rcvry_func recovery;
  };

  //----------------------------------------------------------------------------
  //! Operation template. An Operation is a once-use-only object - once executed
  //! by a Workflow engine it is invalidated. Also if used as an argument for
  //! >> or | the original object gets invalidated.
  //!
  //! @arg HasHndl : true if operation has a handler, false otherwise
  //----------------------------------------------------------------------------
  template<bool HasHndl>
  class Operation
  {
      // Declare friendship between templates
      template<bool>
      friend class Operation;

      friend std::future<XRootDStatus> Async( Pipeline, uint16_t );

      friend class Pipeline;
      friend class PipelineHandler;

    public:

      //------------------------------------------------------------------------
      //! Constructor
      //------------------------------------------------------------------------
      Operation() : valid( true )
      {
      }

      //------------------------------------------------------------------------
      //! Move constructor between template instances.
      //------------------------------------------------------------------------
      template<bool from>
      Operation( Operation<from> && op ) :
          handler( std::move( op.handler ) ), valid( true )
      {
        if( !op.valid ) throw std::invalid_argument( "Cannot construct "
            "Operation from an invalid Operation!" );
        op.valid = false;
      }

      //------------------------------------------------------------------------
      //! Destructor
      //------------------------------------------------------------------------
      virtual ~Operation()
      {
      }

      //------------------------------------------------------------------------
      //! Name of the operation.
      //------------------------------------------------------------------------
      virtual std::string ToString() = 0;

      //------------------------------------------------------------------------
      //! Move current object into newly allocated instance
      //!
      //! @return : the new instance
      //------------------------------------------------------------------------
      virtual Operation<HasHndl>* Move() = 0;

      //------------------------------------------------------------------------
      //! Move current object into newly allocated instance, and convert
      //! it into 'handled' operation.
      //!
      //! @return : the new instance
      //------------------------------------------------------------------------
      virtual Operation<true>* ToHandled() = 0;

    protected:

      //------------------------------------------------------------------------
      //! Run operation
      //!
      //! @param prom   : the promise that we will have a result
      //! @param final  : the object to call at the end of pipeline
      //! @param args   : forwarded arguments
      //! @param bucket : number of the bucket with arguments
      //!
      //! @return       : stOK if operation was scheduled for execution
      //!                 successfully, stError otherwise
      //------------------------------------------------------------------------
      void Run( Timeout                                   timeout,
                std::promise<XRootDStatus>                prms,
                std::function<void(const XRootDStatus&)>  final )
      {
        static_assert(HasHndl, "Only an operation that has a handler can be assigned to workflow");
        handler->Assign( timeout, std::move( prms ), std::move( final ), this );
        XRootDStatus st;
        try
        {
          st = RunImpl( timeout );
        }
        catch( operation_expired& ex )
        {
          st = XRootDStatus( stError, errOperationExpired );
        }
        if( st.IsOK() ) handler.release();
        else
          ForceHandler( st );
      }

      //------------------------------------------------------------------------
      //! Run the actual operation
      //!
      //! @param params :  container with parameters forwarded from
      //!                  previous operation
      //! @return       :  status of the operation
      //! @param bucket : number of the bucket with arguments
      //------------------------------------------------------------------------
      virtual XRootDStatus RunImpl( uint16_t timeout ) = 0;

      //------------------------------------------------------------------------
      //! Handle error caused by missing parameter
      //!
      //! @param err : error object
      //!
      //! @return    : default operation status (actual status containing
      //!              error information is passed to the handler)
      //------------------------------------------------------------------------
      void ForceHandler( const XRootDStatus &status )
      {
        handler->HandleResponse( new XRootDStatus( status ), nullptr );
        // HandleResponse already freed the memory so we have to
        // release the unique pointer
        handler.release();
      }

      //------------------------------------------------------------------------
      //! Add next operation in the pipeline
      //!
      //! @param op : operation to add
      //------------------------------------------------------------------------
      void AddOperation( Operation<true> *op )
      {
        if( handler )
          handler->AddOperation( op );
      }

      //------------------------------------------------------------------------
      //! Operation handler
      //------------------------------------------------------------------------
      std::unique_ptr<PipelineHandler> handler;

      //------------------------------------------------------------------------
      //! Flag indicating if it is a valid object
      //------------------------------------------------------------------------
      bool valid;
  };

  //----------------------------------------------------------------------------
  //! An exception type used to (force) stop a pipeline
  //----------------------------------------------------------------------------
  struct StopPipeline
  {
    StopPipeline( const XRootDStatus &status ) : status( status ) { }
    XRootDStatus status;
  };

  //----------------------------------------------------------------------------
  //! An exception type used to repeat an operation
  //----------------------------------------------------------------------------
  struct RepeatOpeation { };

  //----------------------------------------------------------------------------
  //! A wrapper around operation pipeline. A Pipeline is a once-use-only
  //! object - once executed by a Workflow engine it is invalidated.
  //!
  //! Takes ownership of given operation pipeline (which is in most would
  //! be a temporary object)
  //----------------------------------------------------------------------------
  class Pipeline
  {
      template<bool> friend class ParallelOperation;
      friend std::future<XRootDStatus> Async( Pipeline, uint16_t );

    public:

      //------------------------------------------------------------------------
      //! Constructor
      //------------------------------------------------------------------------
      Pipeline( Operation<true> *op ) :
          operation( op->Move() )
      {

      }

      //------------------------------------------------------------------------
      //! Constructor
      //------------------------------------------------------------------------
      Pipeline( Operation<true> &op ) :
          operation( op.Move() )
      {

      }

      //------------------------------------------------------------------------
      //! Constructor
      //------------------------------------------------------------------------
      Pipeline( Operation<true> &&op ) :
          operation( op.Move() )
      {

      }

      Pipeline( Operation<false> *op ) :
          operation( op->ToHandled() )
      {

      }

      //------------------------------------------------------------------------
      //! Constructor
      //------------------------------------------------------------------------
      Pipeline( Operation<false> &op ) :
          operation( op.ToHandled() )
      {

      }

      //------------------------------------------------------------------------
      //! Constructor
      //------------------------------------------------------------------------
      Pipeline( Operation<false> &&op ) :
          operation( op.ToHandled() )
      {

      }

      Pipeline( Pipeline &&pipe ) :
          operation( std::move( pipe.operation ) )
      {

      }

      //------------------------------------------------------------------------
      //! Constructor
      //------------------------------------------------------------------------
      Pipeline& operator=( Pipeline &&pipe )
      {
        operation = std::move( pipe.operation );
        return *this;
      }

      //------------------------------------------------------------------------
      //! Conversion to Operation<true>
      //!
      //! @throws : std::logic_error if pipeline is invalid
      //------------------------------------------------------------------------
      operator Operation<true>&()
      {
        if( !bool( operation ) ) throw std::logic_error( "Invalid pipeline." );
        return *operation.get();
      }

      //------------------------------------------------------------------------
      //! Conversion to boolean
      //!
      //! @return : true if it's a valid pipeline, false otherwise
      //------------------------------------------------------------------------
      operator bool()
      {
        return bool( operation );
      }

      //------------------------------------------------------------------------
      //! Stop the current pipeline
      //!
      //! @param status : the final status for the pipeline
      //------------------------------------------------------------------------
      inline static void Stop( const XRootDStatus &status = XrdCl::XRootDStatus() )
      {
        throw StopPipeline( status );
      }

      //------------------------------------------------------------------------
      //! Repeat current operation
      //------------------------------------------------------------------------
      inline static void Repeat()
      {
        throw RepeatOpeation();
      }

    private:

      //------------------------------------------------------------------------
      //! Member access declaration, provides access to the underlying
      //! operation.
      //!
      //! @return : pointer to the underlying
      //------------------------------------------------------------------------
      Operation<true>* operator->()
      {
        return operation.get();
      }

      //------------------------------------------------------------------------
      //! Schedules the underlying pipeline for execution.
      //!
      //! @param timeout : pipeline timeout value
      //! @param final   : to be called at the end of the pipeline
      //------------------------------------------------------------------------
      void Run( Timeout timeout, std::function<void(const XRootDStatus&)> final = nullptr )
      {
        if( ftr.valid() )
          throw std::logic_error( "Pipeline is already running" );

        // a promise that the pipe will have a result
        std::promise<XRootDStatus> prms;
        ftr = prms.get_future();

        Operation<true> *opr = operation.release();
        opr->Run( timeout, std::move( prms ), std::move( final ) );
      }

      //------------------------------------------------------------------------
      //! First operation in the pipeline
      //------------------------------------------------------------------------
      std::unique_ptr<Operation<true>> operation;

      //------------------------------------------------------------------------
      //! The future result of the pipeline
      //------------------------------------------------------------------------
      std::future<XRootDStatus> ftr;

  };

  //----------------------------------------------------------------------------
  //! Helper function, schedules execution of given pipeline
  //!
  //! @param pipeline : the pipeline to be executed
  //! @param timeout  : the pipeline timeout
  //!
  //! @return         : future status of the operation
  //----------------------------------------------------------------------------
  inline std::future<XRootDStatus> Async( Pipeline pipeline, uint16_t timeout = 0 )
  {
    pipeline.Run( timeout );
    return std::move( pipeline.ftr );
  }

  //----------------------------------------------------------------------------
  //! Helper function, schedules execution of given pipeline and waits for
  //! the status
  //!
  //! @param pipeline : the pipeline to be executed
  //!
  //! @return         : status of the operation
  //----------------------------------------------------------------------------
  inline XRootDStatus WaitFor( Pipeline pipeline, uint16_t timeout = 0 )
  {
    return Async( std::move( pipeline ), timeout ).get();
  }

  //----------------------------------------------------------------------------
  //! Concrete Operation template
  //! Defines | and >> operator as well as operation arguments.
  //!
  //! @arg Derived : the class that derives from this template (CRTP)
  //! @arg HasHndl : true if operation has a handler, false otherwise
  //! @arg Args    : operation arguments
  //----------------------------------------------------------------------------
  template<template<bool> class Derived, bool HasHndl, typename HdlrFactory, typename ... Args>
  class ConcreteOperation: public Operation<HasHndl>
  {
      template<template<bool> class, bool, typename, typename ...>
      friend class ConcreteOperation;

    public:

      //------------------------------------------------------------------------
      //! Constructor
      //!
      //! @param args : operation arguments
      //------------------------------------------------------------------------
      ConcreteOperation( Args&&... args ) : args( std::tuple<Args...>( std::move( args )... ) ),
                                            timeout( 0 )
      {
        static_assert( !HasHndl, "It is only possible to construct operation without handler" );
      }

      //------------------------------------------------------------------------
      //! Move constructor from other states
      //!
      //! @arg from : state from which the object is being converted
      //!
      //! @param op : the object that is being converted
      //------------------------------------------------------------------------
      template<bool from>
      ConcreteOperation( ConcreteOperation<Derived, from, HdlrFactory, Args...> && op ) :
        Operation<HasHndl>( std::move( op ) ), args( std::move( op.args ) ), timeout( 0 )
      {
      }

      //------------------------------------------------------------------------
      //! Adds ResponseHandler/function/functor/lambda/future handler for
      //! the operation.
      //!
      //! Note: due to reference collapsing this covers both l-value and
      //!       r-value references.
      //!
      //! @param func : function/functor/lambda
      //------------------------------------------------------------------------
      template<typename Hdlr>
      Derived<true> operator>>( Hdlr &&hdlr )
      {
        return this->StreamImpl( HdlrFactory::Create( hdlr ) );
      }

      //------------------------------------------------------------------------
      //! Creates a pipeline of 2 or more operations
      //!
      //! @param op  : operation to add
      //!
      //! @return    : handled operation
      //------------------------------------------------------------------------
      Derived<true> operator|( Operation<true> &op )
      {
        return PipeImpl( *this, op );
      }

      //------------------------------------------------------------------------
      //! Creates a pipeline of 2 or more operations
      //!
      //! @param op :  operation to add
      //!
      //! @return   :  handled operation
      //------------------------------------------------------------------------
      Derived<true> operator|( Operation<true> &&op )
      {
        return PipeImpl( *this, op );
      }

      //------------------------------------------------------------------------
      //! Creates a pipeline of 2 or more operations
      //!
      //! @param op   operation to add
      //!
      //! @return     handled operation
      //------------------------------------------------------------------------
      Derived<true> operator|( Operation<false> &op )
      {
        return PipeImpl( *this, op );
      }

      //------------------------------------------------------------------------
      //! Creates a pipeline of 2 or more operations
      //!
      //! @param op  : operation to add
      //!
      //! @return    : handled operation
      //------------------------------------------------------------------------
      Derived<true> operator|( Operation<false> &&op )
      {
        return PipeImpl( *this, op );
      }

      //------------------------------------------------------------------------
      //! Set recovery procedure in case the operation fails
      //------------------------------------------------------------------------
      Derived<HasHndl> Recovery( rcvry_func recovery )
      {
        this->recovery = std::move( recovery );
        return Transform<HasHndl>();
      }

      //------------------------------------------------------------------------
      //! Move current object into newly allocated instance
      //!
      //! @return : the new instance
      //------------------------------------------------------------------------
      inline Operation<HasHndl>* Move()
      {
        Derived<HasHndl> *me = static_cast<Derived<HasHndl>*>( this );
        return new Derived<HasHndl>( std::move( *me ) );
      }

      //------------------------------------------------------------------------
      //! Transform operation to handled
      //!
      //! @return Operation<true>&
      //------------------------------------------------------------------------
      inline Operation<true>* ToHandled()
      {
        this->handler.reset( new PipelineHandler( std::move( this->recovery ) ) );
        Derived<HasHndl> *me = static_cast<Derived<HasHndl>*>( this );
        return new Derived<true>( std::move( *me ) );
      }

      //------------------------------------------------------------------------
      //! Set operation timeout
      //------------------------------------------------------------------------
      Derived<HasHndl> Timeout( uint16_t timeout )
      {
        this->timeout = timeout;
        return std::move( *this );
      }

    protected:

      //------------------------------------------------------------------------
      //! Transform into a new instance with desired state
      //!
      //! @return : new instance in the desired state
      //------------------------------------------------------------------------
      template<bool to>
      inline Derived<to> Transform()
      {
        Derived<HasHndl> *me = static_cast<Derived<HasHndl>*>( this );
        return Derived<to>( std::move( *me ) );
      }

      //------------------------------------------------------------------------
      //! Implements operator>> functionality
      //!
      //! @param h  :  handler to be added
      //! @
      //! @return   :  return an instance of Derived<true>
      //------------------------------------------------------------------------
      inline Derived<true> StreamImpl( ResponseHandler *handler )
      {
        static_assert( !HasHndl, "Operator >> is available only for operation without handler" );
        this->handler.reset( new PipelineHandler( handler, std::move( this->recovery ) ) );
        return Transform<true>();
      }

      //------------------------------------------------------------------------
      //! Implements operator| functionality
      //!
      //! @param me  :  reference to myself (*this)
      //! @param op  :  reference to the other operation
      //!
      //! @return    :  move-copy of myself
      //------------------------------------------------------------------------
      inline static
      Derived<true> PipeImpl( ConcreteOperation<Derived, true, HdlrFactory,
          Args...> &me, Operation<true> &op )
      {
        me.AddOperation( op.Move() );
        return me.template Transform<true>();
      }

      //------------------------------------------------------------------------
      //! Implements operator| functionality
      //!
      //! @param me  :  reference to myself (*this)
      //! @param op  :  reference to the other operation
      //!
      //! @return    :  move-copy of myself
      //------------------------------------------------------------------------
      inline static
      Derived<true> PipeImpl( ConcreteOperation<Derived, true, HdlrFactory,
          Args...> &me, Operation<false> &op )
      {
        me.AddOperation( op.ToHandled() );
        return me.template Transform<true>();
      }

      //------------------------------------------------------------------------
      //! Implements operator| functionality
      //!
      //! @param me  :  reference to myself (*this)
      //! @param op  :  reference to the other operation
      //!
      //! @return    :  move-copy of myself
      //------------------------------------------------------------------------
      inline static
      Derived<true> PipeImpl( ConcreteOperation<Derived, false, HdlrFactory,
          Args...> &me, Operation<true> &op )
      {
        me.handler.reset( new PipelineHandler( std::move( me.recovery ) ) );
        me.AddOperation( op.Move() );
        return me.template Transform<true>();
      }

      //------------------------------------------------------------------------
      //! Implements operator| functionality
      //!
      //! @param me  :  reference to myself (*this)
      //! @param op  :  reference to the other operation
      //!
      //! @return    :  move-copy of myself
      //------------------------------------------------------------------------
      inline static
      Derived<true> PipeImpl( ConcreteOperation<Derived, false, HdlrFactory,
          Args...> &me, Operation<false> &op )
      {
        me.handler.reset( new PipelineHandler( std::move( me.recovery ) ) );
        me.AddOperation( op.ToHandled() );
        return me.template Transform<true>();
      }

      //------------------------------------------------------------------------
      //! Operation arguments
      //------------------------------------------------------------------------
      std::tuple<Args...> args;

      //------------------------------------------------------------------------
      //! The recovery routine for this operation
      //------------------------------------------------------------------------
      rcvry_func recovery;

      //------------------------------------------------------------------------
      //! Operation timeout
      //------------------------------------------------------------------------
      uint16_t timeout;
    };
}

#endif // __XRD_CL_OPERATIONS_HH__
