/*
This software is subject to the license described in the License.txt file
included with this software distribution. You may not use this file except
in compliance with this license.

Copyright (c) Dynastream Innovations Inc. 2016
All rights reserved.
*/
#ifndef DSI_LIBUSB_LIBRARY_HPP
#define DSI_LIBUSB_LIBRARY_HPP

#include "types.h"

#include "usb.h"

#include <memory>

#include <windows.h>



struct LibusbError
{
   enum Enum
   {
      NONE,
      NO_LIBRARY,
      NO_FUNCTION
   };

   private: LibusbError();
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
class LibusbLibrary
{
  public:
                                                                  //!!maybe return another smart pointer like shared_ptr?
   static BOOL Load(std::auto_ptr<const LibusbLibrary>& clAutoLibrary_);  //!! Alternative to creating directly and having to worry about try statements
                                                            /* Otherwise you'd have to do this...
                                                            //Get a reference to library
                                                            auto_ptr<LibusbLibrary> pclAutoSiLibrary(NULL);
                                                            try { pclAutoSiLibrary.reset(new LibusbLibrary); }
                                                            catch(...) { return clList; }
                                                            const LibusbLibrary& clSiLibrary = *pclAutoSiLibrary;
                                                            */

   //could do...
   //static const LibusbLibrary& Load(BOOL& bSuccess_);

   LibusbLibrary(); //throw(LibusbError::Enum)
   virtual ~LibusbLibrary() throw();


   //Prototypes for functions found in the LibUSB dll.
   typedef int      (*ClaimInterface_t)(void*, int);
   typedef usb_bus* (*GetBusses_t)(void);
   typedef int      (*ClearHalt_t)(usb_dev_handle*, unsigned int);
   typedef int      (*GetStringSimple_t)(usb_dev_handle*,int,char*,size_t);
   typedef int      (*Close_t)(usb_dev_handle*);
   typedef int      (*Reset_t)(usb_dev_handle*);
   typedef void     (*Init_t)(void);
   typedef int      (*FindBusses_t)(void);
   typedef int      (*FindDevices_t)(void);
   typedef usb_dev_handle* (*Open_t)(struct usb_device*);
   typedef int      (*ReleaseInterface_t)(usb_dev_handle*, int);
   typedef int      (*InterruptWrite_t)(usb_dev_handle*, int, char*, int, int);
   typedef int      (*BulkRead_t)(usb_dev_handle*, int, char*, int, int);
   typedef struct usb_device* (*Device_t)(usb_dev_handle*);
   typedef int      (*SetConfiguration_t)(usb_dev_handle*, int);
   typedef void     (*SetDebug_t)(int);
   //Libusb-win32 only asynch functions
   typedef int    (*BulkSetupAsync_t)(usb_dev_handle *dev, void **context, unsigned char ep);
   typedef int    (*SubmitAsync_t)(void *context, char *bytes, int size);
   typedef int    (*ReapAsyncNocancel_t)(void *context, int timeout);
   typedef int    (*CancelAsync_t)(void *context);
   typedef int    (*FreeAsync_t)(void ** context);

   ClaimInterface_t ClaimInterface;
   GetBusses_t GetBusses;
   ClearHalt_t ClearHalt;
   GetStringSimple_t GetStringSimple;
   Close_t Close;
   Reset_t Reset;
   Init_t Init;
   FindBusses_t FindBusses;
   FindDevices_t FindDevices;
   Open_t Open;
   ReleaseInterface_t ReleaseInterface;
   InterruptWrite_t InterruptWrite;
   BulkRead_t BulkRead;
   Device_t Device;
   SetConfiguration_t SetConfiguration;
   SetDebug_t SetDebug;
   //Libusb-win32 only asynch functions
   BulkSetupAsync_t BulkSetupAsync;
   SubmitAsync_t SubmitAsync;
   ReapAsyncNocancel_t ReapAsyncNocancel;
   CancelAsync_t CancelAsync;
   FreeAsync_t FreeAsync;

  private:

   LibusbError::Enum LoadFunctions();
   void FreeFunctions();

   static std::auto_ptr<LibusbLibrary> clAutoInstance;  //keeps the library loaded for the duration of the application
                                                               //NOTE: There is no control when this gets destroyed at end of program
                                                               //       but it doesn't matter because it's main purpose is to keep the library loaded
                                                               //       during the duration of the whole application.

   HMODULE hLibHandle;
   static BOOL bStaticSet;

   //!!Could dynamically make all instances and push them onto a static list to delete when we get to it
};


#endif //DSI_LIBUSB_LIBRARY_HPP