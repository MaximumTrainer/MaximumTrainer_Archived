/*
This software is subject to the license described in the License.txt file
included with this software distribution. You may not use this file except
in compliance with this license.

Copyright (c) Dynastream Innovations Inc. 2016
All rights reserved.
*/
#include "dsi_debug.hpp"

#if defined(DEBUG_FILE)

#include "types.h"
#include "dsi_thread.h"
#include "macros.h"
#include "defines.h"

#include <stdlib.h>
#include <string.h>
#include <stdarg.h>


#define NEW_SESSION_MESG         "New Session.\n"

#define OVERFLOW_ERROR           "\n*** ERROR: BUFFER OVERFLOW! ***\n"
#define OVERFLOW_ERROR_LENGTH    33

#define TRUNCATE_ERROR           " *** TRUNCATED! ***"
#define WRITE_ERROR              "\n*** WRITE ERROR ***\n"

#define MAX_NAME_LENGTH          255
#define MAX_PORTS                ((UCHAR)255)
#define MAX_THREADS              ((UCHAR)10)

#define THREAD_EXIT_TIMEOUT      ((ULONG)30)
#define MUTEX_UNLOCK_DELAY       ((UCHAR)10) //milliseconds to wait for other threads to unlock mutexs

//Buffer class defines//

#define BUFFER_SIZE_NO_CAST      (0xFFFF)   //Must be >= DSI_DEBUG_MAX_STRLEN  && < MAX_ULONG - OVERFLOW_ERROR_LENGTH - 1
#define BUFFER_SIZE              ((ULONG)BUFFER_SIZE_NO_CAST)
#if BUFFER_SIZE_NO_CAST == 0
   #error
#endif

#define ACTUAL_BUFFER_SIZE       ((ULONG)(BUFFER_SIZE + OVERFLOW_ERROR_LENGTH + 1))
      //Buffer size including room for the error message and plus one so we know the buffer is empty if the pointers equal each other
#define FLUSH_PERCENT            ((UCHAR)50)
#define FLUSH_CHECK_PERIOD       ((ULONG)5000)


BOOL DSIDebug::bInitialized = FALSE;

//////////////////////////////////////////////////////////
// Buffer Class Declaration
//////////////////////////////////////////////////////////

class Buffer
{
 public:
   Buffer(UCHAR* pucFilename_, UCHAR* pucDirectory_);
   ~Buffer();
   BOOL Add(UCHAR* pucString_, ULONG ulSize_);

   void SetEnable(BOOL bEnable_);
   BOOL SetDirectory(const UCHAR* pucDirectory_);

 private:

   BOOL bEnable;

   BOOL Flush();
   ULONG GetBufferCount();
   UCHAR GetPercentFull();
   void WriteThread();
   static DSI_THREAD_RETURN StartWriteThread(void* pvParam_);

   //Helper functions
   void SignalFlush();

   ULONG ulInput;
   ULONG ulOutput;

   DSI_MUTEX stInputMutex;
   DSI_MUTEX stFilenameMutex;
   DSI_MUTEX stFlushMutex;
   DSI_CONDITION_VAR stFlushCond;

   UCHAR aucFilename[MAX_NAME_LENGTH];
   UCHAR aucFullPath[MAX_NAME_LENGTH];
   UCHAR aucData[ACTUAL_BUFFER_SIZE];

   //Thread variables
   BOOL bWriteThreadExit;
   DSI_THREAD_ID hWriteThreadID;
   DSI_MUTEX stWriteThreadMutex;
   DSI_CONDITION_VAR stWriteThreadCond;

   BOOL bBufferOverflow;
   volatile BOOL bForceFlush;

};


//////////////////////////////////////////////////////////
// Private Declarations
//////////////////////////////////////////////////////////

typedef struct _TASK_PROP
{
   _TASK_PROP(DSI_THREAD_IDNUM hThreadIDNum_, UCHAR* pucFilename_, UCHAR* pucDirectory_)
   {
      hThreadIDNum = hThreadIDNum_;
      pclBuffer = new Buffer(pucFilename_, pucDirectory_);
   }

   ~_TASK_PROP()
   {
      if(pclBuffer)
        delete pclBuffer;
   }

   DSI_THREAD_IDNUM hThreadIDNum;
   Buffer* pclBuffer;
} THREAD_PROP;

