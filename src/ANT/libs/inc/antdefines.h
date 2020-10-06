/*
This software is subject to the license described in the License.txt file
included with this software distribution. You may not use this file except
in compliance with this license.

Copyright (c) Dynastream Innovations Inc. 2016
All rights reserved.
*/
#ifndef ANTDEFINES_H
#define ANTDEFINES_H

#include "types.h"


//////////////////////////////////////////////
// ANT Message Payload Size
//////////////////////////////////////////////
#define ANT_STANDARD_DATA_PAYLOAD_SIZE             ((UCHAR)8)

//////////////////////////////////////////////
// ANT LIBRARY Extended Data Message Fields
// NOTE: You must check the extended message
// bitfield first to find out which fields
// are present before accessing them!
//////////////////////////////////////////////
#define ANT_EXT_MESG_DEVICE_ID_FIELD_SIZE          ((UCHAR)4)
   #define ANT_EXT_STRING_SIZE                     ((UCHAR)27)             // increase buffer size to support longer messages (32 extra bytes after ANT standard payload)

//////////////////////////////////////////////
// ANT Extended Data Message Bifield Definitions
//////////////////////////////////////////////
#define ANT_EXT_MESG_BITFIELD_DEVICE_ID            ((UCHAR)0x80)           // first field after bitfield

// 4 bits free reserved set to 0
#define ANT_EXT_MESG_BIFIELD_EXTENSION             ((UCHAR)0x01)

// extended message input bitfield defines
#define ANT_EXT_MESG_BITFIELD_OVERWRITE_SHARED_ADR ((UCHAR)0x10)
#define ANT_EXT_MESG_BITFIELD_TRANSMISSION_TYPE    ((UCHAR)0x08)


//////////////////////////////////////////////
// ANT Library Config
//////////////////////////////////////////////
#define ANT_LIB_CONFIG_MASK_ALL                    ((UCHAR)0xFF)
#define ANT_LIB_CONFIG_RADIO_CONFIG_ALWAYS         ((UCHAR)0x01)
#define ANT_LIB_CONFIG_MESG_OUT_INC_TIME_STAMP     ((UCHAR)0x20)
#define ANT_LIB_CONFIG_MESG_OUT_INC_RSSI           ((UCHAR)0x40)
#define ANT_LIB_CONFIG_MESG_OUT_INC_DEVICE_ID      ((UCHAR)0x80)

//////////////////////////////////////////////
// ID Definitions
//////////////////////////////////////////////
#define ANT_ID_SIZE                                ((UCHAR)4)
#define ANT_ID_TRANS_TYPE_OFFSET                   ((UCHAR)3)
#define ANT_ID_DEVICE_TYPE_OFFSET                  ((UCHAR)2)
#define ANT_ID_DEVICE_NUMBER_HIGH_OFFSET           ((UCHAR)1)
#define ANT_ID_DEVICE_NUMBER_LOW_OFFSET            ((UCHAR)0)
#define ANT_ID_DEVICE_TYPE_PAIRING_FLAG            ((UCHAR)0x80)

#define ANT_TRANS_TYPE_SHARED_ADDR_MASK            ((UCHAR)0x03)
#define ANT_TRANS_TYPE_1_BYTE_SHARED_ADDRESS       ((UCHAR)0x02)
#define ANT_TRANS_TYPE_2_BYTE_SHARED_ADDRESS       ((UCHAR)0x03)


//////////////////////////////////////////////
// Assign Channel Parameters
//////////////////////////////////////////////
#define PARAMETER_RX_NOT_TX                        ((UCHAR)0x00)
#define PARAMETER_TX_NOT_RX                        ((UCHAR)0x10)
#define PARAMETER_SHARED_CHANNEL                   ((UCHAR)0x20)
#define PARAMETER_NO_TX_GUARD_BAND                 ((UCHAR)0x40)
#define PARAMETER_ALWAYS_RX_WILD_CARD_SEARCH_ID    ((UCHAR)0x40)                 //Pre-AP2
#define PARAMETER_RX_ONLY                          ((UCHAR)0x40)

