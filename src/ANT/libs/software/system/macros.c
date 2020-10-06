/*
This software is subject to the license described in the License.txt file
included with this software distribution. You may not use this file except
in compliance with this license.

Copyright (c) Dynastream Innovations Inc. 2016
All rights reserved.
*/
#include "types.h"
#include "macros.h"

#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#if defined(_MSC_VER)

//////////////////////////////////////////////////////////////////////////////////
// Public Functions
//////////////////////////////////////////////////////////////////////////////////

int SNPRINTF(char* pcDst_, size_t uNum_, const char* pcFormat_, ...)
{
   va_list hList;
   int iWritten;

   if(uNum_ == 0)
      return -1;

   va_start(hList, pcFormat_);
   iWritten = vsnprintf_s(pcDst_, uNum_, _TRUNCATE, pcFormat_, hList);
   va_end(hList);

   if(iWritten == -1)   //check for truncation
      return ((int)(uNum_-1));

   return iWritten;
}

size_t VSNPRINTF(char* pcDst_, size_t uNum_, const char* pcFormat_, va_list args)
{
    int iWritten;

    if(uNum_ == 0)
       return -1;

    iWritten = vsnprintf_s(pcDst_, uNum_, _TRUNCATE, pcFormat_, args);

    if(iWritten == -1)   //check for truncation
       return uNum_-1;   //uNum-1 to match SNPRINTF return, don't know why it treats truncation that way

    return iWritten;
}

BOOL STRNCPY(char* pcDst_, const char* pcSrc_, size_t uNum_)
{

   if(strlen(pcSrc_) + 1 > uNum_)
      return FALSE;

   return (strcpy_s(pcDst_, uNum_, pcSrc_) == 0);
}

BOOL STRNCAT(char* pcDst_, const char* pcSrc_, size_t uNum_)
{
   if(strlen(pcDst_) + strlen(pcSrc_) + 1 > uNum_)
      return FALSE;

   return (strcat_s(pcDst_, uNum_, pcSrc_) == 0);
}

FILE* FOPEN(const char* pcFilename_, const char* pcMode_)
{
   FILE* pfFile;
   return (fopen_s(&pfFile, pcFilename_, pcMode_) == 0) ? pfFile : NULL;
}

#else

// NOTE: Alternative macro --> #define SNPRINTF(dst, num, fmt, ...)   (snprintf(dst, num, fmt, __VA_ARGS__) != -1 ? strlen(dst) : -1)
// C99 standard states that it will return the number of bytes that should have been written.  This is not necessarily the case with older libraries.
int SNPRINTF(char* pcDst_, size_t uNum_, const char* pcFormat_, ...)
{
   int iReturn;
   va_list hList;

   if(uNum_ == 0)
      return -1;


   if(uNum_ == 1)
   {
      pcDst_[0] = '\0';
      return 0;
   }


   va_start(hList, pcFormat_);
   iReturn = vsnprintf(pcDst_, uNum_, pcFormat_, hList);
   pcDst_[uNum_-1] = '\0';
   va_end(hList);

   if(iReturn < 0)   //check if an error occured
      return iReturn;

   if(iReturn >= (int)( uNum_-1) ) //check if output is truncated
      return uNum_-1;

   return iReturn;
}

size_t VSNPRINTF(char* pcDst_, size_t uNum_, const char* pcFormat_, va_list args)
{
   int iWritten;

   if(uNum_ == 0)
      return -1;

   iWritten = vsnprintf(pcDst_, uNum_, pcFormat_, args);

   if(iWritten >= uNum_) //check if output is truncated
      return uNum_-1; //uNum-1 to match SNPRINTF return, don't know why it treats truncation that way

   return iWritten;
}


BOOL STRNCPY(char* pcDst_, const char* pcSrc_, size_t uNum_)
{

   if(uNum_ == 0) //cannot copy a negative amount
      return FALSE;

   if(strlen(pcSrc_) + 1 > uNum_)  //make sure it won't get truncated
      return FALSE;

   strncpy(pcDst_, pcSrc_, uNum_-1);
   pcDst_[uNum_-1] = '\0';
   return TRUE;
}

BOOL STRNCAT(char* pcDst_, const char* pcSrc_, size_t uNum_)
{

   size_t uDstLen = strlen(pcDst_);
   if(uNum_ == 0 || uNum_-1 < uDstLen) //cannot concatenate a negative amount
      return FALSE;

   if(uDstLen + strlen(pcSrc_) + 1 > uNum_)
      return FALSE;

   strncat(pcDst_, pcSrc_, (uNum_-1)-uDstLen);
   return TRUE;
}

FILE* FOPEN(const char* pcFilename_, const char* pcMode_)
{
   return fopen(pcFilename_, pcMode_);
}


#endif
