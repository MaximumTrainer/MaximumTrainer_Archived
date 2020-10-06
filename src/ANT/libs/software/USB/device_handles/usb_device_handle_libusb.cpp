/*
This software is subject to the license described in the License.txt file
included with this software distribution. You may not use this file except
in compliance with this license.

Copyright (c) Dynastream Innovations Inc. 2016
All rights reserved.
*/
#include "types.h"
#if defined(DSI_TYPES_WINDOWS)


#include "usb_device_handle_libusb.hpp"

#include "macros.h"
#include "usb_device_list.hpp"
#include "dsi_debug.hpp"
#include "antmessage.h"

#include <memory>

using namespace std;

//////////////////////////////////////////////////////////////////////////////////
// Static declarations
//////////////////////////////////////////////////////////////////////////////////

USBDeviceList<const USBDeviceLibusb> USBDeviceHandleLibusb::clDeviceList;


//////////////////////////////////////////////////////////////////////////////////
// Private Definitions
//////////////////////////////////////////////////////////////////////////////////

const UCHAR USB_ANT_CONFIGURATION = 1;
const UCHAR USB_ANT_INTERFACE = 0;
const UCHAR USB_ANT_EP_IN  = 0x81;
const UCHAR USB_ANT_EP_OUT = 0x01;

BOOL CanOpenDevice(const USBDeviceLibusb*const & pclDevice_)  //!!Should we make a static member function that does a more efficient try open (doesn't start a receive thread, etc.)
{
   if(pclDevice_ == FALSE)
      return FALSE;

   return USBDeviceHandleLibusb::TryOpen(*pclDevice_);
}

const USBDeviceListLibusb USBDeviceHandleLibusb::GetAllDevices()  //!!List needs to be deleted!
{
   USBDeviceListLibusb clList;

   clDeviceList = USBDeviceList<const USBDeviceLibusb>();  //clear device list

   //Get a reference to library
   auto_ptr<const LibusbLibrary> pclAutoLibusbLibrary(NULL);
   if(LibusbLibrary::Load(pclAutoLibusbLibrary) == FALSE)
      return clList;
   const LibusbLibrary& clLibusbLibrary = *pclAutoLibusbLibrary;


   int ret;

   //Initialize libusb
   clLibusbLibrary.Init();
   //clLibusbLibrary.SetDebug(255);


   //Find all USB busses on a system
   ret = clLibusbLibrary.FindBusses();
   if(ret < 0) return clList;

   //Find all devices on each bus
   ret = clLibusbLibrary.FindDevices();
   if(ret < 0) return clList;

   //Opens proper device as specified by VIDNUM
   for(usb_bus* bus = clLibusbLibrary.GetBusses(); bus; bus = bus->next)
   {
      struct usb_device* dev;
      /* //!!
      if(bus->root_dev)
      {
         if(dev->descriptor.idVendor == VID)
         {
            //We are communicating to the right device
            device = dev;
            return TRUE;
         }
      }
      */
      for(dev = bus->devices; dev; dev = dev->next)
      {
         clDeviceList.Add( USBDeviceLibusb(*dev) );  //save the copies to the static private list
         clList.Add( clDeviceList.GetAddress(clDeviceList.GetSize()-1) );  //save a pointer to the just added device
      }
   }

   return clList;
}


//!!Will we have a problem if someone asks for a device list twice? (will it dereference the pointers in the first list?)
const USBDeviceListLibusb USBDeviceHandleLibusb::GetAvailableDevices()  //!!List needs to be deleted!
{
   return USBDeviceHandleLibusb::GetAllDevices().GetSubList(CanOpenDevice);
}


BOOL USBDeviceHandleLibusb::Open(const USBDeviceLibusb& clDevice_, USBDeviceHandleLibusb*& pclDeviceHandle_)
{
   try
   {
      pclDeviceHandle_ = new USBDeviceHandleLibusb(clDevice_);
   }
   catch(...)
   {
      pclDeviceHandle_ = NULL;
      return FALSE;
   }

   // Workaround for USB2/m firmware error mishandling USB 'clear feature' request (See Jira ANTPC-45)
   // Firmware data toggle syncronization and busy bit become out of sync, and can be fixed by
   // sending two ANT requests.
   // In the case that the USB pipe is not totally out of sync, and some responses are received,
   // we make sure we read all that data so the workaround is invisible to the app.
   UCHAR aucReqCapabilitiesMsg[MESG_FRAME_SIZE + 2] = {0xA4, 0x02, 0x4D, 0x00, 0x54, 0xBF};
   UCHAR aucCapabilitiesMsg[MESG_MAX_SIZE];
   ULONG ulBytesWritten, ulBytesRead = 0;

   pclDeviceHandle_->Write(aucReqCapabilitiesMsg, sizeof(aucReqCapabilitiesMsg), ulBytesWritten);
   pclDeviceHandle_->Write(aucReqCapabilitiesMsg, sizeof(aucReqCapabilitiesMsg), ulBytesWritten);
   pclDeviceHandle_->Read(aucCapabilitiesMsg, sizeof(aucCapabilitiesMsg), ulBytesRead, 10);
   pclDeviceHandle_->Read(aucCapabilitiesMsg, sizeof(aucCapabilitiesMsg), ulBytesRead, 10);

   return TRUE;
}



