//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztañaga 2005-2006. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/interprocess for documentation.
//
//////////////////////////////////////////////////////////////////////////////
//
// Parts of the pthread code come from Boost Threads code:
//
//////////////////////////////////////////////////////////////////////////////
//
// Copyright (C) 2001-2003
// William E. Kempf
//
// Permission to use, copy, modify, distribute and sell this software
// and its documentation for any purpose is hereby granted without fee,
// provided that the above copyright notice appear in all copies and
// that both that copyright notice and this permission notice appear
// in supporting documentation.  William E. Kempf makes no representations
// about the suitability of this software for any purpose.
// It is provided "as is" without express or implied warranty.
//////////////////////////////////////////////////////////////////////////////

#ifndef BOOST_INTERPROCESS_RECURSIVE_MUTEX_HPP
#define BOOST_INTERPROCESS_RECURSIVE_MUTEX_HPP

#if (defined _MSC_VER) && (_MSC_VER >= 1200)
#  pragma once
#endif

#include <boost/interprocess/detail/config_begin.hpp>
#include <boost/interprocess/detail/workaround.hpp>

#include <boost/noncopyable.hpp>
#include <boost/interprocess/sync/interprocess_mutex.hpp>
#include <boost/interprocess/sync/interprocess_recursive_mutex.hpp>
#include <boost/date_time/posix_time/ptime.hpp>
#include <assert.h>

#if (defined BOOST_WINDOWS) && !(defined BOOST_DISABLE_WIN32)
#  include <boost/interprocess/sync/win32/win32_sync_primitives.hpp>
#else    //#if (defined BOOST_WINDOWS) && !(defined BOOST_DISABLE_WIN32)
#  include <pthread.h>
#  include <errno.h>
#  include <boost/interprocess/sync/posix/pthread_helpers.hpp>   
#endif   //#if (defined BOOST_WINDOWS) && !(defined BOOST_DISABLE_WIN32)

/*!\file
   Describes interprocess_recursive_mutex and shared_recursive_try_mutex classes
*/

namespace boost {

namespace interprocess {

/*!Wraps a interprocess_mutex that can be placed in shared memory and can be 
   shared between processes. Allows several locking calls by the same 
   process. Allows timed lock tries*/
class interprocess_recursive_mutex : private boost::noncopyable
{
public:
   interprocess_recursive_mutex();
  ~interprocess_recursive_mutex();

   /*!Locks interprocess_mutex, sleeps when interprocess_mutex is already locked.
      Throws interprocess_exception if a severe error is found*/
   void lock(void);

   /*! Tries to lock the interprocess_mutex, returns false when interprocess_mutex 
      is already locked, returns true when success.
      Throws interprocess_exception if a severe error is found*/
   bool try_lock(void);

   /*! Tries to lock the interprocess_mutex, if interprocess_mutex can't be locked before
      abs_time time, returns false.
      Throws interprocess_exception if a severe error is found*/
   bool timed_lock(const boost::posix_time::ptime &abs_time);

   /*! Unlocks the interprocess_mutex */
   void unlock(void);

   #if (defined BOOST_WINDOWS) && !(defined BOOST_DISABLE_WIN32)
   interprocess_mutex   m_shared_timed_mutex;
   unsigned int         m_nLockCount;
   unsigned long        m_nOwner;
   #else    //#if (defined BOOST_WINDOWS) && !(defined BOOST_DISABLE_WIN32)
   #if (_POSIX_VERSION >= 200112L || _XOPEN_VERSION >= 500) && defined _POSIX_TIMEOUTS
      pthread_mutex_t m_mut;   
   #else //#if (_POSIX_VERSION >= 200112L || _XOPEN_VERSION >= 500) && defined _POSIX_TIMEOUTS
      pthread_mutex_t   m_mut;
      pthread_cond_t    m_unlocked;
      pthread_t         m_thread_id;
      bool              m_valid_id;
      unsigned int      m_count;
   #endif   //#if (_POSIX_VERSION >= 200112L || _XOPEN_VERSION >= 500) && defined _POSIX_TIMEOUTS
   #endif   //#if (defined BOOST_WINDOWS) && !(defined BOOST_DISABLE_WIN32)
};

}  //namespace interprocess {

}  //namespace boost {

#if (defined BOOST_WINDOWS) && !(defined BOOST_DISABLE_WIN32)
#  include <boost/interprocess/sync/win32/interprocess_recursive_mutex.hpp>
#else 
#  include <boost/interprocess/sync/posix/interprocess_recursive_mutex.hpp>
#endif   //#if (defined BOOST_WINDOWS) && !(defined BOOST_DISABLE_WIN32)

#include <boost/interprocess/detail/config_end.hpp>

#endif   //BOOST_INTERPROCESS_RECURSIVE_MUTEX_HPP