/*
This software is subject to the license described in the License.txt file
included with this software distribution. You may not use this file except
in compliance with this license.

Copyright (c) Dynastream Innovations Inc. 2016
All rights reserved.
*/
#if !defined(ANTFS_CLIENT_CHANNEL_HPP)
#define ANTFS_CLIENT_CHANNEL_HPP

#include "types.h"
#include "dsi_thread.h"
#include "dsi_timer.hpp"
#include "dsi_framer_ant.hpp"
#include "dsi_debug.hpp"

#include "dsi_response_queue.hpp"
#include "dsi_ant_message_processor.hpp"
#include "antfs_client_interface.hpp"


//////////////////////////////////////////////////////////////////////////////////
// Public Definitions
//////////////////////////////////////////////////////////////////////////////////
typedef struct
{
   char acFriendlyName[FRIENDLY_NAME_MAX_LENGTH];
   BOOL bNameSet;
   UCHAR ucIndex;
   UCHAR ucSize;
} ANTFS_FRIENDLY_NAME;

/////////////////////////////////////////////////////////////////
// This class implements the ANT-FS Client specification.
// It is intended to be used together with another class that manages
// the connection to an ANT USB stick (e.g. DSIANTDevice or
// .NET ANT_Device).
/////////////////////////////////////////////////////////////////
class ANTFSClientChannel : public ANTFSClientInterface, public DSIANTMessageProcessor
{
   private:

      //////////////////////////////////////////////////////////////////////////////////
      // Private Definitions
      //////////////////////////////////////////////////////////////////////////////////

      enum ENUM_ANTFS_REQUEST
      {
         ANTFS_REQUEST_NONE = 0,
         ANTFS_REQUEST_INIT,                                    // Init()
         ANTFS_REQUEST_OPEN_BEACON,                             // OpenBeacon()
         ANTFS_REQUEST_CLOSE_BEACON,                            // CloseBeacon()
         ANTFS_REQUEST_CONNECT,                                 // From Host: Switch to Authenticate
         ANTFS_REQUEST_DISCONNECT,                              // From Host: End session
         ANTFS_REQUEST_PING,                                    // From Host: Keep session alive
         ANTFS_REQUEST_AUTHENTICATE,                            // From Host: Send authentication Response
         ANTFS_REQUEST_PAIR,                                    // From Host: Send pairing request to application
         ANTFS_REQUEST_CHANGE_LINK,                             // From Host: Switch frequency/period
         ANTFS_REQUEST_DOWNLOAD,                                // From Host: Download requested
         ANTFS_REQUEST_DOWNLOAD_RESPONSE,                       // SendDownloadResponse();
         ANTFS_REQUEST_UPLOAD,                                  // From Host: Upload requested
         ANTFS_REQUEST_UPLOAD_RESPONSE,                         // SendUploadResponse()
         ANTFS_REQUEST_UPLOAD_COMPLETE,                         // SendUploadComplete()
         ANTFS_REQUEST_ERASE,                                   // From Host: Erase request
         ANTFS_REQUEST_ERASE_RESPONSE,                          // SendEraseResponse()
         ANTFS_REQUEST_CONNECTION_LOST,                         // Internal, keep these at the end of the list order is important
         ANTFS_REQUEST_HANDLE_SERIAL_ERROR,                     // Internal
         ANTFS_REQUEST_SERIAL_ERROR_HANDLED                     // Internal
      };

      enum RETURN_STATUS
      {
         RETURN_FAIL = 0,
         RETURN_PASS,
         RETURN_STOP,
         RETURN_REJECT,
         RETURN_NA,
         RETURN_SERIAL_ERROR
      };

      //////////////////////////////////////////////////////////////////////////////////
      // Private Variables
      //////////////////////////////////////////////////////////////////////////////////
      DSIResponseQueue<ANTFS_CLIENT_RESPONSE> clResponseQueue;

      BOOL bInitFailed;

      UCHAR aucResponseBuf[MESG_MAX_SIZE_VALUE];
      UCHAR aucRxBuf[MESG_MAX_SIZE_VALUE];

