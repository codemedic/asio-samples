//
// Copyright (c) 2010-2011 Marat Abrarov (abrarov@mail.ru)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef MA_TUTORIAL_ASYNC_BASE_HPP
#define MA_TUTORIAL_ASYNC_BASE_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/optional.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/noncopyable.hpp>
#include <boost/system/error_code.hpp>
#include <ma/config.hpp>
#include <ma/handler_storage.hpp>
#include <ma/handler_allocator.hpp>
#include <ma/bind_asio_handler.hpp>
#include <ma/context_alloc_handler.hpp>

#if defined(MA_HAS_RVALUE_REFS)
#include <utility>
#include <ma/type_traits.hpp>
#endif // defined(MA_HAS_RVALUE_REFS)

namespace ma
{
  namespace tutorial
  {    
    class async_base : private boost::noncopyable
    {
    private:
      typedef async_base this_type;

    public:
#if defined(MA_HAS_RVALUE_REFS)

#if defined(MA_BOOST_BIND_HAS_NO_MOVE_CONTRUCTOR)
      template <typename Handler>
      void async_do_something(Handler&& handler)
      {
        typedef typename ma::remove_cv_reference<Handler>::type handler_type;
        strand_.post(ma::make_context_alloc_handler2(std::forward<Handler>(handler), 
          forward_handler_binder<handler_type>(&this_type::begin_do_something<handler_type>, get_shared_base())));
      }
#else
      template <typename Handler>
      void async_do_something(Handler&& handler)
      {
        typedef typename ma::remove_cv_reference<Handler>::type handler_type;
        strand_.post(ma::make_context_alloc_handler2(std::forward<Handler>(handler), 
          boost::bind(&this_type::begin_do_something<handler_type>, get_shared_base(), _1)));
      }
#endif // defined(MA_BOOST_BIND_HAS_NO_MOVE_CONTRUCTOR)

#else  // defined(MA_HAS_RVALUE_REFS)
      template <typename Handler>
      void async_do_something(const Handler& handler)
      {
        strand_.post(ma::make_context_alloc_handler2(handler, 
          boost::bind(&this_type::begin_do_something<Handler>, get_shared_base(), _1)));
      }
#endif // defined(MA_HAS_RVALUE_REFS)

    protected:
      typedef boost::shared_ptr<this_type> async_base_ptr;

      async_base(boost::asio::io_service::strand& strand)
        : strand_(strand)
        , do_something_handler_(strand.get_io_service())
      {
      }

      ~async_base() 
      {
      }

      virtual async_base_ptr get_shared_base() = 0;

      virtual boost::optional<boost::system::error_code> do_something() = 0;

      void complete_do_something(const boost::system::error_code& error)
      {
        do_something_handler_.post(error);
      }

      bool has_do_something_handler() const
      {
        return do_something_handler_.has_target();
      }

    private:

#if defined(MA_HAS_RVALUE_REFS) && defined(MA_BOOST_BIND_HAS_NO_MOVE_CONTRUCTOR)
      template <typename Arg>
      class forward_handler_binder
      {
      private:
        typedef forward_handler_binder this_type;
        this_type& operator=(const this_type&);

      public:
        typedef void result_type;
        typedef void (async_base::*function_type)(const Arg&);

        template <typename AsyncBasePtr>
        forward_handler_binder(function_type function, AsyncBasePtr&& async_base)
          : function_(function)
          , async_base_(std::forward<AsyncBasePtr>(async_base))
        {
        }

        forward_handler_binder(this_type&& other)
          : function_(other.function_)
          , async_base_(std::move(other.async_base_))
        {
        }

        void operator()(const Arg& arg)
        {
          ((*async_base_).*function_)(arg);
        }

      private:
        function_type function_;
        async_base_ptr async_base_;          
      }; // class forward_handler_binder
#endif // defined(MA_HAS_RVALUE_REFS) && defined(MA_BOOST_BIND_HAS_NO_MOVE_CONTRUCTOR)

      template <typename Handler>
      void begin_do_something(const Handler& handler)
      {    
        if (boost::optional<boost::system::error_code> result = do_something())
        {
          strand_.get_io_service().post(ma::detail::bind_handler(handler, *result));
        }
        else
        {
          do_something_handler_.put(handler);
        }
      }

      boost::asio::io_service::strand& strand_;
      ma::handler_storage<boost::system::error_code> do_something_handler_;
    }; // class async_base

  } // namespace tutorial
} // namespace ma

#endif // MA_TUTORIAL_ASYNC_BASE_HPP
