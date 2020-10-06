/*
This software is subject to the license described in the License.txt file
included with this software distribution. You may not use this file except
in compliance with this license.

Copyright (c) Dynastream Innovations Inc. 2016
All rights reserved.
*/
#include "types.h"
#if defined(DSI_TYPES_WINDOWS)

#include "dsi_libusb_library.hpp"

#include <memory>

using namespace std;

#define LIBRARY_NAME    "libusb0.dll"

//Static variable initializations
std::auto_ptr<LibusbLibrary> LibusbLibrary::clAutoInstance(NULL);
BOOL LibusbLibrary::bStaticSet = FALSE;

                                                                     //return an auto_ptr?
BOOL LibusbLibrary::Load(auto_ptr<const LibusbLibrary>& clAutoLibrary_)  //!!Should we lose the dependency on auto_ptr and just return the pointer (and let the user make their own auto_ptr)?
{
   try
   {
      clAutoLibrary_.reset(new LibusbLibrary());
   }
   catch(...)
   {
      clAutoLibrary_.reset(NULL);
      return FALSE;
   }

   return TRUE;
}


LibusbLibrary::LibusbLibrary() //throw(LibusbError::Enum&)
:
   ClaimInterface(NULL),
   GetBusses(NULL),
   ClearHalt(NULL),
   GetStringSimple(NULL),
   Close(NULL),
   Reset(NULL),
   Init(NULL),
   FindBusses(NULL),
   FindDevices(NULL),
   Open(NULL),
   ReleaseInterface(NULL),
   InterruptWrite(NULL),
   BulkRead(NULL),
   Device(NULL),
   SetConfiguration(NULL),
   SetDebug(NULL),

   BulkSetupAsync(NULL),
   SubmitAsync(NULL),
   ReapAsyncNocancel(NULL),
   CancelAsync(NULL),
   FreeAsync(NULL),

   hLibHandle(NULL)
{
   //Check static instance
   if(bStaticSet == FALSE)
   {
      bStaticSet = TRUE;
      try
      {
         clAutoInstance.reset(new LibusbLibrary());
      }
      catch(...)
      {
         bStaticSet = FALSE;
         throw;
      }
   }

   //load library
   LibusbError::Enum ret = LoadFunctions();
   if(ret != LibusbError::NONE)
      throw(ret);

   return;
}

LibusbLibrary::~LibusbLibrary() throw()
{
   FreeFunctions();
}


///////////////////////////////////////////////////////////////////////
// Loads USB interface functions from the DLLs.
///////////////////////////////////////////////////////////////////////
LibusbError::Enum LibusbLibrary::LoadFunctions()
{
   hLibHandle = LoadLibrary(LIBRARY_NAME);
   if(hLibHandle == NULL)
      return LibusbError::NO_LIBRARY;

   BOOL bStatus = TRUE;

   ClaimInterface = (ClaimInterface_t)GetProcAddress(hLibHandle, "usb_claim_interface");
   if(ClaimInterface == NULL)
      bStatus = FALSE;

   GetBusses = (GetBusses_t)GetProcAddress(hLibHandle, "usb_get_busses");
   if(GetBusses == NULL)
      bStatus = FALSE;

   ClearHalt = (ClearHalt_t)GetProcAddress(hLibHandle, "usb_clear_halt");
   if(ClearHalt == NULL)
      bStatus = FALSE;

   GetStringSimple = (GetStringSimple_t)GetProcAddress(hLibHandle, "usb_get_string_simple");
   if(GetStringSimple == NULL)
      bStatus = FALSE;

   Close = (Close_t)GetProcAddress(hLibHandle, "usb_close");
   if(Close == NULL)
      bStatus = FALSE;

   Reset = (Reset_t)GetProcAddress(hLibHandle, "usb_reset");
   if(Reset == NULL)
      bStatus = FALSE;

   Init = (Init_t)GetProcAddress(hLibHandle, "usb_init");
   if(Init == NULL)
      bStatus = FALSE;

   FindBusses = (FindBusses_t)GetProcAddress(hLibHandle, "usb_find_busses");
   if(FindBusses == NULL)
      bStatus = FALSE;

   FindDevices = (FindDevices_t)GetProcAddress(hLibHandle, "usb_find_devices");
   if(FindDevices == NULL)
      bStatus = FALSE;

   Open = (Open_t)GetProcAddress(hLibHandle, "usb_open");
   if(Open == NULL)
      bStatus = FALSE;

   ReleaseInterface = (ReleaseInterface_t)GetProcAddress(hLibHandle, "usb_release_interface");
   if(ReleaseInterface == NULL)
      bStatus = FALSE;

   InterruptWrite = (InterruptWrite_t)GetProcAddress(hLibHandle, "usb_interrupt_write");
   if(InterruptWrite == NULL)
      bStatus = FALSE;

   BulkRead = (BulkRead_t)GetProcAddress(hLibHandle, "usb_bulk_read");
   if(BulkRead == NULL)
      bStatus = FALSE;

   Device = (Device_t)GetProcAddress(hLibHandle, "usb_device");
   if(Device == NULL)
      bStatus = FALSE;

   SetConfiguration = (SetConfiguration_t)GetProcAddress(hLibHandle, "usb_set_configuration");
   if(SetConfiguration == NULL)
      bStatus = FALSE;

   SetDebug = (SetDebug_t)GetProcAddress(hLibHandle, "usb_set_debug");
   if(SetDebug == NULL)
      bStatus = FALSE;

   //Libusb-win32 only asynch functions
   BulkSetupAsync = (BulkSetupAsync_t)GetProcAddress(hLibHandle, "usb_bulk_setup_async");
   if(BulkSetupAsync == NULL)
      bStatus = FALSE;

   SubmitAsync = (SubmitAsync_t)GetProcAddress(hLibHandle, "usb_submit_async");
   if(SubmitAsync == NULL)
      bStatus = FALSE;

   ReapAsyncNocancel = (ReapAsyncNocancel_t)GetProcAddress(hLibHandle, "usb_reap_async_nocancel");
   if(ReapAsyncNocancel == NULL)
      bStatus = FALSE;

   CancelAsync = (CancelAsync_t)GetProcAddress(hLibHandle, "usb_cancel_async");
   if(CancelAsync == NULL)
      bStatus = FALSE;

   FreeAsync = (FreeAsync_t)GetProcAddress(hLibHandle, "usb_free_async");
   if(CancelAsync == NULL)
      bStatus = FALSE;


   if(bStatus == FALSE)
   {
      FreeFunctions();
      return LibusbError::NO_FUNCTION;
   }

   return LibusbError::NONE;
}

///////////////////////////////////////////////////////////////////////
// Unloads USB DLLs.
///////////////////////////////////////////////////////////////////////
void LibusbLibrary::FreeFunctions()
{
   if(hLibHandle != NULL)
   {
      FreeLibrary(hLibHandle);
      hLibHandle = NULL;
   }

   return;
}

#endif //defined(DSI_TYPES_WINDOWS)