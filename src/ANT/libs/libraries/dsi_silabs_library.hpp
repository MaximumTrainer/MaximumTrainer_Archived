/*
This software is subject to the license described in the License.txt file
included with this software distribution. You may not use this file except
in compliance with this license.

Copyright (c) Dynastream Innovations Inc. 2016
All rights reserved.
*/
#ifndef DSI_SILABS_LIBRARY_HPP
#define DSI_SILABS_LIBRARY_HPP

#include "types.h"

#include "DSI_SiUSBXp_3_1.h"

#include <memory>

#include <windows.h>



struct SiLabsError
{
   enum Enum
   {
      NONE,
      NO_LIBRARY,
      NO_FUNCTION
   };

   private: SiLabsError();
};

/*
 * This class started out as a singleton, but you cannot control when a static variable destroys
 * itself at the end of an application.  Therefore, the class was changed to a normal wrapper around
 * the library.  This means that if you retrieve an instance of the class and then destroy it, you will
 * unload the whole library.  Since we would rather the library stay loaded for the rest of the application if
 * we use it, there is an extra call to load the library so that the library is guaranteed to not unload until
 * the end of the application.  As well, since each instance also calls to load the library, the library is
 * not freed until everyone is done using it.
 */

//NOTE: Make sure this class isn't being depended upon by another global variable!
//NOTE: Not thread-safe.
class SiLabsLibrary
{
  public:
                                                   //!!Is this guaranteed to never be a global variable?  If so, then we can return the static instance instead of creating a new one.
                                                                  //!!maybe return another smart pointer like shared_ptr?
   static BOOL Load(std::auto_ptr<const SiLabsLibrary>& clAutoLibrary_);  //!! Alternative to creating directly and having to worry about try statements
                                                            /* Otherwise you'd have to do this...
                                                            //Get a reference to library
                                                            auto_ptr<SiLabsLibrary> pclAutoSiLibrary(NULL);
                                                            try { pclAutoSiLibrary.reset(new SiLabsLibrary); }
                                                            catch(...) { return clList; }
                                                            const SiLabsLibrary& clSiLibrary = *pclAutoSiLibrary;
                                                            */


   SiLabsLibrary(); //throw(SiLabsError::Enum)
   virtual ~SiLabsLibrary() throw();

   // Prototypes for USB functions found in the SI dll.
   typedef SI_STATUS (WINAPI *GetNumDevices_t)(LPDWORD lpdwNumDevices);
   typedef SI_STATUS (WINAPI *GetProductString_t)(DWORD dwDeviceNum, LPVOID lpvDeviceString, DWORD dwFlags);
   typedef SI_STATUS (WINAPI *Open_t)(DWORD dwDevice, HANDLE *cyHandle);
   typedef SI_STATUS (WINAPI *Close_t)(HANDLE cyHandle);
   typedef SI_STATUS (WINAPI *Read_t)(HANDLE cyHandle, LPVOID lpBuffer, DWORD dwBytesToRead, LPDWORD lpdwBytesReturned, OVERLAPPED *o);
   typedef SI_STATUS (WINAPI *Write_t)(HANDLE cyHandle, LPVOID lpBuffer, DWORD dwBytesToWrite, LPDWORD lpdwBytesWritten, OVERLAPPED *o);
   typedef SI_STATUS (WINAPI *SetTimeouts_t)(DWORD dwReadTimeout, DWORD dwWriteTimeout);
   typedef SI_STATUS (WINAPI *GetTimeouts_t)(LPDWORD lpdwReadTimeout, LPDWORD lpdwWriteTimeout);
   typedef SI_STATUS (WINAPI *FlushBuffers_t)(HANDLE cyHandle, BYTE FlushTransmit, BYTE FlushReceive);
   typedef SI_STATUS (WINAPI *CheckRxQueue_t)(HANDLE cyHandle, LPDWORD lpdwNumBytesInQueue, LPDWORD lpdwQueueStatus);
   typedef SI_STATUS (WINAPI *SetBaudRate_t)(HANDLE cyHandle, DWORD dwBaudRate);
   typedef SI_STATUS (WINAPI *SetLineControl_t)(HANDLE cyHandle, WORD wLineControl);
   typedef SI_STATUS (WINAPI *SetFlowControl_t)(HANDLE cyHandle, BYTE bCTS_MaskCode, BYTE bRTS_MaskCode, BYTE bDTR_MaskCode, BYTE bDSR_MaskCode, BYTE bDCD_MaskCode, BOOL bFlowXonXoff);


   Open_t Open;
   GetNumDevices_t GetNumDevices;
   Close_t Close;
   Read_t Read;
   Write_t Write;
   SetTimeouts_t SetTimeouts;
   GetTimeouts_t GetTimeouts;
   FlushBuffers_t FlushBuffers;
   CheckRxQueue_t CheckRxQueue;
   SetBaudRate_t SetBaudRate;
   SetLineControl_t SetLineControl;
   SetFlowControl_t SetFlowControl;
   GetProductString_t GetProductString;


  private:

   SiLabsError::Enum LoadFunctions();
   void FreeFunctions();

   static std::auto_ptr<SiLabsLibrary> clAutoInstance;  //keeps the library loaded for the duration of the application
                                                               //NOTE: There is no control when this gets destroyed at end of program
                                                               //       but it doesn't matter because it's main purpose is to keep the library loaded
                                                               //       during the duration of the whole application.

   HMODULE hLibHandle;
   static BOOL bStaticSet;

   //!!Could dynamically make all instances and push them onto a static list to delete when we get to it
};


#endif //DSI_SILABS_LIBRARY_HPP