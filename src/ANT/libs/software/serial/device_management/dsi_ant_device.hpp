/*
This software is subject to the license described in the License.txt file
included with this software distribution. You may not use this file except
in compliance with this license.

Copyright (c) Dynastream Innovations Inc. 2016
All rights reserved.
*/
#if !defined(DSI_ANT_DEVICE_HPP)
#define DSI_ANT_DEVICE_HPP

#include "types.h"
#include "dsi_thread.h"
#include "dsi_framer_ant.hpp"
#include "dsi_serial_generic.hpp"
#include "dsi_debug.hpp"

#include "antmessage.h"

#include "dsi_ant_message_processor.hpp"
#include "dsi_response_queue.hpp"


#define MAX_ANT_CHANNELS  8

//////////////////////////////////////////////////////////////////////////////////
// Public Definitions
//////////////////////////////////////////////////////////////////////////////////

//----none----

//////////////////////////////////////////////////////////////////////////////////
// Public Class Prototypes
//////////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////
// This class handles the connection to the USB stick, polls for
// ANT messages and dispatches them to appropriate handlers (e.g.
// ANTFSClientChannel or ANTFSHostChannel) for processing
//
// IMPORTANT: this class is under development and subject to change.
// It is not recommended to use this class for custom applications.
/////////////////////////////////////////////////////////////////
class DSIANTDevice
{
   private:

      //////////////////////////////////////////////////////////////////////////////////
      // Private Definitions
      //////////////////////////////////////////////////////////////////////////////////

      //----none----

      //////////////////////////////////////////////////////////////////////////////////
      // Private Variables
      //////////////////////////////////////////////////////////////////////////////////

      volatile BOOL bOpened;

      DSI_THREAD_ID hReceiveThread;                         // Handle for the receive thread.
      DSI_MUTEX stMutexChannelListAccess;                  // Mutex used with the channel list
      DSI_CONDITION_VAR stCondReceiveThreadExit;            // Event to signal the receive thread has ended.

      volatile BOOL bKillThread;
      volatile BOOL bReceiveThreadRunning;
      volatile BOOL bCancel;

      ULONG ulUSBSerialNumber;

      DSIANTMessageProcessor* apclChannelList[MAX_ANT_CHANNELS];

      DSISerialGeneric *pclSerialObject;
      DSIFramerANT *pclANT;

   protected:
      DSI_MUTEX stMutexCriticalSection;                     // Mutex used with the wait condition

      //////////////////////////////////////////////////////////////////////////////////
      // Private Function Prototypes
      //////////////////////////////////////////////////////////////////////////////////

      void DisconnectFromDevice();

      void ReceiveThread(void);
      static DSI_THREAD_RETURN ReceiveThreadStart(void *pvParameter_);
      BOOL ConnectToDevice(void);

   protected:
      virtual void HandleSerialError();   //<-Note needs 'virtual' because it is called internally but needs to be overridden by subclasses


   public:

      DSIANTDevice();
      virtual ~DSIANTDevice();


      /////////////////////////////////////////////////////////////////
      // Opens the first available ANT device.
      // Returns TRUE if successful.  Otherwise, it returns FALSE.
      //
      // This version of Init will only work for the ANT USB stick with
      // USB device descriptor "ANT USB Stick"
      /////////////////////////////////////////////////////////////////
      virtual BOOL Open();

      /////////////////////////////////////////////////////////////////
      // Opens the device with the given device number and baudrate.
      // Returns TRUE if successful.  Otherwise, it returns FALSE.
      // Parameters:
      //    ucUSBDeviceNum_:  USB port number
      //    usBaudRate_:      Serial baud rate
      /////////////////////////////////////////////////////////////////
      virtual BOOL Open(UCHAR ucUSBDeviceNum_, USHORT usBaudRate_);


      /////////////////////////////////////////////////////////////////
      // Stops any pending actions, closes device down and cleans
      // up any resources being used by the library.
      /////////////////////////////////////////////////////////////////
      virtual void Close(void);

      /////////////////////////////////////////////////////////////////
      // Adds a message processor, to process messages for the specified
      // channel number.
      // Parameters:
      //    ucChannelNumber_: ANT channel number
      //    *pclChannel_:    A pointer to a DSIANTMessageProcessor.
      // Returns TRUE if successful.  Otherwise, it returns FALSE.
      // Operation:
      //    After adding attaching a message processor to a channel number,
      //    messages for the corresponding channel, are dispatched to it for
      //    processing.   Only one message processor can be added per ANT channel.
      /////////////////////////////////////////////////////////////////
      BOOL AddMessageProcessor(UCHAR ucChannelNumber_, DSIANTMessageProcessor* pclChannel_);

      /////////////////////////////////////////////////////////////////
      // Unregisters a message processor, to stop processing ANT messages.
      // Parameters:
      //    *pclChannel_:    A pointer to a DSIANTMessageProcessor.
      // Returns TRUE if successful.  Otherwise, it returns FALSE.
      // Operation:
      //    Remove instances of classes derived of DSIANTMessageProcessor
      //    from the list to stop processing messages for that
      //    channel number
      /////////////////////////////////////////////////////////////////
      BOOL RemoveMessageProcessor(DSIANTMessageProcessor* pclChannel_);

      /////////////////////////////////////////////////////////////////
      // Remove all managed channels from the list
      /////////////////////////////////////////////////////////////////
      void ClearManagedChannelList(void);

      /////////////////////////////////////////////////////////////////
      // Returns the serial number of the connected USB device
      /////////////////////////////////////////////////////////////////
      ULONG GetSerialNumber();   //TODO return false when not connected

      #if defined(DEBUG_FILE)
         /////////////////////////////////////////////////////////////////
         // Enables debug files
         // Parameters:
         //    bDebugOn_:     Enable/disable debug logs.
         //    *pcDirectory_: A string to use as the path for storing
         //                   debug logs. Set to NULL to use the working
         //                   directory as the default path.
         /////////////////////////////////////////////////////////////////
         void SetDebug(BOOL bDebugOn_, const char *pcDirectory_ = (const char*) NULL);
      #endif

};

#endif  // DSI_ANT_DEVICE_HPP