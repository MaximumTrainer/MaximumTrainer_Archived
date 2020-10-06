/*
This software is subject to the license described in the License.txt file
included with this software distribution. You may not use this file except
in compliance with this license.

Copyright (c) Dynastream Innovations Inc. 2016
All rights reserved.
*/
#ifndef CM_LIBRARY_HPP
#define CM_LIBRARY_HPP

#include "types.h"

#include "DSI_CP210xManufacturingDLL_3_1.h"

#include <memory>


struct CMError
{
   enum Enum
   {
      NONE,
      NO_LIBRARY,
      NO_FUNCTION
   };

   private: CMError();
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
class CMLibrary
{
  public:
                                                                 //!!maybe return another smart pointer like shared_ptr?
   static BOOL Load(std::auto_ptr<const CMLibrary>& clAutoLibrary_);  //!! Alternative to creating directly and having to worry about try statements
                                                            /* Otherwise you'd have to do this...
                                                            //Get a reference to library
                                                            auto_ptr<CMLibrary> pclAutoLibrary(NULL);
                                                            try { pclAutoLibrary.reset(new CMLibrary); }
                                                            catch(...) { return clList; }
                                                            const CMLibrary& clCMLibrary = *pclAutoCMLibrary;
                                                            */

   CMLibrary(); //throw(CMError::Enum)
   virtual ~CMLibrary() throw();


   typedef CP210x_STATUS (WINAPI* GetDeviceProductString_t)(HANDLE cyHandle, LPVOID lpProduct, LPBYTE lpbLength, BOOL bConvertToASCII);
   typedef CP210x_STATUS (WINAPI* GetDeviceSerialNumber_t)(HANDLE cyHandle, LPVOID lpSerialNumber, LPBYTE lpbLength, BOOL bConvertToASCII);
   typedef CP210x_STATUS (WINAPI* GetDeviceVid_t)(HANDLE cyHandle, LPWORD lpwVid);
   typedef CP210x_STATUS (WINAPI* GetDevicePid_t)(HANDLE cyHandle, LPWORD lpwPid);
   typedef CP210x_STATUS (WINAPI* Reset_t)(HANDLE cyHandle);


   GetDeviceProductString_t GetDeviceProductString;
   GetDeviceSerialNumber_t GetDeviceSerialNumber;
   GetDeviceVid_t GetDeviceVid;
   GetDevicePid_t GetDevicePid;
   Reset_t ResetDevice;


  private:

   CMError::Enum LoadFunctions();
   void FreeFunctions();

   static std::auto_ptr<CMLibrary> clAutoInstance;  //keeps the library loaded for the duration of the application
                                                               //NOTE: There is no control when this gets destroyed at end of program
                                                               //       but it doesn't matter because it's main purpose is to keep the library loaded
                                                               //       during the duration of the whole application.

   HMODULE hLibHandle;
   static BOOL bStaticSet;

   //!!Could dynamically make all instances and push them onto a static list to delete when we get to it
};



#endif //CM_LIBRARY_HPP