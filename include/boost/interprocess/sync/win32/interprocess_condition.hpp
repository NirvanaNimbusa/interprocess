//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztañaga 2005-2006. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/interprocess for documentation.
//
//////////////////////////////////////////////////////////////////////////////

#include <boost/date_time/posix_time/posix_time_types.hpp>

namespace boost {

namespace interprocess {


inline interprocess_condition::interprocess_condition()
{
   m_command      = SLEEP;
   m_num_waiters  = 0;
}

inline interprocess_condition::~interprocess_condition()
{}


inline void interprocess_condition::notify_one()
{
   notify(NOTIFY_ONE);
}

inline void interprocess_condition::notify_all()
{
   notify(NOTIFY_ALL);
}

inline void interprocess_condition::notify(long command)
{
   //This interprocess_mutex guarantees that no other thread can enter to the 
   //do_timed_wait method logic, so that thread count will be
   //constant until the function writes a NOTIFY_ALL command.
   //It also guarantees that no other notification can be signaled 
   //on this interprocess_condition before this one ends
   using namespace boost::detail;
   m_enter_mut.lock();

   //Return if there are no waiters
   if(!winapi::exchange_and_add(&m_num_waiters, 0)) { 
      m_enter_mut.unlock();
      return;
   }

   //Notify that all threads should execute wait logic
   BOOST_INTERLOCKED_COMPARE_EXCHANGE((long*)&m_command, command, SLEEP);
   //The enter interprocess_mutex will rest locked until the last waiting thread unlocks it
}

inline void interprocess_condition::do_wait(interprocess_mutex &mut)
{
   do_timed_wait(false, boost::posix_time::ptime(), mut);
}

inline bool interprocess_condition::do_timed_wait
   (const boost::posix_time::ptime &abs_time, interprocess_mutex &mut)
{
   return do_timed_wait(true, abs_time, mut);
}

inline bool interprocess_condition::do_timed_wait(bool tout_enabled,
                                     const boost::posix_time::ptime &abs_time, 
                                     interprocess_mutex &mut)
{
   using namespace boost::detail;
   boost::posix_time::ptime now = 
      boost::posix_time::microsec_clock::universal_time();
   
   if(tout_enabled){
      if(now >= abs_time) return false;
   }

   //The enter interprocess_mutex guarantees that while executing a notification, 
   //no other thread can execute the do_timed_wait method. 
   {
      //---------------------------------------------------------------
      boost::interprocess::scoped_lock<interprocess_mutex> lock(m_enter_mut);
      //---------------------------------------------------------------
      //We increment the waiting thread count protected so that it will be
      //always constant when another thread enters the notification logic.
      //The increment marks this thread as "waiting on interprocess_condition"
      BOOST_INTERLOCKED_INCREMENT((long*)&m_num_waiters);

      //We unlock the external interprocess_mutex atomically with the increment
      mut.unlock();
   }

   //By default, we suppose that no timeout has happened
   bool timed_out  = false, unlock_enter_mut= false;
   
   //Loop until a notification indicates that the thread should 
   //exit or timeout occurs
   while(1){
      //The thread sleeps/spins until a interprocess_condition commands a notification
      //Notification occurred, we will lock the checking interprocess_mutex so that
      while(winapi::exchange_and_add(&m_command, 0) == SLEEP){
         winapi::sched_yield();

         //Check for timeout
         if(tout_enabled){
            now = boost::posix_time::microsec_clock::universal_time();

            if(now >= abs_time){
               //If we can lock the interprocess_mutex it means that no notification
               //is being executed in this interprocess_condition variable
               timed_out = m_enter_mut.try_lock();

               //If locking fails, indicates that another thread is executing
               //notification, so we play the notification game
               if(!timed_out){
                  //There is an ongoing notification, we will try again later
                  continue;
               }
               //No notification in execution, since enter interprocess_mutex is locked. 
               //We will execute time-out logic, so we will decrement count, 
               //release the enter interprocess_mutex and return false.
               break;
            }
         }
      }

      //If a timeout occurred, the interprocess_mutex will not execute checking logic
      if(tout_enabled && timed_out){
         //Decrement wait count
         BOOST_INTERLOCKED_DECREMENT((long*)&m_num_waiters);
         unlock_enter_mut = true;
         break;
      }
      else{
         //Notification occurred, we will lock the checking interprocess_mutex so that
         //if a notify_one notification occurs, only one thread can exit
        //---------------------------------------------------------------
         boost::interprocess::scoped_lock<interprocess_mutex> lock(m_check_mut);
         //---------------------------------------------------------------
         long result = BOOST_INTERLOCKED_COMPARE_EXCHANGE((long*)&m_command, SLEEP, NOTIFY_ONE);
         if(result == SLEEP){
            //Other thread has been notified and since it was a NOTIFY one
            //command, this thread must sleep again
            continue;
         }
         else if(result == NOTIFY_ONE){
            //If it was a NOTIFY_ONE command, only this thread should  
            //exit. This thread has atomically marked command as sleep before
            //so no other thread will exit.
            //Decrement wait count.
            unlock_enter_mut = true;
            BOOST_INTERLOCKED_DECREMENT((long*)&m_num_waiters);
            break;
         }
         else{
            //If it is a NOTIFY_ALL command, all threads should return 
            //from do_timed_wait function. Decrement wait count. 
            unlock_enter_mut = BOOST_INTERLOCKED_DECREMENT((long*)&m_num_waiters) == 0;
            //Check if this is the last thread of notify_all waiters
            //Only the last thread will release the interprocess_mutex
            if(unlock_enter_mut){
               BOOST_INTERLOCKED_COMPARE_EXCHANGE((long*)&m_command, SLEEP, NOTIFY_ALL);
            }
            break;
         }
      }
   }

   //Unlock the enter interprocess_mutex if it is a single notification, if this is 
   //the last notified thread in a notify_all or a timeout has occurred
   if(unlock_enter_mut){
      m_enter_mut.unlock();
   }

   //Lock external again before returning from the method
   mut.lock();
   return !timed_out;
}

}  //namespace interprocess

}  // namespace boost