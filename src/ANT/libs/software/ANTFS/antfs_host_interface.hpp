/*
This software is subject to the license described in the License.txt file
included with this software distribution. You may not use this file except
in compliance with this license.

Copyright (c) Dynastream Innovations Inc. 2016
All rights reserved.
*/
#if !defined(ANTFS_HOST_INTERFACE_H)
#define ANTFS_HOST_INTERFACE_H

#include "types.h"
#include "dsi_thread.h"

#include "antfs_interface.h"
#include "antfsmessage.h"

//////////////////////////////////////////////////////////////////////////////////
// Public Definitions
//////////////////////////////////////////////////////////////////////////////////
#define ANTFS_STATUS1_RESERVED7_BIT       ((UCHAR) 0x80)
#define ANTFS_STATUS1_DATA_AVAILABLE_BIT  ((UCHAR) 0x20)
#define ANTFS_STATUS1_UPLOAD_ENABLED_BIT  ((UCHAR) 0x10)
#define ANTFS_STATUS1_PAIRING_ENABLED_BIT ((UCHAR) 0x08)
#define ANTFS_STATUS1_BEACON2_BIT         ((UCHAR) 0x04)
#define ANTFS_STATUS1_BEACON1_BIT         ((UCHAR) 0x02)
#define ANTFS_STATUS1_BEACON0_BIT         ((UCHAR) 0x01)

#define ANTFS_STATUS2_RESERVED7_BIT       ((UCHAR) 0x80)
#define ANTFS_STATUS2_RESERVED6_BIT       ((UCHAR) 0x40)
#define ANTFS_STATUS2_RESERVED5_BIT       ((UCHAR) 0x20)
#define ANTFS_STATUS2_RESERVED4_BIT       ((UCHAR) 0x10)
#define ANTFS_STATUS2_CLIENT_STATE3_BIT   ((UCHAR) 0x08)
#define ANTFS_STATUS2_CLIENT_STATE2_BIT   ((UCHAR) 0x04)
#define ANTFS_STATUS2_CLIENT_STATE1_BIT   ((UCHAR) 0x02)
#define ANTFS_STATUS2_CLIENT_STATE0_BIT   ((UCHAR) 0x01)

#define ANTFS_STATUS1_DEFAULT_SEARCH_MASK ((UCHAR) 0x38)
#define ANTFS_STATUS2_DEFAULT_SEARCH_MASK ((UCHAR) 0x00)

// ANT-FS Host Responses
typedef enum
{
   ANTFS_HOST_RESPONSE_NONE = 0,
   ANTFS_HOST_RESPONSE_INIT_PASS,
   ANTFS_HOST_RESPONSE_SERIAL_FAIL,
   ANTFS_HOST_RESPONSE_REQUEST_SESSION_FAIL,
   ANTFS_HOST_RESPONSE_CONNECT_PASS,
   ANTFS_HOST_RESPONSE_DISCONNECT_PASS,
   ANTFS_HOST_RESPONSE_CONNECTION_LOST,
   ANTFS_HOST_RESPONSE_AUTHENTICATE_NA,
   ANTFS_HOST_RESPONSE_AUTHENTICATE_PASS,
   ANTFS_HOST_RESPONSE_AUTHENTICATE_REJECT,
   ANTFS_HOST_RESPONSE_AUTHENTICATE_FAIL,
   ANTFS_HOST_RESPONSE_DOWNLOAD_PASS,
   ANTFS_HOST_RESPONSE_DOWNLOAD_REJECT,
   ANTFS_HOST_RESPONSE_DOWNLOAD_INVALID_INDEX,
   ANTFS_HOST_RESPONSE_DOWNLOAD_FILE_NOT_READABLE,
   ANTFS_HOST_RESPONSE_DOWNLOAD_NOT_READY,
   ANTFS_HOST_RESPONSE_DOWNLOAD_CRC_REJECTED,
   ANTFS_HOST_RESPONSE_DOWNLOAD_FAIL,
   ANTFS_HOST_RESPONSE_UPLOAD_PASS,
   ANTFS_HOST_RESPONSE_UPLOAD_REJECT,
   ANTFS_HOST_RESPONSE_UPLOAD_INVALID_INDEX,
   ANTFS_HOST_RESPONSE_UPLOAD_FILE_NOT_WRITEABLE,
   ANTFS_HOST_RESPONSE_UPLOAD_INSUFFICIENT_SPACE,
   ANTFS_HOST_RESPONSE_UPLOAD_FAIL,
   ANTFS_HOST_RESPONSE_ERASE_PASS,
   ANTFS_HOST_RESPONSE_ERASE_FAIL,
   ANTFS_HOST_RESPONSE_MANUAL_TRANSFER_PASS,
   ANTFS_HOST_RESPONSE_MANUAL_TRANSFER_TRANSMIT_FAIL,
   ANTFS_HOST_RESPONSE_MANUAL_TRANSFER_RESPONSE_FAIL,
   ANTFS_HOST_RESPONSE_CANCEL_DONE
} ANTFS_HOST_RESPONSE;

