/*
This software is subject to the license described in the License.txt file
included with this software distribution. You may not use this file except
in compliance with this license.

Copyright (c) Dynastream Innovations Inc. 2016
All rights reserved.
*/
#if !defined(DSI_ANT_MESSAGE_PROCESSOR_HPP)
#define DSI_ANT_MESSAGE_PROCESSOR_HPP

#include "types.h"
#include "dsi_framer_ant.hpp"

//////////////////////////////////////////////////////////////////////////////////
// Public Definitions
//////////////////////////////////////////////////////////////////////////////////
typedef enum
{
   ANT_DEVICE_NOTIFICATION_RESET = 1,
   ANT_DEVICE_NOTIFICATION_SHUTDOWN = 2
} ANT_DEVICE_NOTIFICATION;

//////////////////////////////////////////////////////////////////////////////////
// Public Class Prototypes
//////////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////
// Inherit from this class to create new classes that can be
// registered to a DSIANTDevice or .NET ANT_Device to
// process ANT messages for a specific channel based on application
// level logic.
/////////////////////////////////////////////////////////////////
class DSIANTMessageProcessor
{
   protected:
      UCHAR ucChannelNumber;

   public:

      virtual ~DSIANTMessageProcessor(){}

      virtual BOOL Init(DSIFramerANT* pclANT_, UCHAR ucChannelNumber_) = 0;
      /////////////////////////////////////////////////////////////////
      // Begins to initialize the DSIANTMessageProcessor object
      // Returns TRUE if successful.  Otherwise, it returns FALSE.
      // Parameters:
      //    *pclANT_:         Pointer to a DSIFramerANT object.
      //    ucChannel_:       Channel number to use for the ANT-FS host
      // Operation:
      //    This function is used from a class managing the connection
      //    to ANT (e.g. DSIANTDevice or .NET ANT_Device), to
      //    initialize the DSIANTMessageProcessor object, assign a channel
      //    number and enable it to send ANT messages.
      //    IT IS NOT NECESSARY TO CALL THIS FUNCTION DIRECTLY FROM USER APPLICATIONS.
      /////////////////////////////////////////////////////////////////

      virtual void Close(void) = 0;
      /////////////////////////////////////////////////////////////////
      // Stops any pending actions and clean up resources
      // Operation:
      //    This function is used from a class managing the connection
      //    to ANT (e.g. DSIANTDevice or .NET ANT_Device), to
      //    clean up any resources.
      //    IT IS NOT NECESSARY TO CALL THIS FUNCTION DIRECTLY FROM USER APPLICATIONS.
      /////////////////////////////////////////////////////////////////

      virtual void ProcessMessage(ANT_MESSAGE* pstMessage_, USHORT usMesgSize_) = 0;
      /////////////////////////////////////////////////////////////////
      // Processes incoming ANT messages
      // Parameters:
      //    pstMessage_:      Pointer to an ANT message structure
      //    usMesgSize_:      ANT message size
      // Operation:
      //    This function is used from a class managing the connection
      //    to ANT (e.g. DSIANTDevice or .NET ANT_Device), and allows
      //    classes derived from DSIANTMessageProcessor to do custom processing of
      //    ANT messages
      //    IT IS NOT NECESSARY TO CALL THIS FUNCTION DIRECTLY FROM USER APPLICATIONS.
      /////////////////////////////////////////////////////////////////

      virtual void ProcessDeviceNotification(ANT_DEVICE_NOTIFICATION eCode_, void* pvParameter_) = 0;
      /////////////////////////////////////////////////////////////////
      // Processes device level notifications regarding events that
      // may impact the operation of the associated channel
      // Parameters:
      //    eCode_:           Device notification event code
      //    pvParameter_:     Pointer to struct defining specific parameters related
      //                      to the event code
      // Operation:
      //    This function is used from a class managing the connection to
      //    ANT (e.g. DSIANTDevice or .NET ANT_Device), and allows
      //    classes derived from DSIANTMessageProcessor to handle device level
      //    notifications, such as a reset or shutting down the device
      /////////////////////////////////////////////////////////////////

      virtual UCHAR GetChannelNumber(void) { return ucChannelNumber; }
      /////////////////////////////////////////////////////////////////
      // Returns the ANT channel number
      /////////////////////////////////////////////////////////////////

      virtual BOOL GetEnabled() = 0;
      /////////////////////////////////////////////////////////////////
      // Returns the current status of ANT message processing
      // Returns TRUE if this class will process ANT messages, and FALSE
      // otherwise.
      // Operation:
      //    This function is used from a class managing the connection
      //    to ANT (e.g. DSIANTDevice or .NET ANT_Device).
      //    IT IS NOT NECESSARY TO CALL THIS FUNCTION DIRECTLY FROM USER APPLICATIONS.
      /////////////////////////////////////////////////////////////////
};

#endif  // DSI_ANT_MESSAGE_PROCESSOR_HPP