//Private Function Declarations
BOOL FindThreadNum(UCHAR* pucNum_);


//Private Variables
ULONG ulStartTime;
BOOL bWriteEnable;
Buffer* apclSerialBuffer[MAX_PORTS];
THREAD_PROP* apstThread[MAX_THREADS];

UCHAR aucLogDirectory[MAX_NAME_LENGTH];
UCHAR aucExecutablePath[MAX_NAME_LENGTH];

DSI_MUTEX stThreadBufferMutex;
DSI_MUTEX stSerialBufferMutex;


//////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////
///////////////      DSIDebug Class        ///////////////
//////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////
// Public Definitions
//////////////////////////////////////////////////////////


//Warning: Not thread safe!
BOOL DSIDebug::Init()
{
   if(bInitialized)
      return TRUE;

   ulStartTime = DSIThread_GetSystemTime();
   bWriteEnable = FALSE;

   //Get the Directory of the executable
   DSIThread_GetWorkingDirectory(aucExecutablePath, MAX_NAME_LENGTH);
   STRNCPY((char*)aucLogDirectory, (char*)aucExecutablePath, MAX_NAME_LENGTH);

   for(UCHAR i=0; i<MAX_PORTS; i++)
      apclSerialBuffer[i] = (Buffer*)NULL;

   for(UCHAR i=0; i<MAX_THREADS; i++)
      apstThread[i] = (THREAD_PROP*)NULL;

   DSIThread_MutexInit(&stThreadBufferMutex);
   DSIThread_MutexInit(&stSerialBufferMutex);

   bInitialized = TRUE;
   return TRUE;
}

//Warning: Not thread safe!
void DSIDebug::Close()
{
   if(!bInitialized)
      return;

   bWriteEnable = FALSE;

   //Clean up all the buffers
   DSIThread_MutexLock(&stSerialBufferMutex);
   for(UCHAR i=0; i<MAX_PORTS; i++)
   {
      if(apclSerialBuffer[i] != NULL)
      {
         delete apclSerialBuffer[i];
         apclSerialBuffer[i] = (Buffer*)NULL;
      }
   }
   DSIThread_MutexUnlock(&stSerialBufferMutex);

   DSIThread_MutexLock(&stThreadBufferMutex);
   for(UCHAR i=0; i<MAX_THREADS; i++)
   {
      if(apstThread[i] != NULL)
      {
         delete apstThread[i];
         apstThread[i] = (THREAD_PROP*)NULL;
      }
   }
   DSIThread_MutexUnlock(&stThreadBufferMutex);


   DSIThread_Sleep(MUTEX_UNLOCK_DELAY);  //Wait for any functions that have the mutex to unlock them.

   DSIThread_MutexDestroy(&stThreadBufferMutex);
   DSIThread_MutexDestroy(&stSerialBufferMutex);

   bInitialized = FALSE;
   return;
}

BOOL DSIDebug::ResetTime()
{
   if(!bInitialized)
      return FALSE;

   ulStartTime = DSIThread_GetSystemTime();

   if(!bWriteEnable)
      return FALSE;

   for(UCHAR i=0; i<MAX_THREADS; i++)
   {
      if(apstThread[i] != NULL)
        apstThread[i]->pclBuffer->Add((UCHAR*)NEW_SESSION_MESG, sizeof(NEW_SESSION_MESG)-1);
   }
   for(UCHAR i=0; i<MAX_PORTS; i++)
   {
      if(apclSerialBuffer[i] != NULL)
        apclSerialBuffer[i]->Add((UCHAR*)NEW_SESSION_MESG, sizeof(NEW_SESSION_MESG)-1);
   }

   return TRUE;
}