      DSI_THREAD_ID hANTFSThread;                           // Handle for the ANTFS thread.
      DSI_MUTEX stMutexResponseQueue;                       // Mutex used with the response queue
      DSI_MUTEX stMutexCriticalSection;                     // Mutex used with the wait condition
      DSI_MUTEX stMutexPairingTimeout;                      // Mutex used with the pairing timeouts
      DSI_CONDITION_VAR stCondANTFSThreadExit;              // Event to signal the ANTFS thread has ended
      DSI_CONDITION_VAR stCondRequest;                      // Event to signal there is a new request
      DSI_CONDITION_VAR stCondRxEvent;                      // Event to signal there is a new Rx message or failure
      DSI_CONDITION_VAR stCondWaitForResponse;              // Event to signal there is a new response to the application

      DSITimer *pclTimer;
      volatile BOOL bTimerRunning;
      volatile BOOL bANTFSThreadRunning;
      volatile BOOL bKillThread;
      volatile BOOL bCancel;       // Internal cancel parameter to use if not configured
      volatile BOOL *pbCancel;

      DSIFramerANT *pclANT;

      // ANT Channel parameters
      UCHAR ucNetworkNumber;
      //UCHAR ucChannelNumber;
      USHORT usRadioChannelID;      // ANT Channel Device ID
      UCHAR ucTheDeviceType;
      UCHAR ucTheTransmissionType;
      USHORT usTheMessagePeriod;
      UCHAR aucTheNetworkkey[8];

      UCHAR ucLinkTxPower;
      UCHAR ucSessionTxPower;
      BOOL bCustomTxPower;

      // Bursting
      volatile ULONG ulPacketCount;
      volatile BOOL bTxError;
      volatile BOOL bRxError;
      volatile BOOL bReceivedBurst;
      volatile BOOL bReceivedCommand;
      volatile BOOL bNewRxEvent;

      // Beacon parameters
      ANTFS_CLIENT_PARAMS stInitParams;         // Initial parameters
      UCHAR aucFriendlyName[FRIENDLY_NAME_MAX_LENGTH];   // Cache application defined friendly name
      UCHAR aucPassKey[PASSWORD_MAX_LENGTH];    // Cache application defined passkey
      UCHAR ucPassKeySize;                      // PassKey length
      UCHAR ucFriendlyNameSize;                 // Friendly name length
      UCHAR ucActiveBeaconFrequency;            // Active radio frequency for the beacon
      UCHAR ucActiveBeaconStatus1;              // Beacon Status 1 byte
      UCHAR aucBeacon[8];                       // Beacon buffer
      USHORT usBeaconChannelPeriod;             // If custom period (not defined by ANT-FS)

      // ANT-FS Broadcast
      BOOL bReturnToBroadcast;                  // Default action when closing the beacon

      // Link state
      ULONG ulHostSerialNumber;

      // Authentication state
      ANTFS_FRIENDLY_NAME stHostFriendlyName;   // Host Friendly Name
      UCHAR ucPassKeyIndex;                     // Current location of auth string Tx block
      UCHAR ucAuthCommandType;                  // Authentication command type in progress
      BOOL bAcceptRequest;                      // Accept/reject authentication request

      // Pairing
      UCHAR ucPairingTimeout;
      volatile BOOL bTimeoutEvent;

      // Disconnect
      ANTFS_DISCONNECT_PARAMS stHostDisconnectParams; // Parameters received from the hsot on a disconnect request

      // Transport state
      ANTFS_REQUEST_PARAMS stHostRequestParams; // Parameters received from the host on a file transfer request
      UCHAR ucRequestResponse;                  // Response from the application to the request

      // Data transfer
      USHORT usTransferDataFileIndex;           // Index of current file being transferred
      volatile ULONG ulTransferFileSize;        // File size of current transfer, in bytes
      volatile ULONG ulTransferBurstIndex;      // Current location within the burst block, in bytes
      volatile ULONG ulTransferBytesRemaining;  // Total remaining data length of the current block, in bytes
      volatile ULONG ulTransferMaxIndex;        // Upper limit of the current transmitted burst block, in bytes
      volatile ULONG ulTransferBlockSize;       // Maximum block size, limited by client device
      volatile USHORT usTransferCrc;            // Data CRC
      volatile USHORT usSavedTransferCrc;       // Used in case upload isn't complete
      volatile ULONG ulDownloadProgress;        // Current download progress (number of bytes transferred)

      volatile ULONG ulTransferBlockOffset;     // Offset, in bytes, of the data block provided by the application
      volatile ULONG ulTransferBufferSize;

      UCHAR *pucDownloadData;                   // Buffer with data to download
      UCHAR *pucTransferBufferDynamic;          // Dynamic buffer for uploads