BOOL USBDeviceHandleLibusb::Close(USBDeviceHandleLibusb*& pclDeviceHandle_, BOOL bReset_)
{
   if(pclDeviceHandle_ == NULL)
      return FALSE;

   pclDeviceHandle_->PClose(bReset_);
   delete pclDeviceHandle_;
   pclDeviceHandle_ = NULL;

   return TRUE;
}

//A more efficient way to test if you can open a device.  For instance, this function won't create a receive loop, etc.)
BOOL USBDeviceHandleLibusb::TryOpen(const USBDeviceLibusb& clDevice_)
{
   //Get a reference to library
   auto_ptr<const LibusbLibrary> pclAutoLibusbLibrary(NULL);
   if(LibusbLibrary::Load(pclAutoLibusbLibrary) == FALSE)
      return FALSE;
   const LibusbLibrary& clLibusbLibrary = *pclAutoLibusbLibrary;

   clLibusbLibrary.Init();
   //pfSetDebug(255);

   usb_dev_handle* pclTempDeviceHandle;
   pclTempDeviceHandle = clLibusbLibrary.Open(&(clDevice_.GetRawDevice()));
   if(pclTempDeviceHandle == NULL)
      return FALSE;  //Doesn't need to call Close because it wouldn't do anything

   int ret;

   ret = clLibusbLibrary.SetConfiguration(pclTempDeviceHandle, USB_ANT_CONFIGURATION);
   if(ret != 0)
   {
      ret = clLibusbLibrary.Close(pclTempDeviceHandle);
      if(ret != 0) {}  //this would be an error

      return FALSE;
   }

   ret = clLibusbLibrary.ClaimInterface(pclTempDeviceHandle, USB_ANT_INTERFACE);
   if(ret != 0)
   {
      ret = clLibusbLibrary.Close(pclTempDeviceHandle);
      if(ret != 0) {}  //this would be an error

      return FALSE;
   }

   ret = clLibusbLibrary.ReleaseInterface(pclTempDeviceHandle, USB_ANT_INTERFACE);
   if(ret != 0) {}  //this would be an error

   ret = clLibusbLibrary.Close(pclTempDeviceHandle);
   if(ret != 0) {}  //this would be an error

   return TRUE;
}


///////////////////////////////////////////////////////////////////////
// Constructor
///////////////////////////////////////////////////////////////////////

USBDeviceHandleLibusb::USBDeviceHandleLibusb(const USBDeviceLibusb& clDevice_)
try
:
   USBDeviceHandle(),
   clDevice(clDevice_), //!!Copy?
   bDeviceGone(TRUE)
{
   hReceiveThread = NULL;
   bStopReceiveThread = TRUE;
   device_handle = NULL;

   clLibusbLibrary.Init();
   //pfSetDebug(255);

   if(POpen() == FALSE)
      throw 0; //!!We need something to throw

   return;
}
catch(...)
{
   throw;
}

///////////////////////////////////////////////////////////////////////
// Destructor
///////////////////////////////////////////////////////////////////////
USBDeviceHandleLibusb::~USBDeviceHandleLibusb()
{
   //!!Delete all the elements in the clOverflowQueue!
}