BOOL DSIDebug::SetDirectory(const char* pcDirectory_)
{
   if(!bInitialized || pcDirectory_ == NULL)
      return FALSE;

   if(strcmp(pcDirectory_, "") == 0)
      STRNCPY((char*)aucLogDirectory, (char*)aucExecutablePath, MAX_NAME_LENGTH);
   else
      SNPRINTF((char*)aucLogDirectory, MAX_NAME_LENGTH, "%s", pcDirectory_);

   for(UCHAR i=0; i<MAX_THREADS; i++)
   {
      if(apstThread[i] != NULL)
         apstThread[i]->pclBuffer->SetDirectory(aucLogDirectory);
   }
   for(UCHAR i=0; i<MAX_PORTS; i++)
   {
      if(apclSerialBuffer[i] != NULL)
         apclSerialBuffer[i]->SetDirectory(aucLogDirectory);
   }

   return TRUE;
}


BOOL DSIDebug::ThreadInit(const char* pucName_)
{
   if(!bInitialized || pucName_ == NULL || strlen(pucName_) > MAX_NAME_LENGTH)
      return FALSE;

   DSIThread_MutexLock(&stThreadBufferMutex);

   UCHAR ucThreadNum;
   if(!FindThreadNum(&ucThreadNum))
   {
     //Buffer is full
      DSIThread_MutexUnlock(&stThreadBufferMutex);
      return FALSE;
   }

   if(apstThread[ucThreadNum] != NULL)
   {
     //we have already set the thread
      DSIThread_MutexUnlock(&stThreadBufferMutex);
      return FALSE;
   }

   UCHAR aucString[MAX_NAME_LENGTH+20];
   if(pucName_ != NULL)
      #if defined (DSI_TYPES_MACINTOSH)
         SNPRINTF((char*)aucString, MAX_NAME_LENGTH+20, "ao_debug_%s.log", pucName_);
      #else
         SNPRINTF((char*)aucString, MAX_NAME_LENGTH+20, "ao_debug_%s.txt", pucName_);
      #endif
   else
      #if defined (DSI_TYPES_MACINTOSH)
         SNPRINTF((char*)aucString, MAX_NAME_LENGTH+20, "ao_debug_Thread%u.log", DSIThread_GetCurrentThreadIDNum());
      #else
         SNPRINTF((char*)aucString, MAX_NAME_LENGTH+20, "ao_debug_Thread%u.txt", DSIThread_GetCurrentThreadIDNum());
      #endif

   apstThread[ucThreadNum] = new THREAD_PROP(DSIThread_GetCurrentThreadIDNum(), aucString, aucLogDirectory);

   DSIThread_MutexUnlock(&stThreadBufferMutex);

   if(apstThread[ucThreadNum] == NULL)
      return FALSE;

   return TRUE;

}


BOOL DSIDebug::ThreadWrite(const char* pcMessage_)
{
   if(!bInitialized || pcMessage_ == NULL)
      return FALSE;

   if(!bWriteEnable)
      return FALSE;

   //Find the index of the proper thread property struct
   UCHAR ucThreadNum;
   if(!FindThreadNum(&ucThreadNum))
      return FALSE;

   if(apstThread[ucThreadNum] == NULL)
      return FALSE;


   char acString[DSI_DEBUG_MAX_STRLEN];
   ULONG ulCurrentTime = DSIThread_GetSystemTime();

   SNPRINTF(acString, DSI_DEBUG_MAX_STRLEN, "%10.3f {%10lu}: %s\n", (ulCurrentTime-ulStartTime)/1000.0, ulCurrentTime, pcMessage_);
   ULONG totalLen = strlen(acString);

   //If we are too long, than overwrite the truncate error to the end
   if(totalLen >= DSI_DEBUG_MAX_STRLEN-1)
      SNPRINTF(acString + DSI_DEBUG_MAX_STRLEN - 2 - strlen(TRUNCATE_ERROR), strlen(TRUNCATE_ERROR)+2, "%s\n", TRUNCATE_ERROR);

   return apstThread[ucThreadNum]->pclBuffer->Add((UCHAR*)acString, totalLen);
}

