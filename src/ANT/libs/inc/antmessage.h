/*
This software is subject to the license described in the License.txt file
included with this software distribution. You may not use this file except
in compliance with this license.

Copyright (c) Dynastream Innovations Inc. 2016
All rights reserved.
*/
#ifndef ANTMESSAGE_H
#define ANTMESSAGE_H

#include "types.h"
#include "antdefines.h"

/////////////////////////////////////////////////////////////////////////////
// Message Format
// Messages are in the format:
//
// AX XX YY -------- CK
//
// where: AX    is the 1 byte sync byte either transmit or recieve
//        XX    is the 1 byte size of the message (0-249) NOTE: THIS WILL BE LIMITED BY THE EMBEDDED RECEIVE BUFFER SIZE
//        YY    is the 1 byte ID of the message (1-255, 0 is invalid)
//        ----- is the data of the message (0-249 bytes of data)
//        CK    is the 1 byte Checksum of the message
/////////////////////////////////////////////////////////////////////////////
#define MESG_TX_SYNC                         ((UCHAR)0xA4)
#define MESG_RX_SYNC                         ((UCHAR)0xA5)
#define MESG_SYNC_SIZE                       ((UCHAR)1)
#define MESG_SIZE_SIZE                       ((UCHAR)1)
#define MESG_ID_SIZE                         ((UCHAR)1)
#define MESG_CHANNEL_NUM_SIZE                ((UCHAR)1)
#define MESG_EXT_MESG_BF_SIZE                ((UCHAR)1)  // NOTE: this could increase in the future
#define MESG_CHECKSUM_SIZE                   ((UCHAR)1)
#define MESG_DATA_SIZE                       ((UCHAR)9)

// The largest serial message is an ANT data message with all of the extended fields
#define MESG_ANT_MAX_PAYLOAD_SIZE            ANT_STANDARD_DATA_PAYLOAD_SIZE

#define MESG_MAX_EXT_DATA_SIZE               (ANT_EXT_MESG_DEVICE_ID_FIELD_SIZE + ANT_EXT_STRING_SIZE) // ANT device ID (4 bytes) +  Padding for ANT EXT string size(27 bytes)

#define MESG_MAX_DATA_SIZE                   (MESG_ANT_MAX_PAYLOAD_SIZE + MESG_EXT_MESG_BF_SIZE + MESG_MAX_EXT_DATA_SIZE) // ANT data payload (8 bytes) + extended bitfield (1 byte) + extended data (31 bytes) = 40 bytes
#define MESG_MAX_SIZE_VALUE                  (MESG_MAX_DATA_SIZE + MESG_CHANNEL_NUM_SIZE)  // this is the maximum value that the serial message size value is allowed to be (40 + 1 = 41 bytes)
#define MESG_BUFFER_SIZE                     (MESG_SIZE_SIZE + MESG_ID_SIZE + MESG_CHANNEL_NUM_SIZE + MESG_MAX_DATA_SIZE + MESG_CHECKSUM_SIZE)
#define MESG_FRAMED_SIZE                     (MESG_ID_SIZE + MESG_CHANNEL_NUM_SIZE + MESG_MAX_DATA_SIZE)
#define MESG_HEADER_SIZE                     (MESG_SYNC_SIZE + MESG_SIZE_SIZE + MESG_ID_SIZE)
#define MESG_FRAME_SIZE                      (MESG_HEADER_SIZE + MESG_CHECKSUM_SIZE)
#define MESG_MAX_SIZE                        (MESG_MAX_DATA_SIZE + MESG_FRAME_SIZE)

#define MESG_SIZE_OFFSET                     (MESG_SYNC_SIZE)
#define MESG_ID_OFFSET                       (MESG_SYNC_SIZE + MESG_SIZE_SIZE)
#define MESG_DATA_OFFSET                     (MESG_HEADER_SIZE)
#define MESG_RECOMMENDED_BUFFER_SIZE         ((UCHAR) 64)                         // This is the recommended size for serial message buffers if there are no RAM restrictions on the system

