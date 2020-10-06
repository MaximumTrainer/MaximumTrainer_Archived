/*
This software is subject to the license described in the License.txt file
included with this software distribution. You may not use this file except
in compliance with this license.

Copyright (c) Dynastream Innovations Inc. 2016
All rights reserved.
*/
#include "types.h"
#if defined(DSI_TYPES_WINDOWS)

#include "dsi_thread.h"
#include "macros.h"

#include <windows.h>
#include <string.h>


//////////////////////////////////////////////////////////////////////////////////
// Public Functions
//////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////
// Code below is no longer based on "Strategies for Implementing POSIX Condition
// Variables on Win32" by Douglas C. Schmidt and Irfan Pyarali of
// Department of Computer Science, Washington University, St. Louis,
// Missouri
// http://www.cs.wustl.edu/~schmidt/win32-cv-1.html
// This implementation had some flaws that were causing fals
///////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////
UCHAR DSIThread_MutexInit(DSI_MUTEX *pstMutex_)
{
   *pstMutex_ = CreateMutex(
      (LPSECURITY_ATTRIBUTES) NULL,                         // This pstMutex_ gets the default security descriptor.
      FALSE,                                             // Not owned by the calling thread; should be "unlocked".
      (LPCTSTR) NULL);                                      // Object name not required.

   if (*pstMutex_ == NULL)
      return DSI_THREAD_EOTHER;                             // We've failed for some reason.

   return DSI_THREAD_ENONE;
}

///////////////////////////////////////////////////////////////////////
UCHAR DSIThread_MutexDestroy(DSI_MUTEX *pstMutex_)
{
   if (CloseHandle(*pstMutex_))
      return DSI_THREAD_ENONE;

   return DSI_THREAD_EOTHER;
}

///////////////////////////////////////////////////////////////////////
UCHAR DSIThread_MutexLock(DSI_MUTEX *pstMutex_)
{
   WaitForSingleObject(*pstMutex_, INFINITE);
   return DSI_THREAD_ENONE;
}

///////////////////////////////////////////////////////////////////////
UCHAR DSIThread_MutexTryLock(DSI_MUTEX *pstMutex_)
{
   DWORD dwWaitObject = WaitForSingleObject(*pstMutex_, 0);

   if (dwWaitObject == WAIT_OBJECT_0)
      return DSI_THREAD_ENONE;

   return DSI_THREAD_EBUSY;
}

///////////////////////////////////////////////////////////////////////
UCHAR DSIThread_MutexUnlock(DSI_MUTEX *pstMutex_)
{
   ReleaseMutex(*pstMutex_);
   return DSI_THREAD_ENONE;
}

///////////////////////////////////////////////////////////////////////
UCHAR DSIThread_CondInit(DSI_CONDITION_VAR *pstConditionVariable_)
{
   pstConditionVariable_->iWaitersCount = 0;
   InitializeCriticalSection(&pstConditionVariable_->stStartWaiterCritSec);

   //Create the broadcast semaphore
   pstConditionVariable_->ahBcastSmph0SignalEvnt1[0] = CreateSemaphore(
      (LPSECURITY_ATTRIBUTES) NULL,                         // This semaphore gets the default security descriptor.
      0,                                                    // Initially 0.
      0x7FFFFFFF,                                           // Max count.
      (LPCTSTR) NULL);                                      // Object name not required.

   //Create the 'signal one' event
   pstConditionVariable_->ahBcastSmph0SignalEvnt1[1] = CreateEvent (
      (LPSECURITY_ATTRIBUTES) NULL,                         // This event gets the default security descriptor.
      FALSE,                                                // FALSE means we use auto-reset.
      FALSE,                                                // Non-signaled initially.
      (LPCTSTR) NULL);                                      // Object name not required.

   pstConditionVariable_->hBroadcastDoneEvent = CreateEvent (
      (LPSECURITY_ATTRIBUTES) NULL,                         // This event gets the default security descriptor.
      FALSE,                                                // FALSE means we use auto-reset.
      FALSE,                                                // Non-signaled initially.
      (LPCTSTR) NULL);                                      // Object name not required.


   if (pstConditionVariable_->ahBcastSmph0SignalEvnt1[0] == NULL
      || pstConditionVariable_->ahBcastSmph0SignalEvnt1[1] == NULL
      || pstConditionVariable_->hBroadcastDoneEvent == NULL)
      return DSI_THREAD_EOTHER;                             // We've failed initializing objects for some reason, call GetLastError() to figure out why

   return DSI_THREAD_ENONE;
}

