/*
This software is subject to the license described in the License.txt file
included with this software distribution. You may not use this file except
in compliance with this license.

Copyright (c) Dynastream Innovations Inc. 2016
All rights reserved.
*/
#if !defined(ANTFS_MESSAGES_H)
#define ANTFS_MESSAGES_H

//////////////////////////////////////////////////////////////////////////////////
// Private ANT-FS Definitions for both Host and Client
//////////////////////////////////////////////////////////////////////////////////

// ANT Application Configuration
#define ANTFS_NETWORK                  ((UCHAR) 0)
#define ANTFS_CHANNEL                  ((UCHAR) 0)

#define NETWORK_KEY                    {0xA8, 0xA4, 0x23, 0xB9, 0xF5, 0x5E, 0x63, 0xC1}   // ANTFS Network Key

// Default Link Configuration
#define ANTFS_DEVICE_TYPE              ((UCHAR) 1)
#define ANTFS_TRANSMISSION_TYPE        ((UCHAR) 5)
#define ANTFS_DEVICE_NUMBER            ((USHORT) 0)         // 0 for searching (host)
#define ANTFS_CLIENT_NUMBER            ((USHORT) 0x0F0F)    // if serial number's two LSB are zero (client)
//#define ANTFS_MESSAGE_PERIOD         ((USHORT) 65535)     // 0.5Hz
#define ANTFS_MESSAGE_PERIOD           ((USHORT) 4096)      // 8 Hz
#define ANTFS_RF_FREQ                  ((UCHAR) 50)         // 2450 MHz for ANT FS
#define ANTFS_SEARCH_TIMEOUT           ((UCHAR) 255)        // Longest search timeout.

///////////////////////////////////////////////////////////////////////
// Always On / Transport Layer Command/Response definitions
///////////////////////////////////////////////////////////////////////
#define ANTFS_BEACON_ID                ((UCHAR) 0x43)
#define ANTFS_COMMAND_RESPONSE_ID      ((UCHAR) 0x44)
#define ANTFS_COMMAND_ID               ((UCHAR) 0x44)
#define ANTFS_RESPONSE_ID              ((UCHAR) 0x44)
#define ANTFS_REQUEST_PAGE_ID          ((UCHAR) 0x46)

// Commands
#define ANTFS_CMD_NONE                 ((UCHAR) 0x00)
#define ANTFS_STATUS_ID                ((UCHAR) 0x01)
#define ANTFS_LINK_ID                  ((UCHAR) 0x02)
#define ANTFS_CONNECT_ID               ((UCHAR) 0x02)
#define ANTFS_DISCONNECT_ID            ((UCHAR) 0x03)
#define ANTFS_AUTHENTICATE_ID          ((UCHAR) 0x04)
#define ANTFS_PING_ID                  ((UCHAR) 0x05)
#define ANTFS_DOWNLOAD_SMALL_ID        ((UCHAR) 0x06)
#define ANTFS_UPLOAD_SMALL_ID          ((UCHAR) 0x07)
#define ANTFS_DOWNLOAD_BIG_ID          ((UCHAR) 0x09)
#define ANTFS_DOWNLOAD_ID              ((UCHAR) 0x09)
#define ANTFS_UPLOAD_BIG_ID            ((UCHAR) 0x0A)
#define ANTFS_UPLOAD_REQUEST_ID        ((UCHAR) 0x0A)
#define ANTFS_ERASE_ID                 ((UCHAR) 0x0B)
#define ANTFS_UPLOAD_DATA_ID           ((UCHAR) 0x0C)
#define ANTFS_SEND_DIRECT_ID           ((UCHAR) 0x0D)

// Responses
#define ANTFS_RESPONSE_AUTH_ID         ((UCHAR) 0x84)
#define ANTFS_RESPONSE_DOWNLOAD_SMALL_ID  ((UCHAR) 0x86)
#define ANTFS_RESPONSE_DOWNLOAD_BIG_ID    ((UCHAR) 0x89)
#define ANTFS_RESPONSE_DOWNLOAD_ID     ((UCHAR) 0x89)
#define ANTFS_RESPONSE_UPLOAD_ID       ((UCHAR) 0x8A)
#define ANTFS_RESPONSE_ERASE_ID        ((UCHAR) 0x8B)
#define ANTFS_RESPONSE_UPLOAD_COMPLETE_ID ((UCHAR) 0x8C)
#define ANTFS_RESPONSE_SEND_DIRECT_ID  ((UCHAR) 0x8D)

