/*
This software is subject to the license described in the License.txt file
included with this software distribution. You may not use this file except
in compliance with this license.

Copyright (c) Dynastream Innovations Inc. 2016
All rights reserved.
*/
#if !defined(DSI_THREAD_H)
#define DSI_THREAD_H

#include "types.h"

#if defined(DSI_TYPES_WINDOWS)
   #include <windows.h>
#elif defined(DSI_TYPES_MACINTOSH) || defined(DSI_TYPES_LINUX)
   #define _GNU_SOURCE
   #include <pthread.h>
#endif


//////////////////////////////////////////////////////////////////////////////////
// Public Definitions
//////////////////////////////////////////////////////////////////////////////////

#define DSI_THREAD_INFINITE            MAX_ULONG

// Error codes.
#define DSI_THREAD_ENONE               ((UCHAR) 0x00)
#define DSI_THREAD_ETIMEDOUT           ((UCHAR) 0x01)
#define DSI_THREAD_EBUSY               ((UCHAR) 0x02)
#define DSI_THREAD_ENOTFOUND           ((UCHAR) 0x03)
#define DSI_THREAD_EINVALID            ((UCHAR) 0x04)
#define DSI_THREAD_EOTHER              ((UCHAR) 0xFF)

typedef void *                         DSI_THREAD_RETURN;

#if defined(DSI_TYPES_WINDOWS)
   //#if (INFINITE != MAX_ULONG)
      //#error "!!! INFINITE is not defined as expected in the windows headers !!!"
   //#endif

   typedef struct
   {
      // All members are for internal use only.  Do not modify!
      LONG iWaitersCount;
      CRITICAL_SECTION stStartWaiterCritSec;
      HANDLE hBroadcastDoneEvent;
      HANDLE ahBcastSmph0SignalEvnt1[2];  //In an array for the WaitForMultipleObjects call
   }                                   DSI_CONDITION_VAR;
   typedef HANDLE                      DSI_MUTEX;
   typedef HANDLE                      DSI_THREAD_ID;
   typedef DWORD                 DSI_THREAD_IDNUM;

#elif defined(DSI_TYPES_MACINTOSH) || defined(DSI_TYPES_LINUX)
   typedef pthread_cond_t              DSI_CONDITION_VAR;
   typedef pthread_mutex_t             DSI_MUTEX;
   typedef pthread_t                   DSI_THREAD_ID;
   typedef pthread_t             DSI_THREAD_IDNUM;

#endif


//////////////////////////////////////////////////////////////////////////////////
// Public Function Prototypes
//////////////////////////////////////////////////////////////////////////////////