///////////////////////////////////////////////////////////////////////
UCHAR DSIThread_CondDestroy(DSI_CONDITION_VAR *pstConditionVariable_)
{
   UCHAR ucRetVal = DSI_THREAD_ENONE;

   EnterCriticalSection(&pstConditionVariable_->stStartWaiterCritSec);  //Make sure we have ownership before we kill it
   DeleteCriticalSection(&pstConditionVariable_->stStartWaiterCritSec); //Now nobody can get it, if someone is still waiting they will be stuck, but it is better than calling LeaveCriticalSection on a deleted cs (which causes possible memory corruption).

   //Release all threads that are stuck waiting so we try not to leave any dead waits
   ReleaseSemaphore(pstConditionVariable_->ahBcastSmph0SignalEvnt1[0], 0x7fffffff, 0);

   //Destroy the rest of the objects
   if (!CloseHandle(pstConditionVariable_->hBroadcastDoneEvent))
      ucRetVal = DSI_THREAD_EOTHER;

   if (!CloseHandle(pstConditionVariable_->ahBcastSmph0SignalEvnt1[1]))
      ucRetVal = DSI_THREAD_EOTHER;

   if (!CloseHandle(pstConditionVariable_->ahBcastSmph0SignalEvnt1[0]))
      ucRetVal = DSI_THREAD_EOTHER;


   return ucRetVal;
}

///////////////////////////////////////////////////////////////////////
UCHAR DSIThread_CondTimedWait(DSI_CONDITION_VAR *pstConditionVariable_, DSI_MUTEX *pstExternalMutex_, ULONG ulMilliseconds_)
{
   DWORD dwResult;

   //Make sure DSI_THREAD_INFINITE translates to INFINITE
   if(ulMilliseconds_ == DSI_THREAD_INFINITE)
      ulMilliseconds_ = INFINITE;

   // Add a count for us when we are allowed to start waiting
   EnterCriticalSection(&pstConditionVariable_->stStartWaiterCritSec);
   InterlockedIncrement(&(pstConditionVariable_->iWaitersCount));
   LeaveCriticalSection(&pstConditionVariable_->stStartWaiterCritSec);

   //Release the variable mutex to allow whatever will signal us (which needs to acquire the mutex) to occur
   if(ReleaseMutex(*pstExternalMutex_) == 0)
      return DSI_THREAD_EOTHER;

   //Wait for broadcast or normal signal
   dwResult = WaitForMultipleObjects(2, pstConditionVariable_->ahBcastSmph0SignalEvnt1, FALSE, ulMilliseconds_);

   if(dwResult - WAIT_OBJECT_0 == 1) //array index is the normal signal event
   {
      //Note: The waiter count is already decremented by the signal function
      dwResult = WaitForSingleObject(*pstExternalMutex_, INFINITE);
   }
   else if(dwResult >= WAIT_ABANDONED) //If we didn't get a signal
   {
      //Even though we have an error, the contract states we need the mutex back before we return
      //Also, since we didn't get a signal, we need to manage the waiterCount here
      //Lastly, If we were in a broadcast and are the last waiter, we need to ensure the signal is fired, and
      //it doesn't hurt to do it out of a broadcast
      //Note: We don't handle this the same as the broadcast case, since we want to return the original failure code, not save a new one
      if(InterlockedDecrement(&(pstConditionVariable_->iWaitersCount)) == 0)
         SignalObjectAndWait(pstConditionVariable_->hBroadcastDoneEvent, *pstExternalMutex_, INFINITE, FALSE);
      else
         WaitForSingleObject(*pstExternalMutex_, INFINITE);
   }
   else //dwResult - WAIT_OBJECT_0 == 0 //array index is the broadcast event
   {
      //Decrement waiter count if we are in broadcast,
      //If we are the last waiter we need to signal the broadcast is done
      //Interlocked, so value is correct between all broadcast released threads
      if(InterlockedDecrement(&(pstConditionVariable_->iWaitersCount)) == 0)
         dwResult = SignalObjectAndWait(pstConditionVariable_->hBroadcastDoneEvent, *pstExternalMutex_, INFINITE, FALSE);
      else
         dwResult = WaitForSingleObject(*pstExternalMutex_, INFINITE);
   }

   if (dwResult == WAIT_OBJECT_0)
      return DSI_THREAD_ENONE;

   if (dwResult == WAIT_TIMEOUT)
      return DSI_THREAD_ETIMEDOUT;

   return DSI_THREAD_EOTHER;
}