//////////////////////////////////////////////
// Ext. Assign Channel Parameters
//////////////////////////////////////////////
#define EXT_PARAM_ALWAYS_SEARCH                    ((UCHAR)0x01)
#define EXT_PARAM_FREQUENCY_AGILITY                ((UCHAR)0x04)

//////////////////////////////////////////////
// Radio TX Power Definitions
//////////////////////////////////////////////
#define RADIO_TX_POWER_LVL_MASK                    ((UCHAR)0x03)

#define RADIO_TX_POWER_LVL_0                       ((UCHAR)0x00)                //(formerly: RADIO_TX_POWER_MINUS20DB) lowest
#define RADIO_TX_POWER_LVL_1                       ((UCHAR)0x01)                //(formerly: RADIO_TX_POWER_MINUS10DB)
#define RADIO_TX_POWER_LVL_2                       ((UCHAR)0x02)                //(formerly: RADIO_TX_POWER_MINUS5DB)
#define RADIO_TX_POWER_LVL_3                       ((UCHAR)0x03)                //(formerly: RADIO_TX_POWER_0DB) highest

//////////////////////////////////////////////
// Channel Status
//////////////////////////////////////////////
#define STATUS_CHANNEL_STATE_MASK                  ((UCHAR)0x03)
#define STATUS_UNASSIGNED_CHANNEL                  ((UCHAR)0x00)
#define STATUS_ASSIGNED_CHANNEL                    ((UCHAR)0x01)
#define STATUS_SEARCHING_CHANNEL                   ((UCHAR)0x02)
#define STATUS_TRACKING_CHANNEL                    ((UCHAR)0x03)

//////////////////////////////////////////////
// Standard capabilities defines
//////////////////////////////////////////////
#define CAPABILITIES_NO_RX_CHANNELS                ((UCHAR)0x01)
#define CAPABILITIES_NO_TX_CHANNELS                ((UCHAR)0x02)
#define CAPABILITIES_NO_RX_MESSAGES                ((UCHAR)0x04)
#define CAPABILITIES_NO_TX_MESSAGES                ((UCHAR)0x08)
#define CAPABILITIES_NO_ACKD_MESSAGES              ((UCHAR)0x10)
#define CAPABILITIES_NO_BURST_TRANSFER             ((UCHAR)0x20)

//////////////////////////////////////////////
// Advanced capabilities defines
//////////////////////////////////////////////
#define CAPABILITIES_OVERUN_UNDERRUN               ((UCHAR)0x01)     // Support for this functionality has been dropped
#define CAPABILITIES_NETWORK_ENABLED               ((UCHAR)0x02)
#define CAPABILITIES_AP1_VERSION_2                 ((UCHAR)0x04)     // This Version of the AP1 does not support transmit and only had a limited release
#define CAPABILITIES_SERIAL_NUMBER_ENABLED         ((UCHAR)0x08)
#define CAPABILITIES_PER_CHANNEL_TX_POWER_ENABLED  ((UCHAR)0x10)
#define CAPABILITIES_LOW_PRIORITY_SEARCH_ENABLED   ((UCHAR)0x20)
#define CAPABILITIES_SCRIPT_ENABLED                ((UCHAR)0x40)
#define CAPABILITIES_SEARCH_LIST_ENABLED           ((UCHAR)0x80)

//////////////////////////////////////////////
// Advanced capabilities 2 defines
//////////////////////////////////////////////
#define CAPABILITIES_LED_ENABLED                   ((UCHAR)0x01)
#define CAPABILITIES_EXT_MESSAGE_ENABLED           ((UCHAR)0x02)
#define CAPABILITIES_SCAN_MODE_ENABLED             ((UCHAR)0x04)
#define CAPABILITIES_RESERVED                      ((UCHAR)0x08)
#define CAPABILITIES_PROX_SEARCH_ENABLED           ((UCHAR)0x10)
#define CAPABILITIES_EXT_ASSIGN_ENABLED            ((UCHAR)0x20)
#define CAPABILITIES_FS_ANTFS_ENABLED              ((UCHAR)0x40)
#define CAPABILITIES_FIT1_ENABLED                  ((UCHAR)0x80)