BOOL DSIDebug::ThreadPrintf(const char* pcMessage_, ...)
{
   char acString[DSI_DEBUG_MAX_STRLEN + 1];  //+1 so truncation error will be detected by ThreadWrite
   va_list args;
   va_start(args, pcMessage_);
   VSNPRINTF(acString, DSI_DEBUG_MAX_STRLEN, pcMessage_, args);
   va_end(args);

   return ThreadWrite(acString);
}

BOOL DSIDebug::ThreadEnable(BOOL bEnable_)
{
   if(!bInitialized)
      return FALSE;

   //Find the index of the proper thread property struct
   UCHAR ucThreadNum;
   if(!FindThreadNum(&ucThreadNum))
      return FALSE;

   if(apstThread[ucThreadNum] == NULL)
      return FALSE;

   apstThread[ucThreadNum]->pclBuffer->SetEnable(bEnable_);
   return TRUE;
}

BOOL DSIDebug::SerialWrite(UCHAR ucPortNum_, const char* pcHeader_, UCHAR* pucData_, USHORT usSize_)
{
   if(!bInitialized)
      return FALSE;

   //pcHeader_ == NULL and pucData_ == NULL taken care of below.

   if(ucPortNum_ >= MAX_PORTS) // || usSize_ < 0)
      return FALSE;

   if(!bWriteEnable)
      return FALSE;

   //Check if the serial buffer has been created yet
   if(apclSerialBuffer[ucPortNum_] == NULL)  //is just here so you don't lock the mutex every time you write.
   {
      DSIThread_MutexLock(&stSerialBufferMutex);   //Note: remember this mutex is shared among every thread that writes to any serial buffer!
      if(apclSerialBuffer[ucPortNum_] == NULL && bWriteEnable)
      {
         UCHAR aucString[MAX_NAME_LENGTH];
         #if defined (DSI_TYPES_MACINTOSH)
            SNPRINTF((char*)aucString, MAX_NAME_LENGTH, "Device%u.log", ucPortNum_);
         #else
            SNPRINTF((char*)aucString, MAX_NAME_LENGTH, "Device%u.txt", ucPortNum_);
         #endif
         apclSerialBuffer[ucPortNum_] = new Buffer(aucString, aucLogDirectory);
      }
      DSIThread_MutexUnlock(&stSerialBufferMutex);
   }

   if(apclSerialBuffer[ucPortNum_] == NULL || !bWriteEnable)
      return FALSE;

   //Get the current time
   ULONG ulCurrentTime = DSIThread_GetSystemTime();

   //Compose the string
   char acString[DSI_DEBUG_MAX_STRLEN];
   SNPRINTF(acString, DSI_DEBUG_MAX_STRLEN, "%10.3f {%10lu} %s - %s", (ulCurrentTime-ulStartTime)/1000.0, ulCurrentTime, pcHeader_ != NULL ? (char*)pcHeader_ : "NULL", usSize_ == 0 ? "NO DATA\n" : "");
   ULONG ulStringLength = strlen(acString);

   if(usSize_ != 0 && ulStringLength < (DSI_DEBUG_MAX_STRLEN - 6))   //6 is room to display at least one byte
   {
      //Write all the bytes we can and put '\n' on the last one
      char* currentPos = acString + ulStringLength;
      USHORT usMaxDataCount = (USHORT)MIN(usSize_, (DSI_DEBUG_MAX_STRLEN-2-ulStringLength)/4); //2 is room for the closing "\n\0"
      for(USHORT i=0; i < usMaxDataCount-1; ++i)
      {
         SNPRINTF(currentPos, 5, "[%02X]", pucData_[i]);
         currentPos += 4;
      }
      SNPRINTF(currentPos, 6, "[%02X]\n", pucData_[usMaxDataCount-1]);

      //Update our string length
      ulStringLength += ((ULONG)usMaxDataCount*4) + 1; //4*bytes + '\n'
   }

   //If we are too long, than overwrite the truncate error to the end
   if(ulStringLength >= DSI_DEBUG_MAX_STRLEN-1)
      SNPRINTF(acString + DSI_DEBUG_MAX_STRLEN - 2 - strlen(TRUNCATE_ERROR), strlen(TRUNCATE_ERROR)+2, "%s\n", TRUNCATE_ERROR);

   //Add the string to the buffer
   return apclSerialBuffer[ucPortNum_]->Add((UCHAR*)acString, ulStringLength);
}