      volatile ENUM_ANTFS_REQUEST eANTFSRequest;
      volatile ANTFS_CLIENT_STATE eANTFSState;
      volatile UCHAR ucLinkCommandInProgress;

      //////////////////////////////////////////////////////////////////////////////////
      // Private Function Prototypes
      //////////////////////////////////////////////////////////////////////////////////

      void ANTFSThread(void);
      static DSI_THREAD_RETURN ANTFSThreadStart(void *pvParameter_);
      void TimerCallback(void);
      static DSI_THREAD_RETURN TimerStart(void *pvParameter_);

      void SetDefaultBeacon(void);
      void ResetClientState(void);
      BOOL ReInitDevice(void);

      void HandleSerialError(void);

      BOOL FilterANTMessages(ANT_MESSAGE* pstMessage_, UCHAR ucANTChannel_);
      BOOL ANTProtocolEventProcess(UCHAR ucChannel_, UCHAR ucMessageCode_);
      BOOL ANTChannelEventProcess(UCHAR ucChannel_, UCHAR ucMessageCode_);

      void AddResponse(ANTFS_CLIENT_RESPONSE eResponse_);
      void LoadBeacon(void);

      RETURN_STATUS AttemptOpenBeacon(void);
      RETURN_STATUS AttemptCloseBeacon(void);
      RETURN_STATUS AttemptAuthenticateResponse(void);
      RETURN_STATUS AttemptEraseResponse(void);
      RETURN_STATUS AttemptDownloadResponse(void);
      RETURN_STATUS AttemptUploadResponse(void);
      RETURN_STATUS AttemptUploadDataResponse(void);

      void DecodeLinkCommand(UCHAR *pucLinkCommand_);
      void DecodeAuthenticateCommand(UCHAR ucControlByte_, UCHAR *pucAuthCommand_);
      void DecodeTransportCommand(UCHAR ucControlByte_, UCHAR *pucTransCommand_);
      void UploadInputData(UCHAR ucControlByte_, UCHAR* pucMesg_);

      RETURN_STATUS SwitchToLink(void);
      RETURN_STATUS SwitchToAuthenticate(void);
      RETURN_STATUS SwitchToTransport(void);
      RETURN_STATUS SwitchLinkParameters(void);

      void SetANTChannelPeriod(UCHAR ucLinkPeriod_);

   public:

      //////////////////////////////////////////////////////////////////////////////////
      // Public Function Prototypes
      //////////////////////////////////////////////////////////////////////////////////

      ANTFSClientChannel();
      ~ANTFSClientChannel();

      BOOL Init(DSIFramerANT* pclANT_, UCHAR ucChannel_);
      /////////////////////////////////////////////////////////////////
      // Begins to initialize the ANTFSClientChannel object.
      // Returns TRUE if successful.  Otherwise, it returns FALSE.
      // Parameters:
      //    *pclANT_:         Pointer to a DSIFramerANT object.
      //    ucChannel_:       Channel number to use for the ANT-FS host
      // Operation:
      //    This function is used from a class managing the connection
      //    to ANT (e.g. DSIANTDevice or .NET ANT_Device), to
      //    initialize the ANTFSClientChannel object. It is not possible
      //    to change the channel number once ANT-FS is running.
      //    The function returns immediately, and the ANTFSHostChannel object
      //    will send a response of ANTFS_HOST_RESPONSE_INIT_PASS.
      //    IT IS NOT NECESSARY TO CALL THIS FUNCTION DIRECTLY FROM USER APPLICATIONS.
      /////////////////////////////////////////////////////////////////

      void Close(void);
      /////////////////////////////////////////////////////////////////
      // Stops any pending actions, closes all threads and cleans
      // up any dynamic memory being used by the library.
      // Operation:
      //    This function is used from a class managing the connection
      //    to ANT (e.g. DSIANTDevice or .NET ANT_Device), to
      //    clean up any resources in use by the ANT-FS host.
      //    IT IS NOT NECESSARY TO CALL THIS FUNCTION DIRECTLY FROM USER APPLICATIONS.
      /////////////////////////////////////////////////////////////////