//////////////////////////////////////////////
// Message ID's
//////////////////////////////////////////////
#define MESG_INVALID_ID                      ((UCHAR)0x00)
#define MESG_EVENT_ID                        ((UCHAR)0x01)

#define MESG_VERSION_ID                      ((UCHAR)0x3E)
#define MESG_RESPONSE_EVENT_ID               ((UCHAR)0x40)

#define MESG_UNASSIGN_CHANNEL_ID             ((UCHAR)0x41)
#define MESG_ASSIGN_CHANNEL_ID               ((UCHAR)0x42)
#define MESG_CHANNEL_MESG_PERIOD_ID          ((UCHAR)0x43)
#define MESG_CHANNEL_SEARCH_TIMEOUT_ID       ((UCHAR)0x44)
#define MESG_CHANNEL_RADIO_FREQ_ID           ((UCHAR)0x45)
#define MESG_NETWORK_KEY_ID                  ((UCHAR)0x46)
#define MESG_RADIO_TX_POWER_ID               ((UCHAR)0x47)
#define MESG_RADIO_CW_MODE_ID                ((UCHAR)0x48)
#define MESG_SYSTEM_RESET_ID                 ((UCHAR)0x4A)
#define MESG_OPEN_CHANNEL_ID                 ((UCHAR)0x4B)
#define MESG_CLOSE_CHANNEL_ID                ((UCHAR)0x4C)
#define MESG_REQUEST_ID                      ((UCHAR)0x4D)

#define MESG_BROADCAST_DATA_ID               ((UCHAR)0x4E)
#define MESG_ACKNOWLEDGED_DATA_ID            ((UCHAR)0x4F)
#define MESG_BURST_DATA_ID                   ((UCHAR)0x50)

#define MESG_CHANNEL_ID_ID                   ((UCHAR)0x51)
#define MESG_CHANNEL_STATUS_ID               ((UCHAR)0x52)
#define MESG_RADIO_CW_INIT_ID                ((UCHAR)0x53)
#define MESG_CAPABILITIES_ID                 ((UCHAR)0x54)

#define MESG_STACKLIMIT_ID                   ((UCHAR)0x55)

#define MESG_SCRIPT_DATA_ID                  ((UCHAR)0x56)
#define MESG_SCRIPT_CMD_ID                   ((UCHAR)0x57)

#define MESG_ID_LIST_ADD_ID                  ((UCHAR)0x59)
#define MESG_CRYPTO_ID_LIST_ADD_ID           ((UCHAR)0x59)
#define MESG_ID_LIST_CONFIG_ID               ((UCHAR)0x5A)
#define MESG_CRYPTO_ID_LIST_CONFIG_ID        ((UCHAR)0x5A)
#define MESG_OPEN_RX_SCAN_ID                 ((UCHAR)0x5B)

#define MESG_EXT_CHANNEL_RADIO_FREQ_ID       ((UCHAR)0x5C)  // OBSOLETE: (for 905 radio)
#define MESG_EXT_BROADCAST_DATA_ID           ((UCHAR)0x5D)
#define MESG_EXT_ACKNOWLEDGED_DATA_ID        ((UCHAR)0x5E)
#define MESG_EXT_BURST_DATA_ID               ((UCHAR)0x5F)

#define MESG_CHANNEL_RADIO_TX_POWER_ID       ((UCHAR)0x60)
#define MESG_GET_SERIAL_NUM_ID               ((UCHAR)0x61)
#define MESG_GET_TEMP_CAL_ID                 ((UCHAR)0x62)
#define MESG_SET_LP_SEARCH_TIMEOUT_ID        ((UCHAR)0x63)
#define MESG_SET_TX_SEARCH_ON_NEXT_ID        ((UCHAR)0x64)
#define MESG_SERIAL_NUM_SET_CHANNEL_ID_ID    ((UCHAR)0x65)
#define MESG_RX_EXT_MESGS_ENABLE_ID          ((UCHAR)0x66)
#define MESG_RADIO_CONFIG_ALWAYS_ID          ((UCHAR)0x67)
#define MESG_ENABLE_LED_FLASH_ID             ((UCHAR)0x68)
#define MESG_XTAL_ENABLE_ID                  ((UCHAR)0x6D)
#define MESG_ANTLIB_CONFIG_ID                ((UCHAR)0x6E)
#define MESG_STARTUP_MESG_ID                 ((UCHAR)0x6F)
#define MESG_AUTO_FREQ_CONFIG_ID             ((UCHAR)0x70)
#define MESG_PROX_SEARCH_CONFIG_ID           ((UCHAR)0x71)