// Request
#define ANTFS_REQUEST_SESSION          ((UCHAR) 0x02)

// Message offsets
#define ANTFS_CONNECTION_OFFSET        0
#define ANTFS_COMMAND_OFFSET           1
#define ANTFS_RESPONSE_OFFSET          1

// Beacon definitions
#define STATUS1_OFFSET                 1
#define BEACON_PERIOD_SHIFT            ((UCHAR) 0x00)
#define BEACON_PERIOD_MASK             ((UCHAR) (0x07 << BEACON_PERIOD_SHIFT))
#define BEACON_PERIOD_0_5_HZ           ((UCHAR) (0x00 << BEACON_PERIOD_SHIFT))
#define BEACON_PERIOD_1_HZ             ((UCHAR) (0x01 << BEACON_PERIOD_SHIFT))
#define BEACON_PERIOD_2_HZ             ((UCHAR) (0x02 << BEACON_PERIOD_SHIFT))
#define BEACON_PERIOD_4_HZ             ((UCHAR) (0x03 << BEACON_PERIOD_SHIFT))
#define BEACON_PERIOD_8_HZ             ((UCHAR) (0x04 << BEACON_PERIOD_SHIFT))
#define BEACON_PERIOD_KEEP             ((UCHAR) (0x07 << BEACON_PERIOD_SHIFT))
#define PAIRING_AVAILABLE_FLAG_SHIFT   3
#define PAIRING_AVAILABLE_FLAG_MASK    ((UCHAR) (0x01 << PAIRING_AVAILABLE_FLAG_SHIFT))
#define UPLOAD_ENABLED_FLAG_SHIFT      4
#define UPLOAD_ENABLED_FLAG_MASK       ((UCHAR) (0x01 << UPLOAD_ENABLED_FLAG_SHIFT))
#define DATA_AVAILABLE_FLAG_SHIFT      5
#define DATA_AVAILABLE_FLAG_MASK       ((UCHAR) (0x01 << DATA_AVAILABLE_FLAG_SHIFT))
#define REQUEST_TO_SEND_FLAG_SHIFT     6
#define REQUEST_TO_SEND_FLAG_MASK      ((UCHAR) (0x01 << REQUEST_TO_SEND_FLAG_SHIFT))
#define STATUS2_OFFSET                 2
#define REMOTE_DEVICE_STATE_SHIFT      0
#define REMOTE_DEVICE_STATE_MASK       ((UCHAR) (0x0F << REMOTE_DEVICE_STATE_SHIFT))
#define REMOTE_DEVICE_STATE_LINK       ((UCHAR) 0x00)
#define REMOTE_DEVICE_STATE_AUTH       ((UCHAR) 0x01)
#define REMOTE_DEVICE_STATE_TRANS      ((UCHAR) 0x02)
#define REMOTE_DEVICE_STATE_BUSY       ((UCHAR) 0x03)
#define REMOTE_DEVICE_STATE_LAST_VALID REMOTE_DEVICE_STATE_BUSY
#define REMOTE_DEVICE_BEACON_NOT_FOUND ((UCHAR) 0x80)   //Valid states are in range 0x0F, so this avoids all valid states
#define AUTHENTICATION_TYPE_OFFSET     3
#define DEVICE_TYPE_OFFSET_LOW         4
#define DEVICE_TYPE_OFFSET_HIGH        5
#define MANUFACTURER_ID_OFFSET_LOW     6
#define MANUFACTURER_ID_OFFSET_HIGH    7
#define HOST_ID_OFFSET                 4

// Connect Command
#define TRANSPORT_CHANNEL_FREQ_OFFSET  2
#define TRANSPORT_CHANNEL_PERIOD       3

// Disconnect Command
#define DISCONNECT_COMMAND_TYPE_OFFSET 2
#define DISCONNECT_TIME_DURATION_OFFSET 3
#define DISCONNECT_APP_DURATION_OFFSET 4
#define DISCONNECT_COMMAND_LINK        ((UCHAR) 0x00)
#define DISCONNECT_COMMAND_BROADCAST   ((UCHAR) 0x01)