// ANT-FS Host States
typedef enum
{
   ANTFS_HOST_STATE_OFF = 0,             // Object created, but not initialized
   ANTFS_HOST_STATE_IDLE,                // Object initialized
   ANTFS_HOST_STATE_DISCONNECTING,
   ANTFS_HOST_STATE_REQUESTING_SESSION,
   ANTFS_HOST_STATE_SEARCHING,           // LINK
   ANTFS_HOST_STATE_CONNECTED,           // AUTH
   ANTFS_HOST_STATE_AUTHENTICATING,
   ANTFS_HOST_STATE_TRANSPORT,
   ANTFS_HOST_STATE_DOWNLOADING,
   ANTFS_HOST_STATE_UPLOADING,
   ANTFS_HOST_STATE_ERASING,
   ANTFS_HOST_STATE_SENDING,
   ANTFS_HOST_STATE_RECEIVING
} ANTFS_HOST_STATE;

typedef struct
{
   ULONG ulDeviceID;
   USHORT usManufacturerID;
   USHORT usDeviceType;
   UCHAR ucAuthenticationType;
   UCHAR ucStatusByte1;
   UCHAR ucStatusByte2;
} ANTFS_DEVICE_PARAMETERS;


//////////////////////////////////////////////////////////////////////////////////
// Public Function Prototypes
//////////////////////////////////////////////////////////////////////////////////

// TODO: Is this class used for anything?  Do we need it?
class ANTFSHostInterface
{
   public:
      virtual ~ANTFSHostInterface(){}

      virtual void Close(void) = 0;
      /////////////////////////////////////////////////////////////////
      // Stops any pending actions, closes all resources and cleans
      // up any dynamic memory being used by the library.
      /////////////////////////////////////////////////////////////////

      virtual ANTFS_HOST_STATE GetStatus(void) = 0;
      /////////////////////////////////////////////////////////////////
      // Returns the current library status.
      /////////////////////////////////////////////////////////////////

      virtual ANTFS_HOST_RESPONSE WaitForResponse(ULONG ulMilliseconds_) = 0;
      /////////////////////////////////////////////////////////////////
      // Wait for a response from the ANTFSHost object.
      // Parameters:
      //    ulMilliseconds_:  Set this value to the minimum time to
      //                      wait before returning.  If the value is
      //                      0, the function will return immediately.
      //                      If the value is DSI_THREAD_INFINITE, the
      //                      function will not time out.
      // If one or more responses are pending before the timeout
      // expires the function will return the first response that
      // occured.  If no response is pending at the time the timeout
      // expires, ANTFS_HOST_RESPONSE_NONE is returned.
      /////////////////////////////////////////////////////////////////

      virtual void Cancel(void) = 0;
      /////////////////////////////////////////////////////////////////
      // Cancels any pending actions and returns the library to the
      // appropriate ANTFS layer if possible.  ie if the library was
      // executing a download command in the transport layer, the
      // library would be returned to ANTFS_HOST_STATE_TRANSPORT after
      // execution of this function.
      // A response of ANTFS_HOST_RESPONSE_CANCEL_DONE will be sent
      // back to the application when the cancel operation is
      // complete.
      /////////////////////////////////////////////////////////////////