BOOL DSIDebug::SerialEnable(UCHAR ucPortNum_, BOOL bEnable_)
{
   if(!bInitialized)
      return FALSE;

   if(apclSerialBuffer[ucPortNum_] == NULL)
      return FALSE;

   apclSerialBuffer[ucPortNum_]->SetEnable(bEnable_);
   return TRUE;
}

void DSIDebug::SetDebug(BOOL bDebugOn_)
{
   bWriteEnable = bDebugOn_;
}


//////////////////////////////////////////////////////////
// Private Definitions
//////////////////////////////////////////////////////////

//Returns TRUE if buffer is not full and
//pucNum_ will either be the found thread num or an available thread num
//to use.  Otherwise, pucNum_ is invalid.
BOOL FindThreadNum(UCHAR* pucNum_)
{
   if(pucNum_ == NULL)
      return FALSE;

   UCHAR i;
   BOOL bNotFull = FALSE;
   DSI_THREAD_IDNUM hThreadIDNum = DSIThread_GetCurrentThreadIDNum();
   for(i=0; i<MAX_THREADS; i++)
   {
      if(apstThread[i] != NULL)
      {
         if(DSIThread_CompareThreads(hThreadIDNum, apstThread[i]->hThreadIDNum))
         {
            *pucNum_ = i;
            return TRUE;
         }
      }
      else
      {
         *pucNum_ = i;
         bNotFull = TRUE;
      }
   }

   return bNotFull;
}



//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
////////                   Buffer Class                           ////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////
// Public Definitions
//////////////////////////////////////////////////////////

Buffer::Buffer(UCHAR* pucFilename_, UCHAR* pucDirectory_)
{
   if(pucFilename_ == NULL || pucDirectory_ == NULL)
   {
      bEnable = FALSE;
      return;
   }

   STRNCPY((char*)aucFilename, (char*)pucFilename_, MAX_NAME_LENGTH);
   SNPRINTF((char*)aucFullPath, MAX_NAME_LENGTH, "%s%s", pucDirectory_, aucFilename);

   DSIThread_MutexInit(&stInputMutex);

   bEnable = TRUE;
   ulInput = 0;
   ulOutput = 0;

   //Filename Mutex
   DSIThread_MutexInit(&stFilenameMutex);

   //Thread variables
   DSIThread_MutexInit(&stFlushMutex);
   DSIThread_CondInit(&stFlushCond);
   bForceFlush = FALSE;
   bBufferOverflow = FALSE;

   DSIThread_MutexInit(&stWriteThreadMutex);
   DSIThread_CondInit(&stWriteThreadCond);
   bWriteThreadExit = FALSE;

   hWriteThreadID = DSIThread_CreateThread(&Buffer::StartWriteThread, this);

}

Buffer::~Buffer()
{
   //Handle output - Exit write thread
   DSIThread_MutexLock(&stWriteThreadMutex);
   if(hWriteThreadID)
   {

      bWriteThreadExit = TRUE;

      //Signal thread to exit.
      SignalFlush();

      //Wait for thread to exit.
      if(DSIThread_CondTimedWait(&stWriteThreadCond, &stWriteThreadMutex, THREAD_EXIT_TIMEOUT) != DSI_THREAD_ENONE)
      {
         DSIThread_DestroyThread(hWriteThreadID);
      }
      DSIThread_ReleaseThreadID(hWriteThreadID);
      hWriteThreadID = (DSI_THREAD_ID)NULL;
   }
   DSIThread_MutexUnlock(&stWriteThreadMutex);

   DSIThread_MutexDestroy(&stWriteThreadMutex);
   DSIThread_CondDestroy(&stWriteThreadCond);



   //Handle input
   DSIThread_MutexLock(&stInputMutex);
      bEnable = FALSE;
   DSIThread_MutexUnlock(&stInputMutex);
   DSIThread_Sleep(MUTEX_UNLOCK_DELAY*4);  //wait for any remaining threads to unlock mutex

   //Destroy input mutex
   DSIThread_MutexDestroy(&stInputMutex);

   //Destroy flush mutex/cond
   DSIThread_MutexDestroy(&stFlushMutex);
   DSIThread_CondDestroy(&stFlushCond);

   //Flush the buffer to make sure it is completely empty.
   Flush();

   //Destroy filename mutex
   DSIThread_MutexDestroy(&stFilenameMutex);
}