// Authenticate Command & Auth Types
#define AUTH_COMMAND_TYPE_OFFSET       2
#define AUTH_COMMAND_GOTO_TRANSPORT    ((UCHAR) 0x00)
#define AUTH_COMMAND_REQ_SERIAL_NUM    ((UCHAR) 0x01)
#define AUTH_COMMAND_PAIR              ((UCHAR) 0x02)
#define AUTH_COMMAND_PASSKEY           ((UCHAR) 0x03)
#define AUTH_FRIENDLY_NAME_LENGTH_OFFSET  3
#define AUTH_HOST_SERIAL_NUMBER_OFFSET 4

// Auth Response
#define AUTH_RESPONSE_OFFSET           2
#define AUTH_RESPONSE_NA               ((UCHAR) 0x00)
#define AUTH_RESPONSE_ACCEPT           ((UCHAR) 0x01)
#define AUTH_RESPONSE_REJECT           ((UCHAR) 0x02)
#define AUTH_PASSWORD_LENGTH_OFFSET    3
#define AUTH_REMOTE_SERIAL_NUMBER_OFFSET  4

// Download/Upload/Erase Commands
#define DATA_INDEX_OFFSET              2
#define DATA_OFFSET_SMALL_OFFSET       4
#define MAX_BLOCK_SIZE_SMALL_OFFSET    6
#define BYTES_REMAINING_SMALL_OFFSET   6

// Download
#define DOWNLOAD_DATA_OFFSET_OFFSET    4
#define DOWNLOAD_DATA_INITIAL_OFFSET   1
#define DOWNLOAD_DATA_CRC_OFFSET       2
#define DOWNLOAD_DATA_SIZE_OFFSET      4
#define DOWNLOAD_INITIAL_REQUEST_OFFSET (DOWNLOAD_DATA_INITIAL_OFFSET + 8)
#define DOWNLOAD_CRC_SEED_OFFSET       (DOWNLOAD_DATA_CRC_OFFSET + 8)
#define DOWNLOAD_MAX_BLOCK_SIZE_OFFSET (DOWNLOAD_DATA_SIZE_OFFSET + 8)

// Download Response
#define DOWNLOAD_RESPONSE_OFFSET       2
#define DOWNLOAD_RESPONSE_OK                ((UCHAR) 0x00)
#define DOWNLOAD_RESPONSE_DOES_NOT_EXIST    ((UCHAR) 0x01)
#define DOWNLOAD_RESPONSE_NOT_DOWNLOADABLE  ((UCHAR) 0x02)
#define DOWNLOAD_RESPONSE_NOT_READY         ((UCHAR) 0x03)
#define DOWNLOAD_RESPONSE_REQUEST_INVALID   ((UCHAR) 0x04)
#define DOWNLOAD_RESPONSE_CRC_FAILED        ((UCHAR) 0x05)
#define DOWNLOAD_RESPONSE_BLOCK_SIZE_OFFSET 4
#define DOWNLOAD_RESPONSE_DATA_OFFSET_OFFSET 0 //This is really 8 but is set to 0 because we are reading it from the rx buffer from the second packet of the transfer
#define DOWNLOAD_RESPONSE_FILE_SIZE_OFFSET  4  //This is really 12 but is set to 4 because we are reading it from the rx buffer from the second packet of the transfer
#define DOWNLOAD_RESPONSE_CRC_OFFSET   6

// Upload
#define UPLOAD_MAX_SIZE_OFFSET         4
#define UPLOAD_DATA_OFFSET_OFFSET      12

// Upload Response
#define UPLOAD_RESPONSE_OFFSET         2
#define UPLOAD_RESPONSE_OK                  ((UCHAR) 0x00)
#define UPLOAD_RESPONSE_DOES_NOT_EXIST      ((UCHAR) 0x01)
#define UPLOAD_RESPONSE_NOT_WRITEABLE       ((UCHAR) 0x02)
#define UPLOAD_RESPONSE_INSUFFICIENT_SPACE  ((UCHAR) 0x03)
#define UPLOAD_RESPONSE_REQUEST_INVALID     ((UCHAR) 0x04)
#define UPLOAD_RESPONSE_NOT_READY           ((UCHAR) 0x05)
#define UPLOAD_RESPONSE_LAST_OFFSET_OFFSET  4
#define UPLOAD_RESPONSE_MAX_SIZE_OFFSET     8
#define UPLOAD_RESPONSE_BLOCK_SIZE_OFFSET   12
#define UPLOAD_RESPONSE_CRC_OFFSET          22