//////////////////////////////////////////////
// Advanced capabilities 3 defines
//////////////////////////////////////////////
#define CAPABILITIES_ADVANCED_BURST_ENABLED              ((UCHAR)0x01)
#define CAPABILITIES_EVENT_BUFFERING_ENABLED             ((UCHAR)0x02)
#define CAPABILITIES_EVENT_FILTERING_ENABLED             ((UCHAR)0x04)
#define CAPABILITIES_HIGH_DUTY_SEARCH_MODE_ENABLED       ((UCHAR)0x08)
#define CAPABILITIES_ACTIVE_SEARCH_SHARING_MODE_ENABLED  ((UCHAR)0x10)
#define CAPABILITIES_SELECTIVE_DATA_UPDATE_ENABLED       ((UCHAR)0x40)
#define CAPABILITIES_ENCRYPTED_CHANNEL_ENABLED           ((UCHAR)0x80)


//////////////////////////////////////////////
// Burst Message Sequence
//////////////////////////////////////////////
#define CHANNEL_NUMBER_MASK                        ((UCHAR)0x1F)
#define SEQUENCE_NUMBER_MASK                       ((UCHAR)0xE0)
#define SEQUENCE_NUMBER_ROLLOVER                   ((UCHAR)0x60)
#define SEQUENCE_FIRST_MESSAGE                     ((UCHAR)0x00)
#define SEQUENCE_LAST_MESSAGE                      ((UCHAR)0x80)
#define SEQUENCE_NUMBER_INC                        ((UCHAR)0x20)

//////////////////////////////////////////////
// Advanced Burst Config defines
//////////////////////////////////////////////
#define ADV_BURST_CONFIG_FREQ_HOP                  ((ULONG)0x00000001)

//////////////////////////////////////////////
// Extended Message ID Mask
//////////////////////////////////////////////
#define MSG_EXT_ID_MASK                            ((UCHAR)0xE0)

//////////////////////////////////////////////
// Control Message Flags
//////////////////////////////////////////////
#define BROADCAST_CONTROL_BYTE                     ((UCHAR)0x00)
#define ACKNOWLEDGED_CONTROL_BYTE                  ((UCHAR)0xA0)

//////////////////////////////////////////////
// Response / Event Codes
//////////////////////////////////////////////
#define RESPONSE_NO_ERROR                          ((UCHAR)0x00)
#define NO_EVENT                                   ((UCHAR)0x00)

#define EVENT_RX_SEARCH_TIMEOUT                    ((UCHAR)0x01)
#define EVENT_RX_FAIL                              ((UCHAR)0x02)
#define EVENT_TX                                   ((UCHAR)0x03)
#define EVENT_TRANSFER_RX_FAILED                   ((UCHAR)0x04)
#define EVENT_TRANSFER_TX_COMPLETED                ((UCHAR)0x05)
#define EVENT_TRANSFER_TX_FAILED                   ((UCHAR)0x06)
#define EVENT_CHANNEL_CLOSED                       ((UCHAR)0x07)
#define EVENT_RX_FAIL_GO_TO_SEARCH                 ((UCHAR)0x08)
#define EVENT_CHANNEL_COLLISION                    ((UCHAR)0x09)
#define EVENT_TRANSFER_TX_START                    ((UCHAR)0x0A)           // a pending transmit transfer has begun

#define EVENT_CHANNEL_ACTIVE                       ((UCHAR)0x0F)

#define EVENT_TRANSFER_TX_NEXT_MESSAGE             ((UCHAR)0x11)           // only enabled in FIT1

#define CHANNEL_IN_WRONG_STATE                     ((UCHAR)0x15)           // returned on attempt to perform an action from the wrong channel state
#define CHANNEL_NOT_OPENED                         ((UCHAR)0x16)           // returned on attempt to communicate on a channel that is not open
#define CHANNEL_ID_NOT_SET                         ((UCHAR)0x18)           // returned on attempt to open a channel without setting the channel ID
#define CLOSE_ALL_CHANNELS                         ((UCHAR)0x19)           // returned when attempting to start scanning mode, when channels are still open

#define TRANSFER_IN_PROGRESS                       ((UCHAR)0x1F)           // returned on attempt to communicate on a channel with a TX transfer in progress
#define TRANSFER_SEQUENCE_NUMBER_ERROR             ((UCHAR)0x20)           // returned when sequence number is out of order on a Burst transfer
#define TRANSFER_IN_ERROR                          ((UCHAR)0x21)
#define TRANSFER_BUSY                              ((UCHAR)0x22)