///////////////////////////////////////////////////////////////////////
UCHAR DSIThread_CondSignal(DSI_CONDITION_VAR *pstConditionVariable_)
{
   //We get the 'start' mutex to ensure we aren't signalling during a broadcast
   EnterCriticalSection(&pstConditionVariable_->stStartWaiterCritSec);

   //CondSignal allows one current waiter through, so only open the event if we have current waiters
   //We decrement waiters count here in signal to avoid race condition where signal could occur multiple
   //times between last waiter thread being signaled and them decrementing their own waiter count (broadcast case is different)
   if(pstConditionVariable_->iWaitersCount > 0)
   {
      InterlockedDecrement(&(pstConditionVariable_->iWaitersCount));
      SetEvent(pstConditionVariable_->ahBcastSmph0SignalEvnt1[1]);
   }

   LeaveCriticalSection(&pstConditionVariable_->stStartWaiterCritSec);

   return DSI_THREAD_ENONE;
}

///////////////////////////////////////////////////////////////////////
UCHAR DSIThread_CondBroadcast(DSI_CONDITION_VAR *pstConditionVariable_)
{
   // We need the waiter lock to ensure the broadcast only signals the current waiters
   // This also blocks CondSignal() from firing
   // This lock also ensures the normal signal doesn't happen during the broadcast and mess up the count
   EnterCriticalSection(&pstConditionVariable_->stStartWaiterCritSec);

   if (pstConditionVariable_->iWaitersCount > 0)
   {
      // Release all the current waiters using the broadcast semaphore
      ReleaseSemaphore(pstConditionVariable_->ahBcastSmph0SignalEvnt1[0], pstConditionVariable_->iWaitersCount, 0);

      // Wait until the last one is done
      WaitForSingleObject(pstConditionVariable_->hBroadcastDoneEvent, INFINITE);
   }

   //Release the lock to allow new waiters again
   LeaveCriticalSection(&pstConditionVariable_->stStartWaiterCritSec);

   return DSI_THREAD_ENONE;
}

///////////////////////////////////////////////////////////////////////
DSI_THREAD_ID DSIThread_CreateThread(DSI_THREAD_RETURN (*fnThreadStart_)(void *), void *pvParameter_)
{
   return
   CreateThread(
      (LPSECURITY_ATTRIBUTES) NULL,                         // Default security descriptor.
      0,                                                    // Default stack size for exe.
      (LPTHREAD_START_ROUTINE) fnThreadStart_,
      pvParameter_,
      0,                                                    // The thread begins execution immediately.
      NULL);                                                // No need for a thread ID.
}

///////////////////////////////////////////////////////////////////////
UCHAR DSIThread_DestroyThread(DSI_THREAD_ID hThreadID_)
{
   if(TerminateThread(hThreadID_, 0) == 0)
      return DSI_THREAD_EOTHER;

   return DSI_THREAD_ENONE;
}

///////////////////////////////////////////////////////////////////////
UCHAR DSIThread_ReleaseThreadID(DSI_THREAD_ID hThreadID)
{
   if(CloseHandle(hThreadID) == 0)
      return DSI_THREAD_EOTHER;

   return DSI_THREAD_ENONE;
}

///////////////////////////////////////////////////////////////////////
DSI_THREAD_IDNUM DSIThread_GetCurrentThreadIDNum(void)
{
   return GetCurrentThreadId();
}

///////////////////////////////////////////////////////////////////////
BOOL DSIThread_CompareThreads(DSI_THREAD_IDNUM hThreadIDNum1, DSI_THREAD_IDNUM hThreadIDNum2)
{
   return (hThreadIDNum1 == hThreadIDNum2);
}

///////////////////////////////////////////////////////////////////////
ULONG DSIThread_GetSystemTime(void)
{
   return GetTickCount();
}

///////////////////////////////////////////////////////////////////////
BOOL DSIThread_GetWorkingDirectory(UCHAR* pucDirectory_, USHORT usLength_)
{
   if(pucDirectory_ == NULL)
      return FALSE;

   if(GetCurrentDirectory((DWORD)(usLength_-1), (LPSTR)pucDirectory_) == 0)
      return FALSE;

   SNPRINTF((char*)(&pucDirectory_[strlen((char*)pucDirectory_)]), 2, "\\");
   return TRUE;

}

///////////////////////////////////////////////////////////////////////
void DSIThread_Sleep(ULONG ulMilliseconds_)
{
   Sleep(ulMilliseconds_);
   return;
}



#endif //defined(DSI_TYPES_WINDOWS)