      virtual USHORT AddSearchDevice(ANTFS_DEVICE_PARAMETERS *pstDeviceSearchMask_, ANTFS_DEVICE_PARAMETERS *pstDeviceParameters_) = 0;
      /////////////////////////////////////////////////////////////////
      // Adds a set of parameters for which to search to the internal
      // search device list.
      // Parameters:
      //    *pstDeviceSearchMask_:  A pointer to an
      //                      ANTFS_DEVICE_PARAMETERS structure.  Set a
      //                      member to zero (0) to wildcard search for
      //                      it.  Otherwise, set the bits that you want
      //                      to be matched to 1 in each member.  Members
      //                      that are integer values will be treated the
      //                      same as bit fields for the purposes for the mask.
      //    *pstDeviceParameters_:  A pointer to an
      //                      ANTFS_DEVICE_PARAMETERS structure.  Set
      //                      the member to the desired search value.
      //                      A member in this structure is ignored if
      //                      the associated member in the
      //                      *pstDeviceSearchMask_ parameter is set
      //                      to zero (0) for wildcard.
      // Returns a handle the the search device entry.  If the return
      // value is NULL, the function failed adding the device entry.
      // This means that the device list is already full.
      // Operation:
      // Note that the default search masks should normally be applied
      // to the ucStatusByte1 and ucStatusByte2 members of the
      // *pstDeviceSearchMask_ parameter.  Eg;
      //    pstMyDeviceSearchMask->ucStatusByte1 =
      //       ANTFS_STATUS1_DEFAULT_SEARCH_MASK & ucMyStatus1Criteria;
      // Setting bits outside the masks, especially reserved bits, may
      // lead to undesired behaviour.
      /////////////////////////////////////////////////////////////////

      virtual void RemoveSearchDevice(USHORT usHandle_) = 0;
      /////////////////////////////////////////////////////////////////
      // Removes a device entry from the internal search list.
      // Parameters:
      //    usHandle_:        Handle to the device entry to be removed
      //                      from the list.
      /////////////////////////////////////////////////////////////////

      virtual void ClearSearchDeviceList(void) = 0;
      /////////////////////////////////////////////////////////////////
      // Clears the internal search device list.
      /////////////////////////////////////////////////////////////////

      virtual ANTFS_RETURN SearchForDevice(UCHAR ucSearchRadioFrequency_ = ANTFS_RF_FREQ, UCHAR ucConnectRadioFrequency_ = ANTFS_DEFAULT_TRANSPORT_FREQ, USHORT usRadioChannelID_ = 0, BOOL bUseRequestPage_ = FALSE) = 0;
      /////////////////////////////////////////////////////////////////
      // Begins a search for ANTFS remote devices.
      // Parameters:
      //    ucSearchRadioFrequency_:   This specifies the frequency on
      //                      which to search for devices.  This
      //                      frequency is calculated as
      //                      (ucSearchRadioFrequency_ * 1 MHz +
      //                      2400 MHz).  MAX_UCHAR (0xFF) is reserved.
      //    ucConnectRadioFrequency_:   This specifies the frequency
      //                      on which the connection communication
      //                      will occur.  This frequency is calculated
      //                      as (ucConnectRadioFrequency_ * 1 MHz +
      //                      2400 MHz).  If ucConnectRadioFrequency_
      //                      is set to ANTFS_AUTO_FREQUENCY_SELECTION
      //                      (0xFF), then the library will use an
      //                      adaptive frequency-hopping scheme.
      //    usRadioChannelID_: This specifies the channel ID for the
      //                      the ANT channel search. Set to zero to
      //                      wildcard.
      //    bUseRequestPage_: Specifies whether the search will include
      //                      ANT-FS broadcast devices, using a request
      //                      page to begin an ANT-FS session
      // Returns ANTFS_RETURN_PASS if successful.  Otherwise, it returns
      // ANTFS_RETURN_FAIL if the library is in the wrong state, or
      // ANTFS_RETURN_BUSY if the library is busy with another request.
      // Operation:
      // Once this function is called successfully, the application
      // must wait for the response from the ANTFSHost object.
      // Possible responses are:
      //    ANTFS_HOST_RESPONSE_CONNECT_PASS
      //    ANTFS__HOST_RESPONSE_SERIAL_FAIL
      // The library will continue to search for devices until a device
      // is found, the Cancel() function is called, an error occurs, or
      // the library is closed.
      /////////////////////////////////////////////////////////////////