#define MESG_ADV_BURST_DATA_ID               ((UCHAR)0x72)
#define MESG_EVENT_BUFFERING_CONFIG_ID       ((UCHAR)0x74)

#define MESG_SET_SEARCH_CH_PRIORITY_ID       ((UCHAR)0x75)

#define MESG_HIGH_DUTY_SEARCH_MODE_ID        ((UCHAR)0x77)
#define MESG_CONFIG_ADV_BURST_ID             ((UCHAR)0x78)
#define MESG_EVENT_FILTER_CONFIG_ID          ((UCHAR)0x79)
#define MESG_SDU_CONFIG_ID                   ((UCHAR)0x7A)
#define MESG_SDU_SET_MASK_ID                 ((UCHAR)0x7B)
#define MESG_USER_CONFIG_PAGE_ID             ((UCHAR)0x7C)
#define MESG_ENCRYPT_ENABLE_ID               ((UCHAR)0x7D)
#define MESG_SET_CRYPTO_KEY_ID               ((UCHAR)0x7E)
#define MESG_SET_CRYPTO_INFO_ID              ((UCHAR)0x7F)
#define MESG_CUBE_CMD_ID                     ((UCHAR)0x80)

#define MESG_ACTIVE_SEARCH_SHARING_ID        ((UCHAR)0x81)
#define MESG_NVM_CRYPTO_KEY_OPS_ID           ((UCHAR)0x83)

#define MESG_GET_PIN_DIODE_CONTROL_ID        ((UCHAR)0x8D)
#define MESG_PIN_DIODE_CONTROL_ID            ((UCHAR)0x8E)
#define MESG_FIT1_SET_AGC_ID                 ((UCHAR)0x8F)

#define MESG_FIT1_SET_EQUIP_STATE_ID         ((UCHAR)0x91)  // *** CONFLICT: w/ Sensrcore, Fit1 will never have sensrcore enabled

// Sensrcore Messages
#define MESG_SET_CHANNEL_INPUT_MASK_ID       ((UCHAR)0x90)
#define MESG_SET_CHANNEL_DATA_TYPE_ID        ((UCHAR)0x91)
#define MESG_READ_PINS_FOR_SECT_ID           ((UCHAR)0x92)
#define MESG_TIMER_SELECT_ID                 ((UCHAR)0x93)
#define MESG_ATOD_SETTINGS_ID                ((UCHAR)0x94)
#define MESG_SET_SHARED_ADDRESS_ID           ((UCHAR)0x95)
#define MESG_ATOD_EXTERNAL_ENABLE_ID         ((UCHAR)0x96)
#define MESG_ATOD_PIN_SETUP_ID               ((UCHAR)0x97)
#define MESG_SETUP_ALARM_ID                  ((UCHAR)0x98)
#define MESG_ALARM_VARIABLE_MODIFY_TEST_ID   ((UCHAR)0x99)
#define MESG_PARTIAL_RESET_ID                ((UCHAR)0x9A)
#define MESG_OVERWRITE_TEMP_CAL_ID           ((UCHAR)0x9B)
#define MESG_SERIAL_PASSTHRU_SETTINGS_ID     ((UCHAR)0x9C)

#define MESG_BIST_ID                         ((UCHAR)0xAA)
#define MESG_UNLOCK_INTERFACE_ID             ((UCHAR)0xAD)
#define MESG_SERIAL_ERROR_ID                 ((UCHAR)0xAE)
#define MESG_SET_ID_STRING_ID                ((UCHAR)0xAF)

