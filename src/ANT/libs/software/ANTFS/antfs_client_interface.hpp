/*
This software is subject to the license described in the License.txt file
included with this software distribution. You may not use this file except
in compliance with this license.

Copyright (c) Dynastream Innovations Inc. 2016
All rights reserved.
*/
#if !defined(ANTFS_CLIENT_INTERFACE_H)
#define ANTFS_CLIENT_INTERFACE_H

#include "types.h"
#include "antfs_interface.h"

//////////////////////////////////////////////////////////////////////////////////
// Public Definitions
//////////////////////////////////////////////////////////////////////////////////

// ANT-FS Client Responses
typedef enum
{
   ANTFS_CLIENT_RESPONSE_NONE = 0,
   ANTFS_CLIENT_RESPONSE_INIT_PASS,
   ANTFS_CLIENT_RESPONSE_SERIAL_FAIL,
   ANTFS_CLIENT_RESPONSE_BEACON_OPEN,
   ANTFS_CLIENT_RESPONSE_BEACON_CLOSED,
   ANTFS_CLIENT_RESPONSE_CONNECT_PASS,
   ANTFS_CLIENT_RESPONSE_DISCONNECT_PASS,
   ANTFS_CLIENT_RESPONSE_CONNECTION_LOST,
   ANTFS_CLIENT_RESPONSE_AUTHENTICATE_NA,
   ANTFS_CLIENT_RESPONSE_AUTHENTICATE_PASS,
   ANTFS_CLIENT_RESPONSE_AUTHENTICATE_REJECT,
   ANTFS_CLIENT_RESPONSE_PAIRING_REQUEST,
   ANTFS_CLIENT_RESPONSE_PAIRING_TIMEOUT,
   ANTFS_CLIENT_RESPONSE_DOWNLOAD_REQUEST,
   ANTFS_CLIENT_RESPONSE_DOWNLOAD_PASS,
   ANTFS_CLIENT_RESPONSE_DOWNLOAD_REJECT,
   ANTFS_CLIENT_RESPONSE_DOWNLOAD_INVALID_INDEX,
   ANTFS_CLIENT_RESPONSE_DOWNLOAD_FILE_NOT_READABLE,
   ANTFS_CLIENT_RESPONSE_DOWNLOAD_NOT_READY,
   ANTFS_CLIENT_RESPONSE_DOWNLOAD_FAIL,
   ANTFS_CLIENT_RESPONSE_UPLOAD_REQUEST,
   ANTFS_CLIENT_RESPONSE_UPLOAD_PASS,
   ANTFS_CLIENT_RESPONSE_UPLOAD_REJECT,
   ANTFS_CLIENT_RESPONSE_UPLOAD_INVALID_INDEX,
   ANTFS_CLIENT_RESPONSE_UPLOAD_FILE_NOT_WRITEABLE,
   ANTFS_CLIENT_RESPONSE_UPLOAD_INSUFFICIENT_SPACE,
   ANTFS_CLIENT_RESPONSE_UPLOAD_FAIL,
   ANTFS_CLIENT_RESPONSE_ERASE_REQUEST,
   ANTFS_CLIENT_RESPONSE_ERASE_PASS,
   ANTFS_CLIENT_RESPONSE_ERASE_REJECT,
   ANTFS_CLIENT_RESPONSE_ERASE_FAIL,
   ANTFS_CLIENT_RESPONSE_CANCEL_DONE
} ANTFS_CLIENT_RESPONSE;

// ANT-FS Client States
typedef enum
{
   ANTFS_CLIENT_STATE_OFF = 0,      // Object created, but not initialized (e.g. no threads running)
   ANTFS_CLIENT_STATE_IDLE,         // Object initialized (e.g. threads running), but not processing messages
   ANTFS_CLIENT_STATE_OPENING,
   ANTFS_CLIENT_STATE_DISCONNECTING,
   ANTFS_CLIENT_STATE_BEACONING,    // LINK
   ANTFS_CLIENT_STATE_CONNECTED,    // AUTH
   ANTFS_CLIENT_STATE_AUTHENTICATING,
   ANTFS_CLIENT_STATE_PAIRING_WAIT_FOR_RESPONSE,
   ANTFS_CLIENT_STATE_TRANSPORT,
   ANTFS_CLIENT_STATE_DOWNLOADING,
   ANTFS_CLIENT_STATE_DOWNLOADING_WAIT_FOR_DATA,
   ANTFS_CLIENT_STATE_UPLOADING,
   ANTFS_CLIENT_STATE_UPLOADING_WAIT_FOR_RESPONSE,
   ANTFS_CLIENT_STATE_ERASING,
} ANTFS_CLIENT_STATE;