#define INVALID_MESSAGE_CRC                        ((UCHAR)0x26)           // returned if there is a framing error on an incomming message
#define MESSAGE_SIZE_EXCEEDS_LIMIT                 ((UCHAR)0x27)           // returned if a data message is provided that is too large
#define INVALID_MESSAGE                            ((UCHAR)0x28)           // returned when the message has an invalid parameter
#define INVALID_NETWORK_NUMBER                     ((UCHAR)0x29)           // returned when an invalid network number is provided
#define INVALID_LIST_ID                            ((UCHAR)0x30)           // returned when the provided list ID or size exceeds the limit
#define INVALID_SCAN_TX_CHANNEL                    ((UCHAR)0x31)           // returned when attempting to transmit on channel 0 when in scan mode
#define INVALID_PARAMETER_PROVIDED                 ((UCHAR)0x33)           // returned when an invalid parameter is specified in a configuration message

#define EVENT_SERIAL_QUE_OVERFLOW                  ((UCHAR)0x34)
#define EVENT_QUE_OVERFLOW                         ((UCHAR)0x35)           // ANT event que has overflowed and lost 1 or more events

#define EVENT_CLK_ERROR                            ((UCHAR)0x36)           // debug event for XOSC16M on LE1
#define EVENT_STATE_OVERRUN                        ((UCHAR)0x37)

#define EVENT_ENCRYPT_NEGOTIATION_SUCCESS          ((UCHAR)0x38)           // ANT stack generated event when connecting to an encrypted channel has succeeded
#define EVENT_ENCRYPT_NEGOTIATION_FAIL             ((UCHAR)0x39)           // ANT stack generated event when connecting to an encrypted channel has failed

#define SCRIPT_FULL_ERROR                          ((UCHAR)0x40)           // error writing to script, memory is full
#define SCRIPT_WRITE_ERROR                         ((UCHAR)0x41)           // error writing to script, bytes not written correctly
#define SCRIPT_INVALID_PAGE_ERROR                  ((UCHAR)0x42)           // error accessing script page
#define SCRIPT_LOCKED_ERROR                        ((UCHAR)0x43)           // the scripts are locked and can't be dumped

#define NO_RESPONSE_MESSAGE                        ((UCHAR)0x50)           // returned to the Command_SerialMessageProcess function, so no reply message is generated
#define RETURN_TO_MFG                              ((UCHAR)0x51)           // default return to any mesg when the module determines that the mfg procedure has not been fully completed

#define FIT_ACTIVE_SEARCH_TIMEOUT                  ((UCHAR)0x60)           // Fit1 only event added for timeout of the pairing state after the Fit module becomes active
#define FIT_WATCH_PAIR                             ((UCHAR)0x61)           // Fit1 only
#define FIT_WATCH_UNPAIR                           ((UCHAR)0x62)           // Fit1 only

#define USB_STRING_WRITE_FAIL                      ((UCHAR)0x70)

// Internal only events below this point
#define INTERNAL_ONLY_EVENTS                       ((UCHAR)0x80)
#define EVENT_RX                                   ((UCHAR)0x80)           // INTERNAL: Event for a receive message
#define EVENT_NEW_CHANNEL                          ((UCHAR)0x81)           // INTERNAL: EVENT for a new active channel
#define EVENT_PASS_THRU                            ((UCHAR)0x82)           // INTERNAL: Event to allow an upper stack events to pass through lower stacks

#define EVENT_BLOCKED                              ((UCHAR)0xFF)           // INTERNAL: Event to replace any event we do not wish to go out, will also zero the size of the Tx message

///////////////////////////////////////////////////////
// Script Command Codes
///////////////////////////////////////////////////////
#define SCRIPT_CMD_FORMAT                          ((UCHAR)0x00)
#define SCRIPT_CMD_DUMP                            ((UCHAR)0x01)
#define SCRIPT_CMD_SET_DEFAULT_SECTOR              ((UCHAR)0x02)
#define SCRIPT_CMD_END_SECTOR                      ((UCHAR)0x03)
#define SCRIPT_CMD_END_DUMP                        ((UCHAR)0x04)
#define SCRIPT_CMD_LOCK                            ((UCHAR)0x05)