///////////////////////////////////////////////////////////////////////
// Opens port, starts receive thread.
///////////////////////////////////////////////////////////////////////
BOOL USBDeviceHandleLibusb::POpen()
{
   int ret;

   //Make sure we are not open before opening again.
   PClose();  //!!Do we want to reset here?

   device_handle = clLibusbLibrary.Open(&(clDevice.GetRawDevice()));
   if(device_handle == NULL)
      return FALSE;  //Doesn't need to call Close because it wouldn't do anything

   bDeviceGone = FALSE;

   //!!Will this do a soft reset if the current configuration gets set?
   ret = clLibusbLibrary.SetConfiguration(device_handle, USB_ANT_CONFIGURATION);
   if(ret != 0)
   {
      PClose();
      return FALSE;
   }

   ret = clLibusbLibrary.ClaimInterface(device_handle, USB_ANT_INTERFACE);
   if(ret != 0)
   {
      PClose();
      return FALSE;
   }

   /* //!!Fails
   ret = usb_set_altinterface(device_handle, 0);
   if(ret != 0)
   {
      Close();
      return FALSE;
   }
   */


   #if !defined(DSI_TYPES_WINDOWS)     //!!Fails on Windows
   {
      ret = clLibusbLibrary.ClearHalt(device_handle, USB_ANT_EP_OUT);
      if(ret != 0)
      {
         Close();
         return FALSE;
      }
   }
   #endif

   if(DSIThread_MutexInit(&stMutexCriticalSection) != DSI_THREAD_ENONE)
   {
      PClose();
      return FALSE;
   }

   if(DSIThread_CondInit(&stEventReceiveThreadExit) != DSI_THREAD_ENONE)
   {
      DSIThread_MutexDestroy(&stMutexCriticalSection);
      PClose();
      return FALSE;
   }

   bStopReceiveThread = FALSE;
   hReceiveThread = DSIThread_CreateThread(&USBDeviceHandleLibusb::ProcessThread, this);
   if (hReceiveThread == NULL)
   {
      DSIThread_CondDestroy(&stEventReceiveThreadExit);
      DSIThread_MutexDestroy(&stMutexCriticalSection);
      PClose();
      return FALSE;
   }

   return TRUE;
}


///////////////////////////////////////////////////////////////////////
// Closes the USB connection, kills receive thread.
///////////////////////////////////////////////////////////////////////
void USBDeviceHandleLibusb::PClose(BOOL bReset_)
{

   bDeviceGone = TRUE;

   if (hReceiveThread)
   {
      DSIThread_MutexLock(&stMutexCriticalSection);
      if(bStopReceiveThread == FALSE)
      {
         bStopReceiveThread = TRUE;

         if (DSIThread_CondTimedWait(&stEventReceiveThreadExit, &stMutexCriticalSection, 3000) != DSI_THREAD_ENONE)
         {
            // We were unable to stop the thread normally.
            DSIThread_DestroyThread(hReceiveThread);
         }
      }
      DSIThread_MutexUnlock(&stMutexCriticalSection);

      DSIThread_ReleaseThreadID(hReceiveThread);
      hReceiveThread = NULL;

      DSIThread_MutexDestroy(&stMutexCriticalSection);
      DSIThread_CondDestroy(&stEventReceiveThreadExit);
   }


    if(device_handle != (usb_dev_handle*)NULL)
    {

      int ret;

      ret = clLibusbLibrary.ReleaseInterface(device_handle, USB_ANT_INTERFACE);
      if(ret != 0) {}  //this would be an error

      if(bReset_)
      {
//         ret = clLibusbLibrary.Reset(device_handle);  //The library reset function can sometimes cause the device/driver to go into an unusable state, long term stability testing shows there is no benifit to including reset for this device/driver combination.
         ret = clLibusbLibrary.Close(device_handle);
         if(ret != 0) {}  //this would be an error
      }
      else
      {
         ret = clLibusbLibrary.Close(device_handle);
         if(ret != 0) {}  //this would be an error
      }

      device_handle = (usb_dev_handle*)NULL;
   }

}

///////////////////////////////////////////////////////////////////////
// Writes usSize_ bytes to USB, returns TRUE if successful.
///////////////////////////////////////////////////////////////////////

//!!Return true if we wrote half the bytes successfully?
USBError::Enum USBDeviceHandleLibusb::Write(void* pvData_, ULONG ulSize_, ULONG& ulBytesWritten_)
{
   ulBytesWritten_ = 0;

   if(bDeviceGone)
      return USBError::DEVICE_GONE;

   if(pvData_ == NULL)
      return USBError::INVALID_PARAM;

   //!!What happens if usSize_ == 0?
   int theWriteCount;
   theWriteCount = clLibusbLibrary.InterruptWrite(device_handle, USB_ANT_EP_OUT, (char*)pvData_, ulSize_, 3000);  //!!Non-blocking is supported in V1.0
   if(theWriteCount < 0)
      return USBError::FAILED;

   ulBytesWritten_ = theWriteCount;
   return USBError::NONE;
}