#define MESG_PORT_GET_IO_STATE_ID            ((UCHAR)0xB4)
#define MESG_PORT_SET_IO_STATE_ID            ((UCHAR)0xB5)

#define MESG_RSSI_ID                         ((UCHAR)0xC0)
#define MESG_RSSI_BROADCAST_DATA_ID          ((UCHAR)0xC1)
#define MESG_RSSI_ACKNOWLEDGED_DATA_ID       ((UCHAR)0xC2)
#define MESG_RSSI_BURST_DATA_ID              ((UCHAR)0xC3)
#define MESG_RSSI_SEARCH_THRESHOLD_ID        ((UCHAR)0xC4)
#define MESG_SLEEP_ID                        ((UCHAR)0xC5)
#define MESG_GET_GRMN_ESN_ID                 ((UCHAR)0xC6)
#define MESG_SET_USB_INFO_ID                 ((UCHAR)0xC7)

#define MESG_HCI_COMMAND_COMPLETE            ((UCHAR)0xC8)

// 0xE0 - 0xEF reserved for extended ID
#define MESG_EXT_ID_0                        ((UCHAR)0xE0)
#define MESG_EXT_ID_1                        ((UCHAR)0xE1)
#define MESG_EXT_ID_2                        ((UCHAR)0xE2)

// 0xE0 extended IDs
#define MESG_EXT_RESPONSE_ID                 ((USHORT)0xE000)

// 0xE1 extended IDs
#define MESG_EXT_REQUEST_ID                  ((USHORT)0xE100)

// 0xE2 extended IDs
#define MESG_FS_INIT_MEMORY_ID               ((USHORT)0xE200)
#define MESG_FS_FORMAT_MEMORY_ID             ((USHORT)0xE201)
#define MESG_FS_GET_USED_SPACE_ID            ((USHORT)0xE202)
#define MESG_FS_GET_FREE_SPACE_ID            ((USHORT)0xE203)
#define MESG_FS_FIND_FILE_INDEX_ID           ((USHORT)0xE204)
#define MESG_FS_DIRECTORY_READ_ABSOLUTE_ID   ((USHORT)0xE205)
#define MESG_FS_DIRECTORY_READ_ENTRY_ID      ((USHORT)0xE206)
#define MESG_FS_DIRECTORY_SAVE_ID            ((USHORT)0xE207)
#define MESG_FS_DIRECTORY_GET_SIZE_ID        ((USHORT)0xE208)
#define MESG_FS_DIRECTORY_REBUILD_ID         ((USHORT)0xE209)
#define MESG_FS_FILE_CREATE_ID               ((USHORT)0xE20A)
#define MESG_FS_FILE_OPEN_ID                 ((USHORT)0xE20B)
#define MESG_FS_FILE_DELETE_ID               ((USHORT)0xE20C)
#define MESG_FS_FILE_CLOSE_ID                ((USHORT)0xE20D)
#define MESG_FS_FILE_READ_ABSOLUTE_ID        ((USHORT)0xE20E)
#define MESG_FS_FILE_READ_RELATIVE_ID        ((USHORT)0xE20F)
#define MESG_FS_FILE_WRITE_ABSOLUTE_ID       ((USHORT)0xE210)
#define MESG_FS_FILE_WRITE_RELATIVE_ID       ((USHORT)0xE211)
#define MESG_FS_FILE_SET_SPECIFIC_FLAGS_ID   ((USHORT)0xE212)
#define MESG_FS_FILE_GET_SIZE_ID             ((USHORT)0xE213)
#define MESG_FS_FILE_GET_SPECIFIC_FILE_FLAGS_ID ((USHORT)0xE214)
#define MESG_FS_FILE_GET_SIZE_IN_MEM_ID      ((USHORT)0xE215)
#define MESG_FS_DIRECTORY_READ_LOCK_ID       ((USHORT)0xE216)