      void ProcessMessage(ANT_MESSAGE* pstMessage_, USHORT usMesgSize_);
      /////////////////////////////////////////////////////////////////
      // Processes incoming ANT messages as per the ANT-FS Technology
      // Specification
      // Parameters:
      //    pstMessage_:      Pointer to an ANT message structure
      //    usMesgSize_:      ANT message size
      // Operation:
      //    This function is used from a class managing the connection
      //    to ANT (e.g. DSIANTDevice or .NET ANT_Device).
      //    IT IS NOT NECESSARY TO CALL THIS FUNCTION DIRECTLY FROM USER APPLICATIONS.
      /////////////////////////////////////////////////////////////////

      void ProcessDeviceNotification(ANT_DEVICE_NOTIFICATION eCode_, void* pvParameter_);
      /////////////////////////////////////////////////////////////////
      // Processes device level notifications
      // Parameters:
      //    eCode_:          Device notification event code
      //    pvParameter_:    Pointer to struct defining specific parameters related
      //                     to the event code
      // Operation:
      //    This function is used from a class managing the connection
      //    to ANT (e.g. DSIANTDevice or .NET ANT_Device).
      //    IT IS NOT NECESSARY TO CALL THIS FUNCTION DIRECTLY FROM USER APPLICATIONS.
      /////////////////////////////////////////////////////////////////

      void Cancel(void);
      /////////////////////////////////////////////////////////////////
      // Cancels any pending actions and returns the library to the
      // appropriate ANTFS layer if possible.  ie if the library was
      // executing a download command in the transport layer, the
      // library would be returned to ANTFS_CLIENT_STATE_TRANSPORT after
      // execution of this function.
      /////////////////////////////////////////////////////////////////

      // TODO: Serial number is configured within InitParams.  Should it be a
      //    separate SetSerialNumber function for consistency with the host, or
      //    should the host be configured with a struct as in this function?
      ANTFS_RETURN ConfigureClientParameters(ANTFS_CLIENT_PARAMS* pstInitParams_);
      /////////////////////////////////////////////////////////////////
      // Set up the ANTFS Client configuration parameters.
      // Returns ANTFS_RETURN_PASS if successful.  Otherwise, it returns
      // ANTFS_RETURN_FAIL if the library is in the wrong state, or
      // ANTFS_RETURN_BUSY if the library is busy with another request.
      // Parameters:
      //   *pstInitParams_:   A pointer to an
      //                      ANTFS_PARAMS structure that defines the
      //                      configuration parameters of the client
      //                      device
      // Operation:
      //      This function can only be used before the beacon is open;
      //      Changes are only applied when calling OpenBeacon().
      //      Certain parameters can be changed at any time, see
      //          SetPairingEnabled
      //          SetUploadEnabled
      //          SetDataAvailable
      //          SetBeaconTimeout
      //          SetPairingTimeout
      //      If the client is not configured prior to opening the
      //      beacon, the default ANT-FS PC beacon configuration is used.
      /////////////////////////////////////////////////////////////////

      ANTFS_RETURN SetPairingEnabled(BOOL bEnable_);
      /////////////////////////////////////////////////////////////////
      // Enable handling of pairing authentication requests, and indicate
      // so in the beacon.
      // Parameters:
      //    bEnable_:         Set to TRUE to enable pairing and FALSE
      //                      to disable.
      // Returns ANTFS_RETURN_PASS if successful.  Otherwise, it returns
      // ANTFS_RETURN_FAIL if the library is in the wrong state, or
      // ANTFS_RETURN_BUSY if the library is busy with another request.
      // Operation:
      //    This function can be used at any time, except while handling
      //    an authentication request
      /////////////////////////////////////////////////////////////////

      ANTFS_RETURN SetUploadEnabled(BOOL bEnable_);
      /////////////////////////////////////////////////////////////////
      // Enable uploads, indicating so in the beacon.
      // Parameters:
      //    bEnable_:         Set to TRUE to enable pairing and FALSE
      //                      to disable.
      // Returns ANTFS_RETURN_PASS if successful.  Otherwise, it returns
      // ANTFS_RETURN_FAIL if the library is in the wrong state, or
      // ANTFS_RETURN_BUSY if the library is busy with another request.
      // Operation:
      //    This function can be used at any time, except while handling
      //    a transport request (i.e. during an upload)
      /////////////////////////////////////////////////////////////////