void Buffer::SetEnable(BOOL bEnable_)
{
   //Variable doesn't change
   if(bEnable == bEnable_)
      return;

   //Enabling buffer
   if(!bEnable)
   {
      bEnable = TRUE;
      return;
   }

   //Disabling buffer
   if(bEnable)
   {
      //Set the enable variable
      bEnable = FALSE;

      //Force a flush
      SignalFlush();
   }
}

BOOL Buffer::SetDirectory(const UCHAR* pucDirectory_)
{
   if(pucDirectory_ == NULL)
      return FALSE;

   //Check if the buffer is enabled
   if(!bEnable)
      return FALSE;

   SignalFlush();
   DSIThread_Sleep(MUTEX_UNLOCK_DELAY);  //wait for flush to finish

   DSIThread_MutexLock(&stFilenameMutex);
   SNPRINTF((char*)aucFullPath, MAX_NAME_LENGTH, "%s%s", pucDirectory_, aucFilename);
   DSIThread_MutexUnlock(&stFilenameMutex);
   return TRUE;
}

//Called by program threads
//Note: Thread could be in this function when class is destroyed and code is deallocated.
BOOL Buffer::Add(UCHAR* pucString_, ULONG ulSize_)
{
   if(pucString_ == NULL || ulSize_ > BUFFER_SIZE)
      return FALSE;

   //Check if the buffer is enabled
   if(!bEnable)
      return FALSE;

   DSIThread_MutexLock(&stInputMutex);

   //Check if the buffer is enabled
   if(!bEnable)
   {
      DSIThread_MutexUnlock(&stInputMutex);
      return FALSE;
   }

   //Check if buffer has enough room for the message
   if(GetBufferCount() + ulSize_ > BUFFER_SIZE)  //Note: possible railing if GetBufferCount()+ulSize_ > MAX_ULONG
   {

      if(!bBufferOverflow)
      {
         //Set up error message
         UCHAR aucErrorMesg[OVERFLOW_ERROR_LENGTH+1];
         STRNCPY((char*)aucErrorMesg, OVERFLOW_ERROR, OVERFLOW_ERROR_LENGTH+1);

         //Add the error message to the buffer
         ULONG ulNewInput = ulInput;
         for(ULONG i=0; i<OVERFLOW_ERROR_LENGTH; i++)
         {
            aucData[ulNewInput] = aucErrorMesg[i];
            ulNewInput++;
            ulNewInput %= ACTUAL_BUFFER_SIZE;
         }

         //Update the input pointer
         ulInput = ulNewInput;

         bBufferOverflow = TRUE;
      }

      //Signal WriteThread to flush
      SignalFlush();

      DSIThread_MutexUnlock(&stInputMutex);
      return FALSE;
   }

   //We don't have a buffer error anymore
   bBufferOverflow = FALSE;

   //Add the message to the buffer
   ULONG ulNewInput = ulInput;
   for(ULONG i=0; i<ulSize_; i++)
   {
      aucData[ulNewInput] = pucString_[i];
      ulNewInput++;
      ulNewInput %= ACTUAL_BUFFER_SIZE;
   }

   //Update the input pointer
   ulInput = ulNewInput;

   //Send a flush signal if the buffer is getting full
   if(GetPercentFull() >= FLUSH_PERCENT)
      SignalFlush();

   DSIThread_MutexUnlock(&stInputMutex);

   return TRUE;
}