#define MESG_FS_FILE_SET_GENERAL_FLAGS_ID    ((USHORT)0xE21E)
#define MESG_FS_DIRECTORY_WRITE_ABSOLUTE_ID  ((USHORT)0xE21F)
// reserved
#define MESG_MEMDEV_EEPROM_INIT_ID           ((USHORT)0xE220)
#define MESG_MEMDEV_FLASH_INIT_ID            ((USHORT)0xE221)
//reserved
#define MESG_FS_ANTFS_EVENT_ID               ((USHORT)0xE230)
#define MESG_FS_ANTFS_OPEN_ID                ((USHORT)0xE231)
#define MESG_FS_ANTFS_CLOSE_ID               ((USHORT)0xE232)
#define MESG_FS_ANTFS_CONFIG_BEACON_ID       ((USHORT)0xE233)
#define MESG_FS_ANTFS_SET_AUTH_STRING_ID     ((USHORT)0xE234)
#define MESG_FS_ANTFS_SET_BEACON_STATE_ID    ((USHORT)0xE235)
#define MESG_FS_ANTFS_PAIR_RESPONSE_ID       ((USHORT)0xE236)
#define MESG_FS_ANTFS_SET_LINK_FREQ_ID       ((USHORT)0xE237)
#define MESG_FS_ANTFS_SET_BEACON_TIMEOUT_ID  ((USHORT)0xE238)
#define MESG_FS_ANTFS_SET_PAIRING_TIMEOUT_ID ((USHORT)0xE239)
#define MESG_FS_ANTFS_REMOTE_FILE_CREATE_EN_ID ((USHORT)0xE23A)
#define MESG_FS_ANTFS_GET_CMD_PIPE_ID        ((USHORT)0xE23B)
#define MESG_FS_ANTFS_SET_CMD_PIPE_ID        ((USHORT)0xE23C)
#define MESG_FS_SYSTEM_TIME_ID               ((USHORT)0xE23D)
#define MESG_FS_ANTFS_SET_ANTFS_STATE_ID     ((USHORT)0xE23E)
// reserved
#define MESG_FS_CRYPTO_ADD_USER_KEY_INDEX_ID ((USHORT)0xE245)
#define MESG_FS_CRYPTO_SET_USER_KEY_INDEX_ID ((USHORT)0xE246)
#define MESG_FS_CRYPTO_SET_USER_KEY_VAL_ID   ((USHORT)0xE247)
// reserved
#define MESG_FS_FIT_FILE_INTEGRITY_CHECK_ID  ((USHORT)0xE250)


//////////////////////////////////////////////
// Message Sizes
//////////////////////////////////////////////
#define MESG_INVALID_SIZE                    ((UCHAR)0)

#define MESG_VERSION_SIZE                    ((UCHAR)13)
#define MESG_RESPONSE_EVENT_SIZE             ((UCHAR)3)
#define MESG_CHANNEL_STATUS_SIZE             ((UCHAR)2)

#define MESG_UNASSIGN_CHANNEL_SIZE           ((UCHAR)1)
#define MESG_ASSIGN_CHANNEL_SIZE             ((UCHAR)3)
#define MESG_CHANNEL_ID_SIZE                 ((UCHAR)5)
#define MESG_CHANNEL_MESG_PERIOD_SIZE        ((UCHAR)3)
#define MESG_CHANNEL_SEARCH_TIMEOUT_SIZE     ((UCHAR)2)
#define MESG_CHANNEL_RADIO_FREQ_SIZE         ((UCHAR)2)
#define MESG_CHANNEL_RADIO_TX_POWER_SIZE     ((UCHAR)2)
#define MESG_NETWORK_KEY_SIZE                ((UCHAR)9)
#define MESG_RADIO_TX_POWER_SIZE             ((UCHAR)2)
#define MESG_RADIO_CW_MODE_SIZE              ((UCHAR)3)
#define MESG_RADIO_CW_INIT_SIZE              ((UCHAR)1)
#define MESG_SYSTEM_RESET_SIZE               ((UCHAR)1)
#define MESG_OPEN_CHANNEL_SIZE               ((UCHAR)1)
#define MESG_CLOSE_CHANNEL_SIZE              ((UCHAR)1)
#define MESG_REQUEST_SIZE                    ((UCHAR)2)
#define MESG_REQUEST_USER_NVM_SIZE           ((UCHAR)5)