#if defined(__cplusplus)
   extern "C" {
#endif

UCHAR DSIThread_MutexInit(DSI_MUTEX *pstMutex_);
   ////////////////////////////////////////////////////////////////////
   // Initializes a mutex object.
   // Parameters:
   //    *pstMutex_:          A pointer to a mutex object to be
   //                         initialized.
   // Returns DSI_THREAD_ENONE if successful.  Otherwise, it returns
   // the error code.
   ////////////////////////////////////////////////////////////////////

UCHAR DSIThread_MutexDestroy(DSI_MUTEX *pstMutex_);
   ////////////////////////////////////////////////////////////////////
   // Cleans up a mutex object that is no longer required.
   // Parameters:
   //    *pstMutex_:          A pointer to a mutex object to be cleaned
   //                         up.
   // Returns DSI_THREAD_ENONE if successful.  Otherwise, it returns
   // the error code.
   ////////////////////////////////////////////////////////////////////

UCHAR DSIThread_MutexLock(DSI_MUTEX *pstMutex_);
   ////////////////////////////////////////////////////////////////////
   // Locks a mutex.  If the mutex is already locked, the calling
   // thread blocks until the mutex becomes available.
   // Parameters:
   //    *pstMutex_:          A pointer to a mutex object to be locked.
   // Returns DSI_THREAD_ENONE if successful.  Otherwise, it returns
   // the error code.
   ////////////////////////////////////////////////////////////////////

UCHAR DSIThread_MutexTryLock(DSI_MUTEX *pstMutex_);
   ////////////////////////////////////////////////////////////////////
   // Locks a mutex if it is not locked.  The calling thread returns
   // immediately.
   // Parameters:
   //    *pstMutex_:          A pointer to a mutex object to be locked.
   // Returns DSI_THREAD_ENONE if successful.  If the mutex object is
   // already locked, DSI_THREAD_EBUSY is returned.  If there is an
   // error, DSI_THREAD_EOTHER is returned.
   ////////////////////////////////////////////////////////////////////

UCHAR DSIThread_MutexUnlock(DSI_MUTEX *pstMutex_);
   ////////////////////////////////////////////////////////////////////
   // Unlocks a mutex that is already locked by the calling thread.
   // Parameters:
   //    *pstMutex_:          A pointer to a mutex object to be
   //                         unlocked.
   // Returns DSI_THREAD_ENONE if successful.  Otherwise, it returns
   // the error code.
   ////////////////////////////////////////////////////////////////////

UCHAR DSIThread_CondInit(DSI_CONDITION_VAR *pstConditionVariable_);
   ////////////////////////////////////////////////////////////////////
   // Initializes a condition variable.
   // Parameters:
   //    *pstConditionVariable_: A pointer to a condition variable
   //                         object to be initialized.
   // Returns DSI_THREAD_ENONE if successful.  Otherwise, it returns
   // the error code.
   ////////////////////////////////////////////////////////////////////

UCHAR DSIThread_CondDestroy(DSI_CONDITION_VAR *pstConditionVariable_);
   ////////////////////////////////////////////////////////////////////
   // Cleans up a condition variable that is no longer required.
   // Parameters:
   //    *pstConditionVariable_: A pointer to a condition variable
   //                         object to be cleaned up.
   // Returns DSI_THREAD_ENONE if successful.  Otherwise, it returns
   // the error code.
   ////////////////////////////////////////////////////////////////////

UCHAR DSIThread_CondTimedWait(DSI_CONDITION_VAR *pstConditionVariable_, DSI_MUTEX *pstExternalMutex_, ULONG ulMilliseconds_);
   ////////////////////////////////////////////////////////////////////
   // Waits for a condition variable to be signaled from another
   // thread.
   // Parameters:
   //    *pstConditionVariable_:  A pointer to a condition variable
   //                         object to be waited on.
   //    *pstExternalMutex_:  A pointer to a mutex object that must be
   //                         locked by the calling thread at some
   //                         point prior to calling this function.
   //    ulMilliseconds_:     The minimum time to allow before
   //                         cancelling the wait.  If this parameter
   //                         is set to DSI_THREAD_INFINTE, this
   //                         function blocks indefinitely or until the
   //                         condition variable is signaled by
   //                         another thread.
   // Returns DSI_THREAD_ENONE if successful.  Returns
   // DSI_THREAD_ETIMEDOUT if the time in ulMilliseconds_ expired
   // before receiving a signal.  If there is an error,
   // DSI_THREAD_EOTHER is returned.
   ////////////////////////////////////////////////////////////////////

UCHAR DSIThread_CondSignal(DSI_CONDITION_VAR *pstConditionVariable_);
   ////////////////////////////////////////////////////////////////////
   // Unblocks at least one of the threads that are waiting on the
   // specified condition variable, if any threads are waiting.  If no
   // threads are waiting, this function has no effect.
   // Parameters:
   //    *pstConditionVariable_: A pointer to a condition variable
   //                         object to be signaled.
   // Returns DSI_THREAD_ENONE if successful.  Otherwise, it returns
   // the error code.
   ////////////////////////////////////////////////////////////////////

UCHAR DSIThread_CondBroadcast(DSI_CONDITION_VAR *pstConditionVariable_);
   ////////////////////////////////////////////////////////////////////
   // Unblocks all of the threads that are waiting on the specified
   // condition variable, if any threads are waiting.  If no threads
   // are waiting, this function has no effect.
   // Parameters:
   //    *pstConditionVariable_: A pointer to a condition variable
   //                         object to be signaled.
   // Returns DSI_THREAD_ENONE if successful.  Otherwise, it returns
   // the error code.
   ////////////////////////////////////////////////////////////////////

DSI_THREAD_ID DSIThread_CreateThread(DSI_THREAD_RETURN (*fnThreadStart_)(void *), void *pvParameter_);
   ////////////////////////////////////////////////////////////////////
   // Creates a thread to begin execution at any time after this
   // function is called.
   // Parameters:
   //    *fnThreadStart_:     A pointer to a function that will become
   //                         the start routine of the new thread.  The
   //                         function should be declared as follows:
   //             DSI_THREAD_RETURN FunctionName(void *pvParameter_);
   //    *pvParameter_:       A parameter that will be passed into the
   //                         the start routine.
   // Returns a non-zero thread ID if successfull.  Otherwise, it
   // returns NULL.
   ////////////////////////////////////////////////////////////////////

UCHAR DSIThread_DestroyThread(DSI_THREAD_ID hThreadID);
   ////////////////////////////////////////////////////////////////////
   // Kills the thread specified.
   // Parameters:
   //    hThreadID:      ID of the thread to destroy.
   // Returns DSI_THREAD_ENONE if successful.  Otherwise, it returns
   // the error code.
   // NOTE: This function will destroy the thread without letting it
   //          clean up!
   ////////////////////////////////////////////////////////////////////

UCHAR DSIThread_ReleaseThreadID(DSI_THREAD_ID hThreadID);
   ////////////////////////////////////////////////////////////////////
   // Gives up the thread handle to the operating system.
   // Does not affect the execution of the thread.
   // Parameters:
   //    hThreadID:      ID of the thread to destroy.
   // Returns DSI_THREAD_ENONE if successful.  Otherwise, it returns
   // the error code.
   ////////////////////////////////////////////////////////////////////

DSI_THREAD_IDNUM DSIThread_GetCurrentThreadIDNum(void);
   ////////////////////////////////////////////////////////////////////
   // Gets the current thread ID number of the caller.
   //
   // Returns the current thread ID of the caller.
   // Does not return any error codes.
   ////////////////////////////////////////////////////////////////////

BOOL DSIThread_CompareThreads(DSI_THREAD_IDNUM hThreadIDNum1, DSI_THREAD_IDNUM hThreadIDNum2);
   ////////////////////////////////////////////////////////////////////
   // Compares two thread ID numbers.
   // Parameters:
   //    hThreadIDNum1/hThreadIDNum2:  the two thread ID numbers to be
   //                                  compared.
   // Returns TRUE if the two threads are the same.
   // Does not return any error codes.
   // NOTE: Behaviour is undefined if either thread id numbers are
   //     invalid.
   ////////////////////////////////////////////////////////////////////

ULONG DSIThread_GetSystemTime(void);
   ////////////////////////////////////////////////////////////////////
   // Gets the current system time in milliseconds.
   //
   // Returns the current system time in milliseconds.
   ////////////////////////////////////////////////////////////////////

BOOL DSIThread_GetWorkingDirectory(UCHAR* pucDirectory, USHORT usLength);
   ////////////////////////////////////////////////////////////////////
   // Gets the current working directory.
   // Parameters:
      //    pucDirectory:  the current working directory of the system
      //    usLength:      the length of the directory's string
      //
      // When successful, pucDirectory will hold the current working
      // directory, else pucDirectory will be undefined.
   //
   // Returns TRUE if successful.
   ////////////////////////////////////////////////////////////////////

void DSIThread_Sleep(ULONG ulMilliseconds);
   ////////////////////////////////////////////////////////////////////
   // Puts the current thread to sleep for the number of milliseconds
      //    specified.
   // Parameters:
   //    ulMilliseconds:      Number of milliseconds to sleep.
   ////////////////////////////////////////////////////////////////////

#if defined(__cplusplus)
   }
#endif

#endif // !defined(DSI_THREAD_H)