      ANTFS_RETURN SetDataAvailable(BOOL bDataAvailable_);
      /////////////////////////////////////////////////////////////////
      // Indicate in the beacon whether data is available for download.
      // Parameters:
      //    bDataAvailable_:  Set to TRUE if data is available and
      //                      FALSE if not.
      // Returns ANTFS_RETURN_PASS if successful.  Otherwise, it returns
      // ANTFS_RETURN_FAIL if the library is in the wrong state, or
      // ANTFS_RETURN_BUSY if the library is busy with another request.
      // Operation:
      //    This function can be used at any time, except while handling
      //    a transport request (i.e. during a download)
      /////////////////////////////////////////////////////////////////

      // TODO: SetState (Status1 byte, consistent with integrated FS interface)?

      void SetBeaconTimeout(UCHAR ucTimeout_);
      /////////////////////////////////////////////////////////////////
      // Set up the time the client will wait without receiving any
      // commands from the host before dropping back to the link state
      // Parameters:
      //   ucTimeout_:           Timeout, in seconds. Set to 0xFF to
      //                         disable (infinite timeout).
      //                         Zero is not an allowed value.
      /////////////////////////////////////////////////////////////////

      void SetPairingTimeout(UCHAR ucTimeout_);
      /////////////////////////////////////////////////////////////////
      // Set up the time the client will wait without receiving any
      // response from the application to a pairing request before
      // rejecting it
      // Parameters:
      //   ucTimeout_:           Timeout, in seconds. Set to 0xFF to
      //                         disable (infinite timeout).
      //                         Zero is not an allowed value.
      /////////////////////////////////////////////////////////////////

      ANTFS_RETURN SetFriendlyName(UCHAR* pucFriendlyName_, UCHAR ucFriendlyNameSize_);
      /////////////////////////////////////////////////////////////////
      // Set up a friendly name for the ANT-FS client
      // Parameters:
      //    *pucFriendlyName: A pointer to a character string
      //                      containing the friendly name of the client.
      //                      Set to NULL to disable
      //    ucFriendlyNameSize: Size of the friendly name string (max 255)
      // Returns ANTFS_RETURN_PASS if successful.  Otherwise, it returns
      // ANTFS_RETURN_FAIL if the library is in the wrong state, or
      // ANTFS_RETURN_BUSY if the library is busy with another request.
      // Operation:
      //    This function can be used at any time, except while handling
      //    an authentication request.
      //    No friendly name will be sent with authentication responses
      //    unless configured with this command.
      //    The friendly name is cached by the client library.
      /////////////////////////////////////////////////////////////////

      ANTFS_RETURN SetPassKey(UCHAR* pucPassKey_, UCHAR ucPassKeySize_);
      /////////////////////////////////////////////////////////////////
      // Set up the pass key for the client to establish authenticated
      // sessions with a host device
      // Parameters:
      //    *pucPassKey:      Array containing the pass key
      //                      Set to NULL to disable
      //    ucPassKeySize:    Size of the passkey (max 255)
      // Returns ANTFS_RETURN_PASS if successful.  Otherwise, it returns
      // ANTFS_RETURN_FAIL if the library is in the wrong state, or
      // ANTFS_RETURN_BUSY if the library is busy with another request.
      // Operation:
      //    This function can be used at any time, except while handling
      //    an authentication request.
      //    PassKey authentication is disabled by default, unless
      //    this command is called to configure a key.
      //    The passkey is cached by the client library.
      /////////////////////////////////////////////////////////////////

      void SetChannelID(UCHAR ucDeviceType_, UCHAR ucTransmissionType_);
      /////////////////////////////////////////////////////////////////
      // Set up the ANT Channel ID for the ANT-FS Client
      // Parameters:
      //   ucDeviceType_:        ANT Channel Device Type
      //   ucTransmissionType_:  ANT Channel Transmission Type
      // Operation:
      //   Configuration changes are applied when the beacon is opened
      /////////////////////////////////////////////////////////////////

      void SetChannelPeriod(USHORT usChannelPeriod_);
      /////////////////////////////////////////////////////////////////
      // Set up a custom ANT channel period
      // Parameters:
      //   usChannelPeriod_:     Message count, in seconds * 32768.
      //                         For example, for 4Hz, set to 8192.
      // Operation:
      //   Use this function if using ANT-FS broadcast and configuring a
      //   beacon period not defined in the ANT-FS Technology Specification
      //   When using this option, set the Link Period beacon parameter to
      //   BEACON_PERIOD_KEEP.  This is the channel period the client will
      //   use when in LINK state or when it returns to broadcast.
      /////////////////////////////////////////////////////////////////