#define MESG_CAPABILITIES_SIZE               ((UCHAR)8)
#define MESG_STACKLIMIT_SIZE                 ((UCHAR)2)

#define MESG_SCRIPT_DATA_SIZE                ((UCHAR)10)
#define MESG_SCRIPT_CMD_SIZE                 ((UCHAR)3)

#define MESG_ID_LIST_ADD_SIZE                ((UCHAR)6)
#define MESG_ID_LIST_CONFIG_SIZE             ((UCHAR)3)
#define MESG_OPEN_RX_SCAN_SIZE               ((UCHAR)1)
#define MESG_EXT_CHANNEL_RADIO_FREQ_SIZE     ((UCHAR)3)

#define MESG_RADIO_CONFIG_ALWAYS_SIZE        ((UCHAR)2)
#define MESG_RX_EXT_MESGS_ENABLE_SIZE        ((UCHAR)2)
#define MESG_SET_TX_SEARCH_ON_NEXT_SIZE      ((UCHAR)2)
#define MESG_SET_LP_SEARCH_TIMEOUT_SIZE      ((UCHAR)2)

#define MESG_SERIAL_NUM_SET_CHANNEL_ID_SIZE  ((UCHAR)3)
#define MESG_ENABLE_LED_FLASH_SIZE           ((UCHAR)2)
#define MESG_GET_SERIAL_NUM_SIZE             ((UCHAR)4)
#define MESG_GET_TEMP_CAL_SIZE               ((UCHAR)4)
#define MESG_CONFIG_ADV_BURST_SIZE           ((UCHAR)9)
#define MESG_CONFIG_ADV_BURST_SIZE_EXT       ((UCHAR)12)
#define MESG_ANTLIB_CONFIG_SIZE              ((UCHAR)2)
#define MESG_XTAL_ENABLE_SIZE                ((UCHAR)1)
#define MESG_STARTUP_MESG_SIZE               ((UCHAR)1)
#define MESG_AUTO_FREQ_CONFIG_SIZE           ((UCHAR)4)
#define MESG_PROX_SEARCH_CONFIG_SIZE         ((UCHAR)2)
#define MESG_EVENT_BUFFERING_CONFIG_SIZE     ((UCHAR)6)
#define MESG_EVENT_FILTER_CONFIG_SIZE        ((UCHAR)3)
#define MESG_HIGH_DUTY_SEARCH_MODE_SIZE      ((UCHAR)3)
#define MESG_SDU_CONFIG_SIZE                 ((UCHAR)3)
#define MESG_SDU_SET_MASK_SIZE               ((UCHAR)9)

#define MESG_USER_CONFIG_PAGE_SIZE           ((UCHAR)3)

#define MESG_ACTIVE_SEARCH_SHARING_SIZE      ((UCHAR)2)

#define MESG_SET_SEARCH_CH_PRIORITY_SIZE     ((UCHAR)2)

#define MESG_ENCRYPT_ENABLE_SIZE             ((UCHAR)4)
#define MESG_SET_CRYPTO_KEY_SIZE             ((UCHAR)17)
#define MESG_SET_CRYPTO_ID_SIZE              ((UCHAR)5)
#define MESG_SET_CRYPTO_USER_INFO_SIZE       ((UCHAR)20)
#define MESG_SET_CRYPTO_RNG_SEED_SIZE        ((UCHAR)17)
#define MESG_NVM_CRYPTO_KEY_LOAD_SIZE        ((UCHAR)3)
#define MESG_NVM_CRYPTO_KEY_STORE_SIZE       ((UCHAR)18)
#define MESG_CRYPTO_ID_LIST_ADD_SIZE         ((UCHAR)6)
#define MESG_CRYPTO_ID_LIST_CONFIG_SIZE      ((UCHAR)3)