// ANT-FS Client Parameters
typedef struct
{
   ULONG ulSerialNumber;            // Client serial number.  Set to zero to use the serial number of the USB device.
   USHORT usBeaconDeviceType;       // Client device type (in beacon)
   USHORT usBeaconDeviceManufID;    // Client manufacturing ID (in beacon)
   UCHAR ucBeaconFrequency;         // Link Radio Frequency
   UCHAR ucLinkPeriod;              // Link Channel Period
   BOOL bPairingEnabled;            // Pairing is enabled/disabled
   BOOL bUploadEnabled;             // Upload is enabled/disabled
   BOOL bDataAvailable;             // Data is available/not available for download
   UCHAR ucAuthType;                // Authentication type to include in beacon
   UCHAR ucBeaconTimeout;           // In seconds.  Timeout disabled = 0xFF.
   UCHAR ucPairingTimeout;          // In seconds.  Timeout disabled = 0xFF.
} ANTFS_CLIENT_PARAMS;

// Parameters received from host for the requested download/upload/erase
typedef struct
{
   USHORT usFileIndex;     // (Download/Upload/Erase) File index
   ULONG ulOffset;         // (Download/Upload) Current offset
   ULONG ulBlockSize;      // (Download) Maximum number of bytes in download block
   ULONG ulMaxSize;        // (Upload) Offset + total remaining bytes.
   USHORT usCRCSeed;       // (Download) CRC seed for the current download
   BOOL bInitialRequest;   // (Download) Flag indicating this is an initial request
} ANTFS_REQUEST_PARAMS;

// Parameters received from host for disconnect command
typedef struct
{
   UCHAR ucCommandType;    // Disconnect command type
   UCHAR ucTimeDuration;   // Requested amount in time (in 30 s increments) to become undiscoverable
   UCHAR ucAppSpecificDuration;  // Requested application specific undiscoverable time
} ANTFS_DISCONNECT_PARAMS;


// Client defined parameters for a download response
typedef struct
{
   USHORT usFileIndex;     // File index
   ULONG ulMaxBlockSize;   // Maximum burst block size that can be sent by the client
} ANTFS_DOWNLOAD_PARAMS;


// Client defined parameters for an upload response
typedef struct
{
   USHORT usFileIndex;     // File index
   ULONG ulMaxSize;        // Maximum number of bytes that can be written to the file
   ULONG ulMaxBlockSize;   // Maximum burst block size that can be received by the client
} ANTFS_UPLOAD_PARAMS;

//////////////////////////////////////////////////////////////////////////////////
// Public Function Prototypes
//////////////////////////////////////////////////////////////////////////////////

// TODO: Is this class used for anything?  Do we need it?
class ANTFSClientInterface
{
   public:
      virtual ~ANTFSClientInterface(){}

      virtual void Close(void) = 0;
      /////////////////////////////////////////////////////////////////
      // Stops any pending actions, and cleans up any dynamic memory
      // being used by the library.
      /////////////////////////////////////////////////////////////////

      virtual ANTFS_RETURN ConfigureClientParameters(ANTFS_CLIENT_PARAMS* pstInitParams_) = 0;
      /////////////////////////////////////////////////////////////////
      // Set up ANTFSClient configuration parameters.
      // Returns ANTFS_RETURN_PASS if successful.  Otherwise, it returns
      // ANTFS_RETURN_FAIL if the library is in the wrong state, or
      // ANTFS_RETURN_BUSY if the library is busy with another request.
      // Parameters:
      //   *pstInitParams_:   A pointer to an
      //                      ANTFS_PARAMS structure that defines the
      //                      configuration parameters of the client
      //                      device
      /////////////////////////////////////////////////////////////////

