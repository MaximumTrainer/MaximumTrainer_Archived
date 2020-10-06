/*
This software is subject to the license described in the License.txt file
included with this software distribution. You may not use this file except
in compliance with this license.

Copyright (c) Dynastream Innovations Inc. 2016
All rights reserved.
*/
#include "types.h"
#if defined(DSI_TYPES_WINDOWS)

#include "dsi_cm_library.hpp"

#include <memory>

using namespace std;

#define LIBRARY_NAME    "DSI_CP210xManufacturing_3_1.dll"

//Static variable initializations
std::auto_ptr<CMLibrary> CMLibrary::clAutoInstance(NULL);
BOOL CMLibrary::bStaticSet = FALSE;


                                                                     //return an auto_ptr?
BOOL CMLibrary::Load(auto_ptr<const CMLibrary>& clAutoLibrary_)  //!!Should we lose the dependency on auto_ptr and just return the pointer (and let the user make their own auto_ptr)?
{
   try
   {
      clAutoLibrary_.reset(new CMLibrary());
   }
   catch(...)
   {
      clAutoLibrary_.reset(NULL);
      return FALSE;
   }

   return TRUE;
}

CMLibrary::CMLibrary() //throw(CMError::Enum&)
:
   GetDeviceProductString(NULL),
   GetDeviceSerialNumber(NULL),
   GetDeviceVid(NULL),
   GetDevicePid(NULL),
   ResetDevice(NULL),

   hLibHandle(NULL)
{
   //Check static instance
   if(bStaticSet == FALSE)
   {
      bStaticSet = TRUE;
      try
      {
         clAutoInstance.reset(new CMLibrary());
      }
      catch(...)
      {
         bStaticSet = FALSE;
         throw;
      }
   }

   //load library
   CMError::Enum ret = LoadFunctions();
   if(ret != CMError::NONE)
      throw(ret);

   return;
}

CMLibrary::~CMLibrary() throw()
{
   FreeFunctions();
}


///////////////////////////////////////////////////////////////////////
// Loads USB interface functions from the DLLs.
///////////////////////////////////////////////////////////////////////
CMError::Enum CMLibrary::LoadFunctions()
{
   hLibHandle = LoadLibrary(LIBRARY_NAME);
   if(hLibHandle == NULL)
      return CMError::NO_LIBRARY;

   BOOL bStatus = TRUE;

   GetDeviceProductString = (GetDeviceProductString_t)GetProcAddress(hLibHandle, "CP210x_GetDeviceProductString");
   if(GetDeviceProductString == NULL)
      bStatus = FALSE;

   GetDeviceSerialNumber = (GetDeviceSerialNumber_t)GetProcAddress(hLibHandle, "CP210x_GetDeviceSerialNumber");
   if(GetDeviceSerialNumber == NULL)
      bStatus = FALSE;

   GetDeviceVid = (GetDeviceVid_t)GetProcAddress(hLibHandle, "CP210x_GetDeviceVid");
   if(GetDeviceVid == NULL)
      bStatus = FALSE;

   GetDevicePid = (GetDevicePid_t)GetProcAddress(hLibHandle, "CP210x_GetDevicePid");
   if(GetDevicePid == NULL)
      bStatus = FALSE;

   ResetDevice = (Reset_t)GetProcAddress(hLibHandle, "CP210x_Reset");
   if(ResetDevice == NULL)
      bStatus = FALSE;


   if(bStatus == FALSE)
   {
      FreeFunctions();
      return CMError::NO_FUNCTION;
   }

   return CMError::NONE;
}

///////////////////////////////////////////////////////////////////////
// Unloads USB DLLs.
///////////////////////////////////////////////////////////////////////
void CMLibrary::FreeFunctions()
{
   if(hLibHandle != NULL)
   {
      FreeLibrary(hLibHandle);
      hLibHandle = NULL;
   }

   return;
}

#endif //defined(DSI_TYPES_WINDOWS)