      virtual BOOL GetFoundDeviceParameters(ANTFS_DEVICE_PARAMETERS *pstDeviceParameters_, UCHAR *aucFriendlyName_, UCHAR *pucBufferSize_) = 0;
      /////////////////////////////////////////////////////////////////
      // Copies the parameters of the most recently found device to the
      // supplied parameter.
      // Parameters:
      //    *ptsDeviceParameters_:  A pointer to a structure that will
      //                      receive a copy of the device parameters.
      //    *aucFriendlyName_: A pointer to a buffer where the remote
      //                      device friendly name will be copied.
      //    *pucBufferSize_:  Pointer to a UCHAR variable that should contain the
      //                      maximum size of the buffer pointed to by aucFriendlyName_.
      //                      After the function returns, the UCHAR variable
      //                      will be set to reflect the size of friendly name string
      //                      that has been copied to aucFriendlyName_.
      // Returns TRUE if successful.  Otherwise, it returns FALSE.
      /////////////////////////////////////////////////////////////////

      virtual BOOL GetUploadStatus(ULONG *pulByteProgress_, ULONG *pulTotalLength_) = 0;
      /////////////////////////////////////////////////////////////////
      // Gets the transfer progress of a pending or a complete
      // upload.
      // Parameters:
      //    *pulByteProgress_:   Pointer to a ULONG that will receive
      //                      the current byte progress of a pending or
      //                      complete upload.
      //    *pulTotalLength_: Pointer to a ULONG that will receive the
      //                      total length of the file being uploaded.
      // Returns TRUE if successful.  Otherwise, it returns FALSE.
      // Operation:
      // A data upload occurs when information is requested to be sent to
      // a remote device.  This function may be called at any point
      // during an upload as a progress indicator.  After the upload
      // is complete, this information is valid until another request
      // for a data transfer is made.
      /////////////////////////////////////////////////////////////////

      virtual BOOL GetDownloadStatus(ULONG *pulByteProgress_, ULONG *pulTotalLength_) = 0;
      /////////////////////////////////////////////////////////////////
      // Gets the transfer progress of a pending or a complete
      // download.
      // Parameters:
      //    *pulByteProgress_:   Pointer to a ULONG that will receive
      //                      the current byte progress of a pending or
      //                      complete download.
      //    *pulTotalLength_: Pointer to a ULONG that will receive the
      //                      total expected length of the download.
      // Returns TRUE if successful.  Otherwise, it returns FALSE.
      // Operation:
      // A data download occurs when information is requested from a
      // remote device.  This function may be called at any point
      // during a download as a progress indicator.  After the transfer
      // is complete, this information is valid until another request
      // for a data transfer is made.
      /////////////////////////////////////////////////////////////////

      virtual BOOL GetTransferData(ULONG *pulDataSize_ , void *pvData_ = NULL) = 0;
      /////////////////////////////////////////////////////////////////
      // Gets the received data from a transfer (download/manual transfer).
      //
      // Parameters:
      //    *ulDataSize_     :   Pointer to a ULONG that will receive
      //                      the size of the data available in bytes.
      //    *pvData_        : Pointer to a buffer where the received data
      //                      will be copied.  NULL can be passed to this
      //                      parameter so that the size can be retried
      //                      without copying any data.  The application
      //                      can then call this function again to after it
      //                      has allocated a buffer of sufficient size to
      //                      handle the data.
      // Returns TRUE if successful.  Otherwise, it returns FALSE.
      /////////////////////////////////////////////////////////////////

