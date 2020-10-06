/*
This software is subject to the license described in the License.txt file
included with this software distribution. You may not use this file except
in compliance with this license.

Copyright (c) Dynastream Innovations Inc. 2016
All rights reserved.
*/
#ifndef DSI_TS_QUEUE_HPP
#define DSI_TS_QUEUE_HPP

#include <list>
#include <queue>
#include <deque>


//NOTE: Make sure nobody is still using this queue when it is being destroyed!
template < class T, class Container = std::deque<T> >
class TSQueue  //thread-safe queue
{
  public:

   TSQueue()
   {
      UCHAR ret;
      ret = DSIThread_CondInit(&stEventPush);
      if(ret != DSI_THREAD_ENONE)
         throw; //!!Need to throw something!

      ret = DSIThread_MutexInit(&stMutex);
      if(ret != DSI_THREAD_ENONE)
      {
         DSIThread_CondDestroy(&stEventPush);
         throw; //!!Need to throw something!
      }

      return;
   }

   ~TSQueue()
   {
      DSIThread_MutexDestroy(&stMutex);
      DSIThread_CondDestroy(&stEventPush);
      return;
   }


   void Push(const T& tElement_)
   {
      DSIThread_MutexLock(&stMutex);
      {
         clQueue.push(tElement_);
         DSIThread_CondSignal(&stEventPush);
      }
      DSIThread_MutexUnlock(&stMutex);

      return;
   }

   void PushArray(T* const ptElementArray_, ULONG ulSize_)
   {
      //!!Need to handle when ptElementArray_ == NULL

      DSIThread_MutexLock(&stMutex);
      {
         for(ULONG i=0; i<ulSize_; i++)      //!!would it be faster if we assigned or used an iterator instead of pushing?
            clQueue.push(ptElementArray_[i]);
         DSIThread_CondSignal(&stEventPush);
      }
      DSIThread_MutexUnlock(&stMutex);

      return;
   }


   BOOL Pop(T& tElement_, ULONG ulWaitTime_ = 0)
   {
      DSIThread_MutexLock(&stMutex);
      {
         if(clQueue.empty() == TRUE)
         {
            UCHAR ret;
            ret = DSIThread_CondTimedWait(&stEventPush, &stMutex, ulWaitTime_);
            if(ret != DSI_THREAD_ENONE)
            {
               DSIThread_MutexUnlock(&stMutex);
               return FALSE;
            }
         }

         tElement_ = clQueue.front();
         clQueue.pop();
      }
      DSIThread_MutexUnlock(&stMutex);

      return TRUE;
   }

   ULONG PopArray(T* const ptElementArray_, ULONG ulMaxSize_, ULONG ulWaitTime_ = 0)
   {
      //!!Need to handle when ptElementArray_ == NULL

      ULONG ulFinalSize;

      DSIThread_MutexLock(&stMutex);
      {
         if(clQueue.empty() == TRUE)
         {
            UCHAR ret;
            ret = DSIThread_CondTimedWait(&stEventPush, &stMutex, ulWaitTime_);
            if(ret != DSI_THREAD_ENONE)
            {
               DSIThread_MutexUnlock(&stMutex);
               return 0;
            }
         }

         size_t QueueSize = clQueue.size();

         if (QueueSize > MAX_ULONG)
            QueueSize = MAX_ULONG;

         ULONG ulQueueSize = (ULONG)QueueSize;

         ULONG ulSize = (ulMaxSize_ < ulQueueSize) ? ulMaxSize_ : ulQueueSize;  //MIN(ulMaxSize_, ulQueueSize);
         for(ULONG i=0; i<ulSize; i++)
         {
            ptElementArray_[i] = clQueue.front();
            clQueue.pop();
         }
         ulFinalSize = ulSize;
      }
      DSIThread_MutexUnlock(&stMutex);


      return ulFinalSize;
   }

  private:
   DSI_CONDITION_VAR stEventPush;
   DSI_MUTEX stMutex;

   std::queue<T, Container> clQueue;

};



#endif //DSI_TS_QUEUE_HPP