      virtual ANTFS_RETURN OpenBeacon() = 0;
      /////////////////////////////////////////////////////////////////
      // Begins the channel configuration to transmit the ANT-FS beacon
      // Returns ANTFS_RETURN_PASS if successful.  Otherwise, it returns
      // ANTFS_RETURN_FAIL if the library is in the wrong state, or
      // ANTFS_RETURN_BUSY if the library is busy with another request.
      // Operation:
      //    Once the channel is successfully configured and opened,
      //    a response of ANTFS_RESPONSE_BEACON_OPEN will be sent.
      /////////////////////////////////////////////////////////////////

      virtual ANTFS_RETURN CloseBeacon(BOOL bReturnToBroadcast_ = FALSE) = 0;
      /////////////////////////////////////////////////////////////////
      // Closes the ANT-FS beacon.
      // Returns ANTFS_RETURN_PASS if successful.  Otherwise, it returns
      // ANTFS_RETURN_FAIL if the library is in the wrong state, or
      // ANTFS_RETURN_BUSY if the library is busy with another request.
      // Parameters:
      //      bReturnToBroadcast_:   If TRUE, the channel will remain
      //                         open, and the application can continue
      //                         to process messages. If FALSE,
      //                         the channel will be closed.
      // Operation:
      //    Once the channel is successfully closed, a response of
      //    ANTFS_CLIENT_RESPONSE_BEACON_CLOSED will be sent,
      //    and the ANTFSClient will be in the ANTFS_STATE_IDLE state.
      //    If the ReturnToBroadcast parameter is not used, the default
      //    behavior will be to close the channel and stop broadcasting.
      /////////////////////////////////////////////////////////////////

      virtual ANTFS_CLIENT_STATE GetStatus(void) = 0;
      /////////////////////////////////////////////////////////////////
      // Returns the current library status.
      /////////////////////////////////////////////////////////////////

      virtual BOOL GetHostName(UCHAR *aucHostFriendlyName_, UCHAR *pucBufferSize_) = 0;
      /////////////////////////////////////////////////////////////////
      // Copies at most ucBufferSize_ characters from the host's
      // friendly name string (for the most recent session) into the
      // supplied pucHostFriendlyName_ buffer.
      // Parameters:
      //    *aucFriendlyName_: A pointer to a buffer where the remote
      //                      device friendly name will be copied.
      //    *pucBufferSize_:  Pointer to a UCHAR variable that should contain the
      //                      maximum size of the buffer pointed to by
      //                      aucHostFriendlyName_.
      //                      After the function returns, the UCHAR variable
      //                      will be set to reflect the size of the friendly
      //                      name string that has been copied to the buffer.
      // Returns TRUE if successful.  Otherwise, it returns FALSE.
      // Operation:
      //    If the host's friendly name string has fewer than ucBufferSize_
      //    characters, the *aucFriendlyName_ buffer will be padded with
      //    zeroes.
      /////////////////////////////////////////////////////////////////

      virtual BOOL GetRequestParameters(ANTFS_REQUEST_PARAMS* stRequestParams_) = 0;
      /////////////////////////////////////////////////////////////////
      // Gets the parameters for a download, upload or erase request
      // received from the host
      //
      // Parameters:
      //    *pstRequestParams_: Pointer to an ANTFS_REQUEST_PARAMS
      //                      structure that will receive the details
      //                      of the download, upload or erase request.
      // Returns TRUE if successful.  Otherwise, it returns FALSE.
      /////////////////////////////////////////////////////////////////