     virtual BOOL RecoverTransferData(ULONG *pulDataSize_ , void *pvData_ = NULL) = 0;
      /////////////////////////////////////////////////////////////////
      // Gets the partially received data from a failed transfer (download/manual transfer).
      //
      // Parameters:
      //    *ulDataSize_     :   Pointer to a ULONG that will receive
      //                      the size of the data available in bytes.
      //    *pvData_        : Pointer to a buffer where the received data
      //                      will be copied.  NULL can be passed to this
      //                      parameter so that the size can be retried
      //                      without copying any data.  The application
      //                      can then call this function again to after it
      //                      has allocated a buffer of sufficient size to
      //                      handle the data.
      // Returns TRUE if successful.  Otherwise, it returns FALSE.
      /////////////////////////////////////////////////////////////////

      virtual ANTFS_RETURN Authenticate(UCHAR ucAuthenticationType_, UCHAR *pucAuthenticationString_, UCHAR ucLength_, UCHAR *aucResponseBuffer_, UCHAR *pucResponseBufferSize_, ULONG ulResponseTimeout_) = 0;
      /////////////////////////////////////////////////////////////////
      // Request to pair with the connected remote device.
      // Parameters:
      //    ucAuthenticationType_:  The type of authentication to
      //                      execute on the remote device.
      //    *pucAuthenticationString_: A string that may be used in
      //                      conjunction with the particular
      //                      authentication type in use (e.g. a
      //                      password or a friendly name.
      //    ucLength_:        The length of the authentication string,
      //                      including any '\n' terminator.  This
      //                      parameter may be set to zero if an
      //                      authentication string is not required.
      //    *pucResponseBuffer_: Pointer to the buffer where additional
      //                      data from the response will be saved.  This will
      //                      include data such as passwords and friendly names.
      //                      This memory must be allocated by the application.
      //    *pucResponseBufferSize_: Pointer to UCHAR varible that contains the
      //                      size of the buffer pointed to by pucResponseBuffer_.
      //                      After the response has be recived, the UCHAR variable
      //                      will be set to reflect the size of the new data received
      //                      in pucResponseBuffer_.
      //    ulResponseTimeout_: Number of miliseconds to wait for a response after
      //                      the authenticate command is sent.
      // Returns ANTFS_RETURN_PASS if successful.  Otherwise, it returns
      // ANTFS_RETURN_FAIL if the library is in the wrong state, or
      // ANTFS_RETURN_BUSY if the library is busy with another request.
      // Operation:
      // Once the request is posted, the application must wait for the
      // response from the ANTFSHost object.  Possible responses are:
      //    ANTFS_HOST_RESPONSE_AUTHENTICATE_NA
      //    ANTFS_HOST_RESPONSE_AUTHENTICATE_PASS
      //    ANTFS_HOST_RESPONSE_AUTHENTICATE_REJECT
      //    ANTFS_HOST_RESPONSE_AUTHENTICATE_FAIL
      //    ANTFS_HOST_RESPONSE_CONNECTION_LOST
      //    ANTFS_HOST_RESPONSE_SERIAL_FAIL
      // Upon receiving the ANTFS_HOST_RESPONSE_AUTHENTICATE_PASS, an
      // authentication string provided by the remoted device may be
      // available in the response buffer.  This depends on the
      // authentication type used.  The transport
      // layer connection is also only established after receiving
      // ANTFS_HOST_RESPONSE_AUTHENTICATE_PASS .
      /////////////////////////////////////////////////////////////////