#define MESG_GET_PIN_DIODE_CONTROL_SIZE      ((UCHAR)1)
#define MESG_PIN_DIODE_CONTROL_ID_SIZE       ((UCHAR)2)
#define MESG_FIT1_SET_EQUIP_STATE_SIZE       ((UCHAR)2)
#define MESG_FIT1_SET_AGC_SIZE               ((UCHAR)4)

#define MESG_BIST_SIZE                       ((UCHAR)6)
#define MESG_UNLOCK_INTERFACE_SIZE           ((UCHAR)1)
#define MESG_SET_SHARED_ADDRESS_SIZE         ((UCHAR)3)

#define MESG_GET_GRMN_ESN_SIZE               ((UCHAR)5)

#define MESG_PORT_SET_IO_STATE_SIZE          ((UCHAR)5)


#define MESG_SLEEP_SIZE                      ((UCHAR)1)


#define MESG_EXT_DATA_SIZE                   ((UCHAR)13)

#define MESG_RSSI_SIZE                       ((UCHAR)5)
#define MESG_RSSI_DATA_SIZE                  ((UCHAR)17)
#define MESG_RSSI_RESPONSE_SIZE              ((UCHAR)7)
#define MESG_RSSI_SEARCH_THRESHOLD_SIZE      ((UCHAR)2)

#define MESG_MEMDEV_EEPROM_INIT_SIZE               ((UCHAR) 0x04)
#define MESG_FS_INIT_MEMORY_SIZE                   ((UCHAR) 0x01)
#define MESG_FS_FORMAT_MEMORY_SIZE                 ((UCHAR) 0x05)
#define MESG_FS_DIRECTORY_SAVE_SIZE                ((UCHAR) 0x01)
#define MESG_FS_DIRECTORY_REBUILD_SIZE             ((UCHAR) 0x01)
#define MESG_FS_FILE_DELETE_SIZE                   ((UCHAR) 0x02)
#define MESG_FS_FILE_CLOSE_SIZE                    ((UCHAR) 0x02)
#define MESG_FS_FILE_SET_SPECIFIC_FLAGS_SIZE       ((UCHAR) 0x03)
#define MESG_FS_DIRECTORY_READ_LOCK_SIZE           ((UCHAR) 0x02)
#define MESG_FS_SYSTEM_TIME_SIZE                   ((UCHAR) 0x05)
#define MESG_FS_CRYPTO_ADD_USER_KEY_INDEX_SIZE     ((UCHAR) 0x34)
#define MESG_FS_CRYPTO_SET_USER_KEY_INDEX_SIZE     ((UCHAR) 0x05)
#define MESG_FS_CRYPTO_SET_USER_KEY_VAL_SIZE       ((UCHAR) 0x33)
#define MESG_FS_FIT_FILE_INTEGRITY_CHECK_SIZE      ((UCHAR) 0x02)
#define MESG_FS_ANTFS_OPEN_SIZE                    ((UCHAR) 0x01)
#define MESG_FS_ANTFS_CLOSE_SIZE                   ((UCHAR) 0x01)
#define MESG_FS_ANTFS_CONFIG_BEACON_SIZE           ((UCHAR) 0x09)
#define MESG_FS_ANTFS_SET_AUTH_STRING_SIZE         ((UCHAR) 0x02)
#define MESG_FS_ANTFS_SET_BEACON_STATE_SIZE        ((UCHAR) 0x02)
#define MESG_FS_ANTFS_PAIR_RESPONSE_SIZE           ((UCHAR) 0x02)
#define MESG_FS_ANTFS_SET_LINK_FREQ_SIZE           ((UCHAR) 0x03)
#define MESG_FS_ANTFS_SET_BEACON_TIMEOUT_SIZE      ((UCHAR) 0x02)
#define MESG_FS_ANTFS_SET_PAIRING_TIMEOUT_SIZE     ((UCHAR) 0x02)
#define MESG_FS_ANTFS_REMOTE_FILE_CREATE_EN_SIZE   ((UCHAR) 0x02)
#define MESG_FS_GET_USED_SPACE_SIZE                ((UCHAR) 0x03)
#define MESG_FS_GET_FREE_SPACE_SIZE                ((UCHAR) 0x03)
#define MESG_FS_FIND_FILE_INDEX_SIZE               ((UCHAR) 0x07)
#define MESG_FS_DIRECTORY_READ_ABSOLUTE_SIZE       ((UCHAR) 0x08)
#define MESG_FS_DIRECTORY_READ_ENTRY_SIZE          ((UCHAR) 0x05)
#define MESG_FS_DIRECTORY_GET_SIZE_SIZE            ((UCHAR) 0x03)
#define MESG_FS_FILE_CREATE_SIZE                   ((UCHAR) 0x0B)
#define MESG_FS_FILE_OPEN_SIZE                     ((UCHAR) 0x06)
#define MESG_FS_FILE_READ_ABSOLUTE_SIZE            ((UCHAR) 0x09)
#define MESG_FS_FILE_READ_RELATIVE_SIZE            ((UCHAR) 0x05)
#define MESG_FS_FILE_WRITE_ABSOLUTE_SIZE           ((UCHAR) 0x09)
#define MESG_FS_FILE_WRITE_RELATIVE_SIZE           ((UCHAR) 0x05)
#define MESG_FS_FILE_GET_SIZE_SIZE                 ((UCHAR) 0x04)
#define MESG_FS_FILE_GET_SIZE_IN_MEM_SIZE          ((UCHAR) 0x04)
#define MESG_FS_FILE_GET_SPECIFIC_FILE_FLAGS_SIZE  ((UCHAR) 0x04)
#define MESG_FS_SYSTEM_TIME_REQUEST_SIZE           ((UCHAR) 0x03)
#define MESG_FS_ANTFS_GET_CMD_PIPE_SIZE            ((UCHAR) 0x05)
#define MESG_FS_ANTFS_SET_CMD_PIPE_SIZE            ((UCHAR) 0x05)
#define MESG_FS_REQUEST_RESPONSE_SIZE              ((UCHAR) 0x03)


