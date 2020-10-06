/*
This software is subject to the license described in the License.txt file
included with this software distribution. You may not use this file except
in compliance with this license.

Copyright (c) Dynastream Innovations Inc. 2016
All rights reserved.
*/
#include "types.h"
#include "dsi_timer.hpp"
#include <stdio.h>


//////////////////////////////////////////////////////////////////////////////////
// Public Class Functions
//////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////
/*
DSITimer::DSITimer()
{

}
*/

DSITimer::DSITimer(DSI_THREAD_RETURN (*fnTimerFunc_)(void *), void *pvTimerFuncParameter_, ULONG ulInterval_, BOOL bRecurring_)
{
   fnTimerFunc = fnTimerFunc_;
   pvTimerFuncParameter = pvTimerFuncParameter_;
   ulInterval = ulInterval_;
   bRecurring = bRecurring_;
   bClosing = FALSE;
   hTimerThread = (DSI_THREAD_ID)NULL;

   if (DSIThread_MutexInit(&stMutexCriticalSection) != DSI_THREAD_ENONE)
   {
      fnTimerFunc = (void*(*)(void*))NULL;
      return;
   }

   if (DSIThread_CondInit(&stCondTimerThreadExit) != DSI_THREAD_ENONE)
   {
      DSIThread_MutexDestroy(&stMutexCriticalSection);
      fnTimerFunc = (void*(*)(void*))NULL;
      return;
   }

   if (DSIThread_CondInit(&stCondTimerWait) != DSI_THREAD_ENONE)
   {
      DSIThread_MutexDestroy(&stMutexCriticalSection);
      DSIThread_CondDestroy(&stCondTimerThreadExit);
      fnTimerFunc = (void*(*)(void*))NULL;
      return;
   }


   hTimerThread = DSIThread_CreateThread(&DSITimer::TimerThreadStart, this);
   if (hTimerThread == NULL)
   {
      DSIThread_MutexDestroy(&stMutexCriticalSection);
      DSIThread_CondDestroy(&stCondTimerThreadExit);
      DSIThread_CondDestroy(&stCondTimerWait);
      fnTimerFunc = (void*(*)(void*))NULL;
   }
}
///////////////////////////////////////////////////////////////////////
DSITimer::~DSITimer()
{
   if (hTimerThread)
   {
      DSIThread_MutexLock(&stMutexCriticalSection);
      bClosing = TRUE;                                                    //Set the exit flag
      DSIThread_CondSignal(&stCondTimerWait);                             //Wake up the timer thread early if it is "sleeping"

      if (fnTimerFunc != NULL)                                            //This means the timer thread is still active
      {
         if (DSIThread_CondTimedWait(&stCondTimerThreadExit, &stMutexCriticalSection, 3000) != DSI_THREAD_ENONE)
         {
            // We were unable to stop the thread normally, so kill it.
            DSIThread_DestroyThread(hTimerThread);
         }
      }
      DSIThread_MutexUnlock(&stMutexCriticalSection);

      DSIThread_ReleaseThreadID(hTimerThread);
      hTimerThread = (DSI_THREAD_ID)NULL;

      DSIThread_MutexDestroy(&stMutexCriticalSection);
      DSIThread_CondDestroy(&stCondTimerThreadExit);
      DSIThread_CondDestroy(&stCondTimerWait);
   }
}

///////////////////////////////////////////////////////////////////////
BOOL DSITimer::NoError()
{
   if ((fnTimerFunc != NULL) && (bClosing == FALSE))   //There was an error if the function pointer is set to NULL and we havent tried to close yet.
      return TRUE;

   return FALSE;
}

///////////////////////////////////////////////////////////////////////
DSI_THREAD_RETURN DSITimer::TimerThreadStart(void *pvParameter_)
{
   DSITimer *This = (DSITimer *) pvParameter_;

   This->TimerThread();

   return 0;
}

///////////////////////////////////////////////////////////////////////
void DSITimer::TimerThread(void)
{
   ULONG ulWaitTime;
   ULONG ulTargetTime = DSIThread_GetSystemTime();

   while(1) //We break on bClosing, but need to check it within the critical section
   {
      DSIThread_MutexLock(&stMutexCriticalSection);
      if (bClosing)
      {
         DSIThread_MutexUnlock(&stMutexCriticalSection);
         break;
      }

      ulTargetTime += ulInterval;                                                //set the new target time
      ulWaitTime = ulTargetTime - DSIThread_GetSystemTime();                     //figure out how long it is from now

      if (ulWaitTime && ulWaitTime < MAX_ULONG/2)                                //check if it we need to wait
      {
         if (DSIThread_CondTimedWait(&stCondTimerWait, &stMutexCriticalSection, ulWaitTime) != DSI_THREAD_ETIMEDOUT)   //wait till our next interval
         {
            //If we get anything other than a timeout we will exit right away
            bClosing = TRUE;
            DSIThread_MutexUnlock(&stMutexCriticalSection);
            break;
         }
      }
      DSIThread_MutexUnlock(&stMutexCriticalSection);

      fnTimerFunc(pvTimerFuncParameter);                                         //Call the timer interval function

      if (bRecurring == FALSE)
         bClosing = TRUE;
   }

   DSIThread_MutexLock(&stMutexCriticalSection);
      fnTimerFunc = (void*(*)(void*))NULL;
      DSIThread_CondSignal(&stCondTimerThreadExit);                       // Set an event to alert the main process that the timer thread is finished.
   DSIThread_MutexUnlock(&stMutexCriticalSection);
}