      virtual ANTFS_RETURN Disconnect(USHORT usBlackoutTime_, UCHAR ucDisconnectType_ = 0, UCHAR ucTimeDuration_ = 0, UCHAR ucAppSpecificDuration_ = 0) = 0;
      /////////////////////////////////////////////////////////////////
      // Disconnect from a remote device.  Optionally put that device
      // on a blackout list for a period of time. The application can
      // also request the remote device to become undiscoverable for a
      // specified time/application specific duration.
      // Parameters:
      //    usBlackoutTime_:  The number of seconds the device ID
      //                      should remain on the blackout list. If
      //                      set to zero (0), then the device will
      //                      not be put on the blackout list.  If set
      //                      to MAX_USHORT (0xFFFF), the device will
      //                      remain on the blackout list until
      //                      explicitly removed, or until the blackout
      //                      list is reset.
      //    ucDisconnectType_:   Specifies whether the client should
      //                      return to LINK state or broadcast mode
      //                      (ANT-FS broadcast)
      //    ucTimeDuration_:  Time, in 30 seconds increments, the client
      //                      device will remain undiscoverable after
      //                      disconnect has been requested. Set to 0 to
      //                      disable.
      //    ucAppSpecificDuration_: Application specific duration the client
      //                      shall remain undiscoverable after disconnection.
      //                      This field is left to the developer, or defined
      //                      in an ANT+ Device Profile. Set to 0 to disable.
      // Returns ANTFS_RETURN_PASS if successful.  Otherwise, it returns
      // ANTFS_RETURN_FAIL if the library is in the wrong state, or
      // ANTFS_RETURN_BUSY if the library is busy with another request.
      // Operation:
      // Once the request is posted, the application must wait for the
      // response from the ANTFSHost object.  Possible responses are:
      //    ANTFS_HOST_RESPONSE_DISCONNECT_PASS
      //    ANTFS_HOST_RESPONSE_CONNECTION_LOST
      //    ANTFS_HOST_RESPONSE_SERIAL_FAIL
      // The remote device will not show up in any search results for
      // the duration specified in usBlackoutTime_.  This allows the
      // host to more easily find other devices in crowded
      // environments. The host can also request the remote device to
      // become undiscoverable, to more easily find other devices, however
      // not all client devices might implement this feature.  Not all
      // client devices support a broadcast mode.
      /////////////////////////////////////////////////////////////////

      virtual BOOL Blackout(ULONG ulDeviceID_, USHORT usManufacturerID_, USHORT usDeviceType_, USHORT usBlackoutTime_) = 0;
      /////////////////////////////////////////////////////////////////
      // Put the device on a blackout list for a period of time.
      // Parameters:
      //    ulDeviceID_:      The device ID of a specific device.
      //    usManufacturerID_:   The specific manufacturer ID.
      //    usDeviceType_:    The specific device type.
      //    usBlackoutTime_:  The number of seconds the device ID
      //                      should remain on the blackout list. If
      //                      set to zero (0), then the device will
      //                      not be put on the blackout list.  If set
      //                      to MAX_USHORT (0xFFFF), the device will
      //                      remain on the blackout list until
      //                      explicitly removed, or until the blackout
      //                      list is reset.
      // Returns TRUE if successful.  Otherwise, it returns FALSE.
      // A wildcard parameter (0) is not allowed in any of the device
      // ID parameters and will result in returning FALSE.
      // Operation:
      // The remote device will not show up in any search results for
      // the duration specified in usBlackoutTime_.  This allows the
      // host to more easily find other devices in crowded
      // environments.
      /////////////////////////////////////////////////////////////////

      virtual BOOL RemoveBlackout(ULONG ulDeviceID_, USHORT usManufacturerID_, USHORT usDeviceType_) = 0;
      /////////////////////////////////////////////////////////////////
      // Remove the device from the blackout list.
      // Parameters:
      //    ulDeviceID_:      The device ID of a specific device.
      //    usManufacturerID_:   The specific manufacturer ID.
      //    usDeviceType_:    The specific device type.
      // Returns TRUE if successful.  Returns FALSE if the device is
      // not found in the blackout list.  A wildcard parameter (0) is
      // not allowed in any of the device ID parameters and will result
      // in returning FALSE.
      /////////////////////////////////////////////////////////////////