      void SetNetworkKey(UCHAR ucNetwork_, UCHAR ucNetworkKey[]);
      /////////////////////////////////////////////////////////////////
      // Set up the network key to use with the ANT-FS Client
      // Parameters:
      //   ucNetwork_:        Network number
      //   ucNetorkKey_:      Array containing the 8-byte network key
      // Operation:
      //   Configuration changes are applied when the beacon is opened
      /////////////////////////////////////////////////////////////////

      void SetTxPower(UCHAR ucPairingLv_, UCHAR ucConnectedLv_);
      /////////////////////////////////////////////////////////////////
      // Set up the transmit power for the ANT-FS Channel.
      // Parameters:
      //      ucPairingLv_:   Power level to use while beaconing (link)
      //      ucConnectedLv_: Power level to use during a session
      // Operation:
      //      This command can be used to facilitate pairing when
      //      proximity search is used in the host device.
      //      If the ANT part does not support setting the transmit power
      //      on a per channel basis, it is set for all channels
      //      Configuration changes are applied when the client switches
      //      to the link and authentication states.
      /////////////////////////////////////////////////////////////////

      ANTFS_RETURN OpenBeacon(void);
      /////////////////////////////////////////////////////////////////
      // Begins the channel configuration to transmit the ANT-FS beacon
      // Returns ANTFS_RETURN_PASS if successful.  Otherwise, it returns
      // ANTFS_RETURN_FAIL if the library is in the wrong state, or
      // ANTFS_RETURN_BUSY if the library is busy with another request.
      // Operation:
      //    This function will configure the beacon and start broadcasting.
      //    If the channel is already open (i.e. broadcast mode), only the
      //    contents of the beacon will change, not the channel configuration.
      //    Once the channel is successfully configured and opened,
      //    a response of ANTFS_CLIENT_RESPONSE_BEACON_OPEN will be sent.
      /////////////////////////////////////////////////////////////////

      ANTFS_RETURN CloseBeacon(BOOL bReturnToBroadcast_ = FALSE);
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

      BOOL GetEnabled(void);
      /////////////////////////////////////////////////////////////////
      // Returns the current status of ANT-FS message processing
      // Returns TRUE if ANT-FS is enabled.  Otherwise, it returns FALSE.
      // Operation:
      //    This function is used from a class managing the connection
      //    to ANT (e.g. DSIANTDevice or .NET ANT_Device).
      //    IT IS NOT NECESSARY TO CALL THIS FUNCTION DIRECTLY FROM USER APPLICATIONS.
      /////////////////////////////////////////////////////////////////

      ANTFS_CLIENT_STATE GetStatus(void);
      /////////////////////////////////////////////////////////////////
      // Returns the current library status.
      /////////////////////////////////////////////////////////////////

      BOOL GetHostName(UCHAR *aucHostFriendlyName_, UCHAR *pucBufferSize_);
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

      BOOL GetRequestParameters(ANTFS_REQUEST_PARAMS* pstRequestParams_);
      /////////////////////////////////////////////////////////////////
      // Gets the full parameters for a download, upload or erase request
      // received from the host.
      //
      // Parameters:
      //    *pstRequestParams_: Pointer to an ANTFS_REQUEST_PARAMS
      //                      structure that will receive the details
      //                      of the download, upload or erase request.
      // Returns TRUE if successful.  Otherwise, it returns FALSE.
      // Operation:
      //    This function makes available all of the parameters received
      //    from the host when requesting a data transfer.  These
      //    parameters are available for information purposes; the client
      //    application only needs to process the index to handle the
      //    request.
      //    This information is valid while a download, upload or erase
      //    request is in progress.
      /////////////////////////////////////////////////////////////////

      BOOL GetRequestedFileIndex(USHORT *pusIndex_);
      /////////////////////////////////////////////////////////////////
      // Gets the index requested for a download, upload or erase request
      // Parameters:
      //    *pulByteProgress_: Pointer to a USHORT that will receive
      //                      the current fle index requested
      // Returns TRUE if successful.  Otherwise, it returns FALSE.
      // Operation:
      //    This information is valid while a download, upload or erase
      //    request is in progress.
      /////////////////////////////////////////////////////////////////