      virtual BOOL GetDownloadStatus(ULONG *pulByteProgress_, ULONG *pulTotalLength_) = 0;
      /////////////////////////////////////////////////////////////////
      // Gets the transfer progress of a pending or a complete
      // download.
      // Parameters:
      //    *pulByteProgress_: Pointer to a ULONG that will receive
      //                      the current byte progress of a pending or
      //                      complete download.
      //    *pulTotalLength_: Pointer to a ULONG that will receive the
      //                      total expected length of the download.
      // Returns TRUE if successful.  Otherwise, it returns FALSE.
      // Operation:
      //    A data download occurs when information is requested from a
      //    remote device.  This function may be called at any point
      //    during a download as a progress indicator.  After the transfer
      //    is complete, this information is valid until another request
      //    for a data transfer is made.
      /////////////////////////////////////////////////////////////////

      virtual BOOL GetTransferData(ULONG *pulOffset_ ,ULONG *pulDataSize_ , void *pvData_ = NULL) = 0;
      /////////////////////////////////////////////////////////////////
      // Gets the received data from a transfer (upload).
      //
      // Parameters:
     //    *pulOffset_:      Pointer to a ULONG that will receive
     //                the offset for the client file
      //    *ulDataSize_:     Pointer to a ULONG that will receive
      //                      the size of the data available in bytes.
      //    *pvData_:         Pointer to a buffer where the received data
      //                      will be copied.  NULL can be passed to this
      //                      parameter so that the size can be retrieved
      //                      without copying any data.  The application
      //                      can then call this function again to after it
      //                      has allocated a buffer of sufficient size to
      //                      handle the data.
      // Returns TRUE if successful.  Otherwise, it returns FALSE.
      /////////////////////////////////////////////////////////////////

      virtual ANTFS_RETURN SendPairingResponse(BOOL bAccept_) = 0;
      /////////////////////////////////////////////////////////////////
      // Sends a response to a pairing request.
      // Parameters:
      //    bAccept_:         Set this value to TRUE to accept the
      //                      pairing request, and FALSE to reject it.
      // Returns ANTFS_RETURN_PASS if successful.  Otherwise, it returns
      // ANTFS_RETURN_FAIL if the library is in the wrong state, or
      // ANTFS_RETURN_BUSY if the library is busy with another request.
      // Operation:
      //    A pairing request will automatically be rejected if no
      //    response is sent within the pairing timeout, and the
      //    application will receive an ANTFS_RESPONSE_PAIRING_TIMEOUT
      //    response.
      /////////////////////////////////////////////////////////////////

      virtual ANTFS_RETURN SendDownloadResponse(UCHAR ucResponse_, ANTFS_DOWNLOAD_PARAMS* stDownloadInfo_, ULONG ulDataLength_, void *pvData_) = 0;
      /////////////////////////////////////////////////////////////////
      // Sends the response to a download request from an authenticated
      // device.
      // Parameters:
      //    ucResponse_:      The response to the download request.
      //    stDownloadInfo_:  Pointer to an ANTFS_DOWNLOAD_PARAMS
      //                      structure holding the parameters of the
      //                      download response.
      //    ulDataLength_:    The byte length of the data block to be
      //                      downloaded to the host device. This is the
      //                      size of the entire file, as specified in
      //                      the directory.  Set to zero if no data
      //                      is to be downloaded.
      //    *pvData_:         Pointer to the location where the data
      //                      to be downloaded is stored.  The pointer
      //                      should correspond with the beginning of the
      //                      file, without applying any offsets.
      //                      Set to NULL if no data is to be
      //                      downloaded (response rejected).
      // Returns ANTFS_RETURN_PASS if successful.  Otherwise, it returns
      // ANTFS_RETURN_FAIL if the library is in the wrong state, or
      // ANTFS_RETURN_BUSY if the library is busy with another request.
      // Operation:
      //    The data block provided to this function should be the entire
      //    file. Handling of offsets and CRC calculations is done
      //    internally within the library.
      //    Once the request is posted, the application must wait for the
      //    response from the library.  Possible responses are:
      //       ANTFS_CLIENT_RESPONSE_DOWNLOAD_PASS
      //       ANTFS_CLIENT_RESPONSE_DOwNLOAD_REJECT
      //       ANTFS_CLIENT_RESPONSE_DOWNLOAD_FAIL
      //       ANTFS_CLIENT_RESPONSE_SERIAL_FAIL
      /////////////////////////////////////////////////////////////////