      virtual void ClearBlackoutList(void) = 0;
      /////////////////////////////////////////////////////////////////
      // Clears the blackout list.
      /////////////////////////////////////////////////////////////////

      virtual ANTFS_RETURN Download(USHORT usFileIndex_, ULONG ulDataOffset_, ULONG ulMaxDataLength_, ULONG ulMaxBlockSize_ = 0) = 0;
      /////////////////////////////////////////////////////////////////
      // Request a download of a file from the authenticated device.
      // Parameters:
      //    usFileIndex_:     The file number to be downloaded. Some
      //                      file numbers are reserved for special
      //                      purposes, such as the device directory
      //                      (0).  Consult the ANT_FS specification
      //                      and any docs for specific device types.
      //    ulDataOffset_:    The byte offset from where to begin
      //                      transferring the data.
      //    ulMaxDataLength_: ULONG varible that contains the maximum
      //                      number of bytes to download.  Set to 0 if
      //                      if the host does not wish to limit the
      //                      size of the download.
      //    ulMaxBlockSize_:  Maximum number of bytes that the host
      //                      wishes to download in a single block.
      //                      Set to zero to disable.
      // Returns ANTFS_RETURN_PASS if successful.  Otherwise, it returns
      // ANTFS_RETURN_FAIL if the library is in the wrong state, or
      // ANTFS_RETURN_BUSY if the library is busy with another request.
      // Operation:
      // Once the request is posted, the application must wait for the
      // response from the ANTFSHost object.  Possible responses are:
      //    ANTFS_HOST_RESPONSE_DOWNLOAD_PASS
      //    ANTFS_HOST_RESPONSE_DOWNLOAD_REJECT
      //    ANTFS_HOST_RESPONSE_DOWNLOAD_FAIL
      //    ANTFS_HOST_RESPONSE_CONNECTION_LOST
      //    ANTFS_HOST_RESPONSE_SERIAL_FAIL
      // Upon receiving ANTFS_HOST_RESPONSE_DOWNLOAD_PASS the downloaed data
      // will be available in the transfer buffer.  See GetTransferData().
      /////////////////////////////////////////////////////////////////

      virtual ANTFS_RETURN Upload(USHORT usFileIndex_, ULONG ulDataOffset_, ULONG ulDataLength_, void *pvData_, BOOL bForceOffset_ = TRUE, ULONG ulMaxBlockSize_ = 0) = 0;
      /////////////////////////////////////////////////////////////////
      // Request an upload of a file to the authenticated device.
      // Parameters:
      //    usFileIndex_:     The file number to be uploaded.  Some
      //                      file numbers are reserved for special
      //                      purposes, such as the device directory
      //                      (0).  Consult the ANT_FS specification
      //                      and any docs for specific device types.
      //    ulDataOffset_:    The byte offset from where to begin
      //                      transferring the data.
      //    ulDataLength_:    The byte length of data to be uploaded
      //                      to the remote device.
      //                      Return value:
      //    *pvData_:         Pointer to the location where the data
      //                      to be uploaded is stored.
      //    bForceOffset_:    Set to TRUE (default) to enforce the
      //                      provided byte data offset.  Set to FALSE
      //                      to continue a transfer, indicating that
      //                      the host will continue the upload at the
      //                      last data offset specified by the client
      //                      in the Upload Response.
      //    ulMaxBlockSize_:  Maximum block size that the host can send
      //                      in a single block of data.  Set to zero
      //                      to disable
      // Returns ANTFS_RETURN_PASS if successful.  Otherwise, it returns
      // ANTFS_RETURN_FAIL if the library is in the wrong state, or
      // ANTFS_RETURN_BUSY if the library is busy with another request.
      // Operation:
      // Once the request is posted, the application must wait for the
      // response from the ANTFSHost object.  Possible responses are:
      //    ANTFS_HOST_RESPONSE_UPLOAD_PASS
      //    ANTFS_HOST_RESPONSE_UPLOAD_REJECT
      //    ANTFS_HOST_RESPONSE_UPLOAD_FAIL
      //    ANTFS_HOST_RESPONSE_CONNECTION_LOST
      //    ANTFS_HOST_RESPONSE_SERIAL_FAIL
      /////////////////////////////////////////////////////////////////