//////////////////////////////////////////////
// ANT serial message structure
//////////////////////////////////////////////
#if defined (DSI_TYPES_OTHER_OS) || defined(__BORLANDC__)  // Nameless unions/struct are not supported by all compilers, remove this if you wish to use the following structures

typedef union
{
   UCHAR ucExtMesgBF;
   struct
   {
      BOOL bExtFieldCont : 1;
      BOOL               : 1;
      BOOL               : 1;
      BOOL               : 1;
      BOOL               : 1;
      BOOL bANTTimeStamp : 1;
      BOOL bReserved1    : 1;
      BOOL bANTDeviceID  : 1;
   };

} EXT_MESG_BF;                                                          // extended message bitfield

typedef union
{
   ULONG ulForceAlign;                                                  // force the struct to be 4-byte aligned, required for some casting in command.c
   UCHAR aucMessage[MESG_BUFFER_SIZE];                                  // the complete message buffer pointer
   struct
   {
      UCHAR ucSize;                                                     // the message size
      union
      {
         UCHAR aucFramedData[MESG_FRAMED_SIZE];                         // pointer for serial framer
         struct
         {
            UCHAR ucMesgID;                                             // the message ID
            union
            {
               UCHAR aucMesgData[MESG_MAX_SIZE_VALUE];                   // the message data
               struct
               {
                  union
                  {
                     UCHAR       ucChannel;                              // ANT channel number
                     UCHAR       ucSubID;                                // subID portion of ext ID message
                  };
                  UCHAR       aucPayload[ANT_STANDARD_DATA_PAYLOAD_SIZE];// ANT message payload
                  EXT_MESG_BF sExtMesgBF;                               // extended message bitfield (NOTE: this will not be here for longer data messages)
                  UCHAR       aucExtData[MESG_MAX_EXT_DATA_SIZE];       // extended message data
               };
            };
         };
      };
      UCHAR ucCheckSum;                                                 // the message checksum
   };


} ANT_MESSAGE;

#endif

#endif // !ANTMESSAGE_H
