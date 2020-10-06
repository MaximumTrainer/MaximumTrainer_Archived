/*
This software is subject to the license described in the License.txt file
included with this software distribution. You may not use this file except
in compliance with this license.

Copyright (c) Dynastream Innovations Inc. 2016
All rights reserved.
*/
#if !defined(DSI_TIMER_HPP)
#define DSI_TIMER_HPP

#include "types.h"
#include "dsi_thread.h"

//////////////////////////////////////////////////////////////////////////////////
// Public Definitions
//////////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////////
// Public Class Prototypes
//////////////////////////////////////////////////////////////////////////////////

class DSITimer
{
   private:
      DSI_THREAD_RETURN (*fnTimerFunc)(void *);
      void *pvTimerFuncParameter;
      BOOL bClosing;
      DSI_MUTEX stMutexCriticalSection;
      DSI_CONDITION_VAR stCondTimerWait;
      DSI_CONDITION_VAR stCondTimerThreadExit;
      DSI_THREAD_ID hTimerThread;
      BOOL bRecurring;
      ULONG ulInterval;

      static DSI_THREAD_RETURN TimerThreadStart(void *pvParameter_);
      void TimerThread(void);

   public:

      // Constuctor and Destructor
      DSITimer(DSI_THREAD_RETURN (*fnTimerFunc_)(void *) = (DSI_THREAD_RETURN(*)(void *))NULL, void *pvTimerFuncParameter_ = NULL, ULONG ulInterval_ = 0, BOOL bRecurring_ = FALSE);
      ~DSITimer();

      BOOL NoError(void);
};

#endif //DSI_TIMER_HPP