      virtual ANTFS_RETURN ManualTransfer(USHORT usFileIndex_, ULONG ulDataOffset_, ULONG ulDataLength_, void *pvData_) = 0;
      /////////////////////////////////////////////////////////////////
      // Send data directly to the device without requesting first.
      // This is especially useful for communicating small pieces of
      // data to the device.  Another use is the implementation of a
      // higher-level communication protocol.  Care must be taken to
      // ensure the device can handle the amount of data being sent
      // using this method.
      //    usFileIndex_:     The file number to be uploaded.  Some
      //                      file numbers are reserved for special
      //                      purposes, such as the device directory
      //                      (0).  Consult the ANT_FS specification
      //                      and any docs for specific device types.
      //    ulDataOffset_:    The byte offset from where to begin
      //                      transferring the data.  Note that this
      //                      value will always get rounded up to the
      //                      next higher multiple of 8.  Under normal
      //                      use, this parameter should always be set
      //                      to zero, and the only time it would be
      //                      non-zero is for retrying ManualTransfer()
      //                      from a known offset.
      //    ulDataLength_:    The byte length of data to be sent to
      //                      the remote device.
      //    *pvData_:         The Pointer to a buffer where the
      //                      data to be sent is stored.
      // Returns ANTFS_RETURN_PASS if successful.  Otherwise, it returns
      // ANTFS_RETURN_FAIL if the library is in the wrong state, or
      // ANTFS_RETURN_BUSY if the library is busy with another request.
      // Operation:
      // Once the request is posted, the application must wait for the
      // response from the ANTFSHost object.  Possible responses are:
      //    ANTFS_HOST_RESPONSE_MANUAL_TRANSFER_PASS
      //    ANTFS_HOST_RESPONSE_MANUAL_TRANSFER_TRANSMIT_FAIL
      //    ANTFS_HOST_RESPONSE_MANUAL_TRANSFER_RESPONSE_FAIL
      //    ANTFS_HOST_RESPONSE_CONNECTION_LOST
      //    ANTFS_HOST_RESPONSE_SERIAL_FAIL
      // Upon receiving ANTFS_HOST_RESPONSE_MANUAL_TRANSFER_PASS the downloaed data
      // will be available in the transfer buffer.  See GetTransferData().
      /////////////////////////////////////////////////////////////////

      virtual ANTFS_RETURN EraseData(USHORT usFileIndex_) = 0;
      /////////////////////////////////////////////////////////////////
      // Request the erasure of a file on the authenticated remote
      // device.
      // Parameters:
      //    usFileIndex_:     The file number of the file to be erased.
      // Returns ANTFS_RETURN_PASS if successful.  Otherwise, it returns
      // ANTFS_RETURN_FAIL if the library is in the wrong state, or
      // ANTFS_RETURN_BUSY if the library is busy with another request.
      // Operation:
      // Once the request is posted, the application must wait for the
      // response from the ANTFSHost object.  Possible responses are:
      //    ANTFS_HOST_RESPONSE_ERASE_PASS
      //    ANTFS_HOST_RESPONSE_ERASE_FAIL
      //    ANTFS_HOST_RESPONSE_CONNECTION_LOST
      //    ANTFS_HOST_RESPONSE_SERIAL_FAIL
      /////////////////////////////////////////////////////////////////

      virtual BOOL EnablePing(BOOL bEnable_) = 0;
      /////////////////////////////////////////////////////////////////
      // Enables ping message to be sent to the remote device periodically.
      // This can be used to keep the remote device from timing out during
      // operations that wait for user input (i.e. pairing).
      // Parameters:
      //    bEnable_:     Periodic ping enable.
      // Returns TRUE if successful.  Otherwise, it returns FALSE.
      /////////////////////////////////////////////////////////////////
};

#endif // !defined(ANTFS_HOST_INTERFACE_H)