      virtual ANTFS_RETURN SendUploadResponse(UCHAR ucResponse_, ANTFS_UPLOAD_PARAMS* stUploadInfo_, ULONG ulDataLength_, void *pvData_) = 0;
     /////////////////////////////////////////////////////////////////
      // Sends the response to an upload request from an authenticated
      // device.
      // Parameters:
      //    ucResponse_:      The response to the upload request.
      //    pstUploadInfo_:   Pointer to an ANTFS_UPLOAD_PARAMS
      //                      structure holding the parameters of the
      //                      upload response.
      //    ulDataLength_:    The byte length of the data that is
      //                      currently stored at the requested upload
      //                      location.  Set to zero if uploading to a
      //                      new index or if the uploaded data will
      //                      overwrite all existing data
      //    *pvData_:         Pointer to the location of the data at the
      //                      requested upload index. Set to NULL
      //                      if no data is available or if the uploaded
      //                      data will overwrite the existing file.
      // Returns ANTFS_RETURN_PASS if successful.  Otherwise, it returns
      // ANTFS_RETURN_FAIL if the library is in the wrong state, or
      // ANTFS_RETURN_BUSY if the library is busy with another request.
      // Operation:
      //    The data block provided to this function should be the entire
      //    file. Handling of offsets and CRC calculations is done
      //    internally within the library.
      //    Once the request is posted, the application must wait for the
      //    response from the ANTFSClient object.  Possible responses are:
      //       ANTFS_CLIENT_RESPONSE_UPLOAD_PASS
      //       ANTFS_CLIENT_RESPONSE_UPLOAD_REJECT
      //       ANTFS_CLIENT_RESPONSE_UPLOAD_FAIL
      //       ANTFS_CLIENT_RESPONSE_SERIAL_FAIL
      // Upon receiving ANTFS_CLIENT_RESPONSE_UPLOAD_PASS the uploaded data
      // will be available in the transfer buffer.  See GetTransferData().
      /////////////////////////////////////////////////////////////////

      virtual ANTFS_RETURN SendEraseResponse(UCHAR ucResponse_) = 0;
      /////////////////////////////////////////////////////////////////
      // Sends a response to a request to erase a file from an
      // authenticated remote device
      // Parameters:
      //    ucResponse_:      The response to the erase request.
      // Returns ANTFS_RETURN_PASS if successful.  Otherwise, it returns
      // ANTFS_RETURN_FAIL if the library is in the wrong state, or
      // ANTFS_RETURN_BUSY if the library is busy with another request.
      /////////////////////////////////////////////////////////////////

      virtual ANTFS_CLIENT_RESPONSE WaitForResponse(ULONG ulMilliseconds_) = 0;
      /////////////////////////////////////////////////////////////////
      // Wait for a response from the ANTFS client library
      // Parameters:
      //    ulMilliseconds_:  Set this value to the minimum time to
      //                      wait before returning.  If the value is
      //                      0, the function will return immediately.
      //                      If the value is DSI_THREAD_INFINITE, the
      //                      function will not time out.
      // If one or more responses are pending before the timeout
      // expires the function will return the first response that
      // occurred.  If no response is pending at the time the timeout
      // expires, ANTFS_CLIENT_RESPONSE_NONE is returned.
      // Operation:
      // Some of the events return parameters associated with the event.
      // Possible events and parameters:
      //    ANTFS_CLIENT_RESPONSE_PAIRING_REQUEST   -  GetHostName()
      //    ANTFS_CLIENT_RESPONSE_DOWNLOAD_REQUEST  -  GetRequestParameters()
      //    ANTFS_CLIENT_RESPONSE_UPLOAD_REQUEST    -  GetRequestParameters()
      //    ANTFS_CLIENT_RESPONSE_ERASE_REQUEST     -  GetRequestParameters()
      //    ANTFS_CLIENT_RESPONSE_UPLOAD_PASS       -  GetTransferData()
      /////////////////////////////////////////////////////////////////
};


#endif // ANTFS_CLIENT_INTERFACE_H