///////////////////////////////////////////////////////
// USB Descriptor Strings
///////////////////////////////////////////////////////
#define USB_DESCRIPTOR_VID_PID                     ((UCHAR) 0x00)
#define USB_DESCRIPTOR_MANUFACTURER_STRING         ((UCHAR) 0x01)
#define USB_DESCRIPTOR_DEVICE_STRING               ((UCHAR) 0x02)
#define USB_DESCRIPTOR_SERIAL_STRING               ((UCHAR) 0x03)

///////////////////////////////////////////////////////
// Reset Mesg Codes
///////////////////////////////////////////////////////
#define RESET_FLAGS_MASK                           ((UCHAR)0xE0)
#define RESET_SUSPEND                              ((UCHAR)0x80)              // this must follow bitfield def
#define RESET_SYNC                                 ((UCHAR)0x40)              // this must follow bitfield def
#define RESET_CMD                                  ((UCHAR)0x20)              // this must follow bitfield def
#define RESET_WDT                                  ((UCHAR)0x02)
#define RESET_RST                                  ((UCHAR)0x01)
#define RESET_POR                                  ((UCHAR)0x00)

//////////////////////////////////////////////
// PC Application Event Codes
/////////////////////////////////////// ///////
//NOTE: These events are not generated by the embedded ANT module

#define EVENT_RX_BROADCAST                         ((UCHAR)0x9A)           // returned when module receives broadcast data
#define EVENT_RX_ACKNOWLEDGED                      ((UCHAR)0x9B)           // returned when module receives acknowledged data
#define EVENT_RX_BURST_PACKET                      ((UCHAR)0x9C)           // returned when module receives burst data

#define EVENT_RX_EXT_BROADCAST                     ((UCHAR)0x9D)           // returned when module receives broadcast data
#define EVENT_RX_EXT_ACKNOWLEDGED                  ((UCHAR)0x9E)           // returned when module receives acknowledged data
#define EVENT_RX_EXT_BURST_PACKET                  ((UCHAR)0x9F)           // returned when module receives burst data

#define EVENT_RX_RSSI_BROADCAST                    ((UCHAR)0xA0)           // returned when module receives broadcast data
#define EVENT_RX_RSSI_ACKNOWLEDGED                 ((UCHAR)0xA1)           // returned when module receives acknowledged data
#define EVENT_RX_RSSI_BURST_PACKET                 ((UCHAR)0xA2)           // returned when module receives burst data

#define EVENT_RX_FLAG_BROADCAST                    ((UCHAR)0xA3)          // returned when module receives broadcast data with flag attached
#define EVENT_RX_FLAG_ACKNOWLEDGED                 ((UCHAR)0xA4)          // returned when module receives acknowledged data with flag attached
#define EVENT_RX_FLAG_BURST_PACKET                 ((UCHAR)0xA5)          // returned when module receives burst data with flag attached

//////////////////////////////////////////////
// Integrated ANT-FS Client Response/Event Codes
//////////////////////////////////////////////
#define MESG_FS_ANTFS_EVENT_PAIR_REQUEST           ((UCHAR)0x01)
#define MESG_FS_ANTFS_EVENT_DOWNLOAD_START         ((UCHAR)0x02)
#define MESG_FS_ANTFS_EVENT_UPLOAD_START           ((UCHAR)0x03)
#define MESG_FS_ANTFS_EVENT_DOWNLOAD_COMPLETE      ((UCHAR)0x04)
#define MESG_FS_ANTFS_EVENT_UPLOAD_COMPLETE        ((UCHAR)0x05)
#define MESG_FS_ANTFS_EVENT_ERASE_COMPLETE         ((UCHAR)0x06)
#define MESG_FS_ANTFS_EVENT_LINK_STATE             ((UCHAR)0x07)
#define MESG_FS_ANTFS_EVENT_AUTH_STATE             ((UCHAR)0x08)
#define MESG_FS_ANTFS_EVENT_TRANSPORT_STATE        ((UCHAR)0x09)
#define MESG_FS_ANTFS_EVENT_CMD_RECEIVED           ((UCHAR)0x0A)
#define MESG_FS_ANTFS_EVENT_CMD_PROCESSED          ((UCHAR)0x0B)