// Upload Data
#define UPLOAD_DATA_CRC_SEED_OFFSET    2
#define UPLOAD_DATA_DATA_OFFSET_OFFSET 4
#define UPLOAD_DATA_CRC_OFFSET         6    //This is set to 6 because the CRC starts on byte 6 of the last packet
#define UPLOAD_DATA_MESG_OVERHEAD     16    //Number of bytes of overhead contained in the upload data message

// Erase Response
#define ERASE_RESPONSE_OFFSET          2
#define ERASE_RESPONSE_OK              ((UCHAR) 0x00)
#define ERASE_RESPONSE_REJECT          ((UCHAR) 0x01)

// ANT-FS Request Page
#define REQUEST_COMMAND_TYPE_OFFSET    7
#define REQUEST_PAGE_NUMBER_OFFSET     6
#define REQUEST_TX_RESPONSE_OFFSET     5
#define REQUEST_PAGE_INVALID           ((UCHAR) 0xFF)


// Timeouts are in milliseconds
// NOTE:    Some of these values are runtime-configurable; check with ANTFS_CONFIG_PARAMETERS definition and ANTFSHost::cfgParams.
//          If a value is in ANTFS_CONFIG_PARAMETERS, do not use its value defined here. Instead use the current value in ANTFSHost::cfgParams.
#define MESSAGE_TIMEOUT                ((ULONG) 1000)       // 1 second
#define ACKNOWLEDGED_TIMEOUT           ((ULONG) 10000)      // 10 seconds, 2 seconds to drop to search and 7.5 (max) seconds for transport search timeout.
#define PING_TIMEOUT                   ((ULONG) 3000)
#define DISCONNECT_TIMEOUT             ((ULONG) 3000)
#define CONNECT_TIMEOUT                ((ULONG) 3000)
#define BROADCAST_TIMEOUT              ((ULONG) 3000)
#define BURST_TIMEOUT                  ((ULONG) 100)
#define ANT_CLOSE_TIMEOUT              ((ULONG) 2100)       // Longest channel period + some margin.
#define PAIRING_TIMEOUT                ((ULONG) 60000)      // 60 Seconds.
#define AUTH_TIMEOUT                   ((ULONG) 5000)       // May be too short for some devices
#define ERASE_TIMEOUT                  ((ULONG) 60000)      // 60 seconds.
#define ID_TIMEOUT                     ((ULONG) 5000)       // 5 seconds.
#define DOWNLOAD_RESYNC_TIMEOUT        ((ULONG) 10000)      // 10 seconds.
#define UPLOAD_REQUEST_TIMEOUT         ((ULONG) 3000)       // 3 seconds.  //changed to 3s to be longer than 2s drop to search
#define UPLOAD_RESPONSE_TIMEOUT        ((ULONG) 60000)      // 60 seconds. //allow the device to take up to 60 seconds as long as it's beacon remains busy
#define BURST_CHECK_TIMEOUT            ((ULONG) 10000)      // 10 seconds.
#define ERASE_WAIT_TIMEOUT             ((ULONG) 5000)       // 5 seconds.
#define SEARCH_STATUS_CHECK_TIMEOUT    ((ULONG) 60000)      // 60 seconds.
#define BURST_FAIL_STATUS_CHECK_TIMEOUT ((ULONG) 2500)      // 2.5 seconds.
#define DOWNLOAD_LOOP_TIMEOUT          ((ULONG) 10000)      // 10 seconds.
#define UPLOAD_LOOP_TIMEOUT            ((ULONG) 15000)      // 15 seconds.
#define COMMAND_TIMEOUT                ((ULONG) 60000)      // 60 Seconds, drop to link if no commands received from host
#define ANTFS_SERIAL_WATCHDOG_COUNT    ((USHORT)15)         // 15 seconds
#define CMD_TIMEOUT_DISABLED           ((UCHAR) 0xFF)
#define REQUEST_TIMEOUT                ((ULONG) 3000)       // 3 seconds

#endif // ANTFS_MESSAGES_H