      BOOL GetDownloadStatus(ULONG *pulByteProgress_, ULONG *pulTotalLength_);
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
      // A data download occurs when information is requested from a
      // remote device.  This function may be called at any point
      // during a download as a progress indicator.  After the transfer
      // is complete, this information is valid until another request
      // for a data transfer is made.
      /////////////////////////////////////////////////////////////////

      BOOL GetUploadStatus(ULONG *pulByteProgress_, ULONG *pulTotalLength_);
      /////////////////////////////////////////////////////////////////
      // Gets the transfer progress of a pending or a complete
      // upload.
      // Parameters:
      //    *pulByteProgress_: Pointer to a ULONG that will receive
      //                      the current byte progress of a pending or
      //                      complete upload.
      //    *pulTotalLength_: Pointer to a ULONG that will receive the
      //                      total expected length of the upload.
      // Returns TRUE if successful.  Otherwise, it returns FALSE.
      // Operation:
      // A data upload occurs when information is sent to a
      // remote device.  This function may be called at any point
      // during an upload as a progress indicator.  After the transfer
      // is complete, this information is valid until another request
      // for a data transfer is made.
      /////////////////////////////////////////////////////////////////

      BOOL GetTransferData(ULONG *pulOffset_, ULONG *pulDataSize_ , void *pvData_ = NULL);
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

      BOOL GetDisconnectParameters(ANTFS_DISCONNECT_PARAMS* pstDisconnectParams_);
      /////////////////////////////////////////////////////////////////
      // Gets the full parameters for a disconnect command
      // received from the host.
      //
      // Parameters:
      //    *pstRequestParams_: Pointer to an ANTFS_DISCONNECT_PARAMS
      //                      structure that will receive the details
      //                      of the disconnect request.
      // Returns TRUE if successful.  Otherwise, it returns FALSE.
      // Operation:
      //    This function makes available all of the parameters received
      //    from the host when requesting to disconnect from the client.
      //    These parameters can let the client know whether it is
      //    returning to broadcast/link state, as well as if it needs
      //    to make itself undiscoverable for a period of time.
      //    This information is valid after an ANTFS_RESPONSE_DISCONNECT_PASS
      //    is received
      /////////////////////////////////////////////////////////////////

      ANTFS_RETURN SendPairingResponse(BOOL bAccept_);
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
      //    application will receive an ANTFS_CLIENT_RESPONSE_PAIRING_TIMEOUT
      //    response.
      /////////////////////////////////////////////////////////////////

      ANTFS_RETURN SendDownloadResponse(UCHAR ucResponse_, ANTFS_DOWNLOAD_PARAMS* pstDownloadInfo_, ULONG ulDataLength_, void *pvData_);
      /////////////////////////////////////////////////////////////////
      // Sends the response to a download request from an authenticated
      // device.
      // Parameters:
      //    ucResponse_:      The response to the download request.
      //    stDownloadInfo_:  Pointer to an ANTFS_CLIENT_DOWNLOAD_PARAMS
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

      ANTFS_RETURN SendUploadResponse(UCHAR ucResponse_, ANTFS_UPLOAD_PARAMS* pstUploadInfo_, ULONG ulDataLength_, void *pvData_);
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
      //    response from the library.  Possible responses are:
      //       ANTFS_CLIENT_RESPONSE_UPLOAD_PASS
      //       ANTFS_CLIENT_RESPONSE_UPLOAD_REJECT
      //       ANTFS_CLIENT_RESPONSE_UPLOAD_FAIL
      //       ANTFS_CLIENT_RESPONSE_SERIAL_FAIL
      // Upon receiving ANTFS_CLIENT_RESPONSE_UPLOAD_PASS the uploaded data
      // will be available in the transfer buffer.  See GetTransferData().
      /////////////////////////////////////////////////////////////////

      ANTFS_RETURN SendEraseResponse(UCHAR ucResponse_);
      /////////////////////////////////////////////////////////////////
      // Sends a response to a request to erase a file from an
      // authenticated remote device
      // Parameters:
      //    ucResponse_:      The response to the erase request.
      // Returns ANTFS_RETURN_PASS if successful.  Otherwise, it returns
      // ANTFS_RETURN_FAIL if the library is in the wrong state, or
      // ANTFS_RETURN_BUSY if the library is busy with another request.
      /////////////////////////////////////////////////////////////////

      ANTFS_CLIENT_RESPONSE WaitForResponse(ULONG ulMilliseconds_);
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


#endif // ANTFS_CLIENT_CHANNEL_HPP