///////////////////////////////////////////////////////////////////////
USBError::Enum USBDeviceHandleLibusb::Read(void* pvData_, ULONG ulSize_, ULONG& ulBytesRead_, ULONG ulWaitTime_)
{
   if(bDeviceGone)
      return USBError::DEVICE_GONE;

   ulBytesRead_ = clRxQueue.PopArray(reinterpret_cast<UCHAR* const>(pvData_), ulSize_, ulWaitTime_);
   return USBError::NONE;
}



void USBDeviceHandleLibusb::ReceiveThread()
{
   #if defined(DEBUG_FILE)
   BOOL bRxDebug = DSIDebug::ThreadInit("ao_libusb_receive.txt");
   DSIDebug::ThreadEnable(TRUE);
   #endif

   UCHAR ucConsecIoErrors = 0;
   UCHAR aucData[4096];

   VOID *asyncContextObject = NULL;
   BOOL isRequestSubmitted = FALSE;

   INT iRet = clLibusbLibrary.BulkSetupAsync(device_handle, &asyncContextObject, USB_ANT_EP_IN);
   if(iRet < 0)
   {
      #if defined(DEBUG_FILE)
         char acMesg[255];
         SNPRINTF(acMesg, 255, "ReceiveThread(): BulkSetupAsync() Error: %d", iRet);
         DSIDebug::ThreadWrite(acMesg);
      #endif
      bStopReceiveThread = TRUE;
   }


   while(!bStopReceiveThread)
   {
      //size must be the largest usb message we will receive or else it will get dropped, or will
      //  get a -1 ("permission error") and then -104 ("connection reset by peer") errors!

      if(!isRequestSubmitted)
      {
         iRet = clLibusbLibrary.SubmitAsync(asyncContextObject, (char*)aucData, sizeof(aucData));
         if(iRet >= 0)
         {
            isRequestSubmitted = TRUE;
         }
         else
         {
            #if defined(DEBUG_FILE)
               char acMesg[255];
               SNPRINTF(acMesg, 255, "ReceiveThread(): SubmitAsync() Error: %d", iRet);
               DSIDebug::ThreadWrite(acMesg);
            #endif
            bStopReceiveThread = TRUE;
            break;
         }
      }

         iRet = clLibusbLibrary.ReapAsyncNocancel(asyncContextObject, 1000);

      if(iRet > 0)
      {
         clRxQueue.PushArray(aucData, iRet);
         isRequestSubmitted = FALSE;   //We need to resubmit the request or we will just get the same data again
         ucConsecIoErrors = 0;
      }
     else if(iRet == -116)
     {
        //an error occured,  errno -116: Connection timed out is usually not an error for us because
        //we are in a read loop but this error is also returned when the Windows sleeps and the GetOverlappedResult()
        //call in reap_async() returns Win error 995 ERROR_OPERATION_ABORTED
        //When the OS goes to sleep the stick powered down and lost its config, so we are hosed and need to restart
        if(GetLastError() == 995)
        {
           #if defined(DEBUG_FILE)
                if(bRxDebug)
                   DSIDebug::ThreadWrite("ReceiveThread(): ReapAsyncNocancel()=-116 && GetLastError()=995 => Sleep/Hibernate occured and reset device");
          #endif
          bStopReceiveThread = TRUE;
        }
     }
      else if(iRet < 0)
      {
         #if defined(DEBUG_FILE)
         if(bRxDebug)
         {
            char acMesg[255];
            SNPRINTF(acMesg, 255, "ReceiveThread(): ReapAsyncNocancel() Error: %d, Win_GetLastError: %lu", iRet, GetLastError());
            DSIDebug::ThreadWrite(acMesg);
         }
         #endif

         //We assume that the device is gone if we get multiple errors in a row
         ucConsecIoErrors++;

         if(ucConsecIoErrors == 10)
         {
            #if defined(DEBUG_FILE)
               if(bRxDebug)
                  DSIDebug::ThreadWrite("ReceiveThread(): ReapAsyncNocancel() Had 10 IO errors in a row.");
            #endif
            bStopReceiveThread = TRUE;
         }
         else  //We can try to cancel our request and start a new one to see if it fixes the problem
            //TODO //!!up to this point in time, we don't know what errors are actually causing this, so restarting the request may be entirely useless
         {
            if(clLibusbLibrary.CancelAsync(asyncContextObject) == 0
               && clLibusbLibrary.FreeAsync(&asyncContextObject) == 0
               && clLibusbLibrary.BulkSetupAsync(device_handle, &asyncContextObject, USB_ANT_EP_IN) == 0)
            {
               isRequestSubmitted = FALSE;
            }
            else
            {
               #if defined(DEBUG_FILE)
                  if(bRxDebug)
                     DSIDebug::ThreadWrite("ReceiveThread(): Failed to free old async request and create a new one.");
               #endif
               bStopReceiveThread = TRUE;
            }
         }
      }

   }

   bDeviceGone = TRUE;  //The read loop is dead, since we can't get any info, the device might as well be gone

   if(asyncContextObject != NULL)
   {
      iRet = clLibusbLibrary.CancelAsync(asyncContextObject);
      #if defined(DEBUG_FILE)
         if(iRet < 0)
         {
            char acMesg[255];
            SNPRINTF(acMesg, 255, "ReceiveThread(): CancelAsync() Error: %d\n", iRet);
            DSIDebug::ThreadWrite(acMesg);
         }
      #endif
      iRet = clLibusbLibrary.FreeAsync(&asyncContextObject);
      #if defined(DEBUG_FILE)
         if(iRet < 0)
         {
            char acMesg[255];
            SNPRINTF(acMesg, 255, "ReceiveThread(): FreeAsync() Error: %d\n", iRet);
            DSIDebug::ThreadWrite(acMesg);
         }
      #endif
     asyncContextObject = NULL;
   }

   DSIThread_MutexLock(&stMutexCriticalSection);
      bStopReceiveThread = TRUE;
      DSIThread_CondSignal(&stEventReceiveThreadExit);                       // Set an event to alert the main process that Rx thread is finished and can be closed.
   DSIThread_MutexUnlock(&stMutexCriticalSection);
}