#define FS_NO_ERROR_RESPONSE                          ((UCHAR) 0x00)
#define FS_MEMORY_UNFORMATTED_ERROR_RESPONSE          ((UCHAR) 0x01)
#define FS_MEMORY_NO_FREE_SECTORS_ERROR_RESPONSE      ((UCHAR) 0x02)
#define FS_MEMORY_READ_ERROR_RESPONSE                 ((UCHAR) 0x03)
#define FS_MEMORY_WRITE_ERROR_RESPONSE                ((UCHAR) 0x04)
#define FS_MEMORY_ERASE_ERROR_RESPONSE                ((UCHAR) 0x05)
#define FS_TOO_MANY_FILES_OPEN_RESPONSE               ((UCHAR) 0x06)
#define FS_FILE_INDEX_INVALID_ERROR_RESPONSE          ((UCHAR) 0x07)
#define FS_FILE_INDEX_EXISTS_ERROR_RESPONSE           ((UCHAR) 0x08)
#define FS_AUTO_INDEX_FAILED_TRY_AGAIN_ERROR_RESPONSE ((UCHAR) 0x09)
#define FS_FILE_ALREADY_OPEN_ERROR_RESPONSE           ((UCHAR) 0x0A)
#define FS_FILE_NOT_OPEN_ERROR_RESPONSE               ((UCHAR) 0x0B)
#define FS_DIR_CORRUPTED_ERROR_RESPONSE               ((UCHAR) 0x0C)
#define FS_INVALID_OFFSET_ERROR_RESPONSE              ((UCHAR) 0x0D)
#define FS_BAD_PERMISSIONS_ERROR_RESPONSE             ((UCHAR) 0x0E)
#define FS_EOF_REACHED_ERROR_RESPONSE                 ((UCHAR) 0x0F)
#define FS_INVALID_FILE_HANDLE_ERROR_RESPONSE         ((UCHAR) 0x10)

#define FS_CRYPTO_OPEN_PERMISSION_ERROR_RESPONSE   ((UCHAR) 0x32)
#define FS_CRYPTO_HANDLE_ALREADY_IN_USE_RESPONSE   ((UCHAR) 0x33)
#define FS_CRYPTO_USER_KEY_NOT_SPECIFIED_RESPONSE  ((UCHAR) 0x34)
#define FS_CRYPTO_USER_KEY_ADD_ERROR_RESPONSE      ((UCHAR) 0x35)
#define FS_CRYPTO_USER_KEY_FETCH_ERROR_RESPONSE    ((UCHAR) 0x36)
#define FS_CRYPTO_IVNONE_READ_ERROR_RESPONSE       ((UCHAR) 0x37)
#define FS_CRYPTO_BLOCK_OFFSET_ERROR_RESPONSE      ((UCHAR) 0x38)
#define FS_CRYPTO_BLOCK_SIZE_ERROR_RESPONSE        ((UCHAR) 0x39)
#define FS_CRYPTO_CFG_TYPE_NOT_SUPPORTED_RESPONSE  ((UCHAR) 0x40)

#define FS_FIT_FILE_HEADER_ERROR_RESPONSE             ((UCHAR) 0x64)
#define FS_FIT_FILE_SIZE_INTEGRITY_ERROR_RESPONSE     ((UCHAR) 0x65)
#define FS_FIT_FILE_CRC_ERROR_RESPONSE                ((UCHAR) 0x66)
#define FS_FIT_FILE_CHECK_PERMISSION_ERROR_RESPONSE   ((UCHAR) 0x67)
#define FS_FIT_FILE_CHECK_FILE_TYPE_ERROR_RESPONSE    ((UCHAR) 0x68)
#define FS_FIT_FILE_OP_ABORT_ERROR_RESPONSE           ((UCHAR) 0x69)

//////////////////////////////////////////////
// ANT EVENT return structure
//////////////////////////////////////////////
typedef struct
{
   UCHAR ucChannel;
   UCHAR ucEvent;

} ANT_EVENT;

#endif // !ANTDEFINES_H