//////////////////////////////////////////////////////////
// Private Definitions
//////////////////////////////////////////////////////////

//Called by WriteThread
BOOL Buffer::Flush()
{
   bForceFlush = FALSE;

   //Get size of array we are going to write
   ULONG ulFixedCount = GetBufferCount();

   if (ulFixedCount == 0)
      return TRUE;

   //Open the file
   DSIThread_MutexLock(&stFilenameMutex);
   FILE* pfFile = FOPEN((char*)aucFullPath, "a");
   DSIThread_MutexUnlock(&stFilenameMutex);
   if(pfFile == NULL)
      return FALSE;


   //Write data to file
   if(ulFixedCount + ulOutput <= ACTUAL_BUFFER_SIZE)
   {
      //We don't go off the edge of the array, so we can write it all at once
      if(fwrite(&aucData[ulOutput], sizeof(UCHAR), ulFixedCount, pfFile) != ulFixedCount)
         fwrite(WRITE_ERROR, sizeof(UCHAR), strlen(WRITE_ERROR), pfFile);

   }
   else
   {
      //Write to end of array
      if(fwrite(&aucData[ulOutput], sizeof(UCHAR), ACTUAL_BUFFER_SIZE-ulOutput, pfFile) != ACTUAL_BUFFER_SIZE-ulOutput)
         fwrite(WRITE_ERROR, sizeof(UCHAR), strlen(WRITE_ERROR), pfFile);

      ULONG ulLeft = ulFixedCount - (ACTUAL_BUFFER_SIZE-ulOutput);

      //Write from beginning of array
      if(fwrite(&aucData[0], sizeof(UCHAR), ulLeft, pfFile) != ulLeft)
         fwrite(WRITE_ERROR, sizeof(UCHAR), strlen(WRITE_ERROR), pfFile);
   }

   //Close file
   fclose(pfFile);

   //Update the output pointer
   ULONG ulNewOutput = (ulOutput + ulFixedCount)%ACTUAL_BUFFER_SIZE;
   ulOutput = ulNewOutput;

   return TRUE;
}

ULONG Buffer::GetBufferCount()
{
   //Lock values of input and output pointers
   ULONG ulFixedInput = ulInput;
   ULONG ulFixedOutput = ulOutput;

   ULONG ulFixedCount;
   if(ulFixedInput >= ulFixedOutput)
      ulFixedCount = ulFixedInput - ulFixedOutput;
   else
      ulFixedCount = ACTUAL_BUFFER_SIZE - (ulFixedOutput - ulFixedInput);

   return ulFixedCount;
}

UCHAR Buffer::GetPercentFull()
{
   return (UCHAR)((double)GetBufferCount()/BUFFER_SIZE * 100);
}

void Buffer::SignalFlush()
{
   DSIThread_MutexLock(&stFlushMutex);
      bForceFlush = TRUE;
      DSIThread_CondSignal(&stFlushCond);
   DSIThread_MutexUnlock(&stFlushMutex);
}

//////////////////
// Write Thread //
//////////////////

void Buffer::WriteThread()
{

   while(!bWriteThreadExit)
   {
      if(bEnable || bForceFlush)
         this->Flush();

      DSIThread_MutexLock(&stFlushMutex);
      if(!bForceFlush && !bWriteThreadExit)
         DSIThread_CondTimedWait(&stFlushCond, &stFlushMutex, FLUSH_CHECK_PERIOD);
      DSIThread_MutexUnlock(&stFlushMutex);
   }


   //Exit thread
   DSIThread_MutexLock(&stWriteThreadMutex);
      DSIThread_CondSignal(&stWriteThreadCond);
      bWriteThreadExit = TRUE;
   DSIThread_MutexUnlock(&stWriteThreadMutex);

   return;
}


DSI_THREAD_RETURN Buffer::StartWriteThread(void* pvParam_)
{
   if(pvParam_ == NULL)
      return 0;

   Buffer* pclBuffer = (Buffer*)pvParam_;
   pclBuffer->WriteThread();
   return 0;
}


#endif /* DEBUG_FILE */