/*
///////////////////////////////////////////////////////////////////////
USBError::Enum USBDeviceHandleLibusb::Read(void* pvData_, ULONG ulSize_, ULONG& ulBytesRead_, ULONG ulWaitTime_)
{

   ULONG ulSizeLeft = ulSize_;

   //give the user any data we may have in the data queue
   while(ulSizeLeft != 0 && clOverflowQueue.size() != 0)
   {
      SerialData* pstFrontArray = clOverflowQueue.front();
      ULONG ulDataSize = pstFrontArray->iSize - pstFrontArray->iDataStart;
      ULONG ulTransferSize = (ulSizeLeft < ulDataSize) ? ulSizeLeft : ulDataSize;  //min
      memcpy(pvData_, pstFrontArray->data, ulTransferSize);

      ulSizeLeft -= ulTransferSize;
      pstFrontArray->iDataStart += ulTransferSize;

      if(pstFrontArray->iSize - pstFrontArray->iDataStart == 0)
      {
         delete pstFrontArray;
         clOverflowQueue.pop_front();
      }

   }

   return ulSize_ - ulSizeLeft;
}




//!!Put read errors into the queue?
void USBDeviceHandleLibusb::ReceiveThread()
{
   UCHAR ucIoErrors = 0;
   while(!bStopReceiveThread)
   {
      //size must be the largest usb message we will receive or else it will get dropped, or will
      //  get a -1 ("permission error") and then -104 ("connection reset by peer") errors!

      SerialData* mesgin = new SerialData();
      mesgin->iSize = clLibusbLibrary.BulkRead(device_handle, USB_ANT_EP_IN, mesgin->data, sizeof(mesgin->data), ulWaitTime_);

      if(mesgin->iSize < 0 && mesgin->iSize != -116) //errno -116: Connection timed out
      {
         //We are assuming that the device is gone if we get multiple IO errors (errno -5) in a row
         if(mesgin->iSize == -5)
            ucIoErrors++;
         else
            ucIoErrors = 0;

         if(ucIoErrors == 10)
         {
            bDeviceGone = TRUE;
            bStopReceiveThread = TRUE;
         }

         delete mesgin;  //this data is no good
         mesgin = NULL;
      }
      else
      {
         clOverflowQueue.push_back(mesgin);
      }

   }

   bDeviceGone = TRUE;

   DSIThread_MutexLock(&stMutexCriticalSection);
      bStopReceiveThread = TRUE;
      DSIThread_CondSignal(&stEventReceiveThreadExit);                       // Set an event to alert the main process that Rx thread is finished and can be closed.
   DSIThread_MutexUnlock(&stMutexCriticalSection);
}
*/




///////////////////////////////////////////////////////////////////////
DSI_THREAD_RETURN USBDeviceHandleLibusb::ProcessThread(void* pvParameter_)
{
   USBDeviceHandleLibusb* This = reinterpret_cast<USBDeviceHandleLibusb*>(pvParameter_);
   This->ReceiveThread();

   return 0;
}

#endif //defined(DSI_TYPES_WINDOWS)
