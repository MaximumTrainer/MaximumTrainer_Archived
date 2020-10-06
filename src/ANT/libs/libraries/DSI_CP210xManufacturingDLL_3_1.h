/*
This software is subject to the license described in the License.txt file
included with this software distribution. You may not use this file except
in compliance with this license.

Copyright (c) Dynastream Innovations Inc. 2016
All rights reserved.
*/
#if !defined(DSI_CP210XMANUFACTURINGDLL_3_1_H)
#define DSI_CP210XMANUFACTURINGDLL_3_1_H
// The following ifdef block is the standard way of creating macros which make exporting
// from a DLL simpler. All files within this DLL are compiled with the CP210xDLL_EXPORTS
// symbol defined on the command line. this symbol should not be defined on any project
// that uses this DLL. This way any other project whose source files include this file see
// CP210xDLL_API functions as being imported from a DLL, wheras this DLL sees symbols
// defined with this macro as being exported.
#ifdef CP210xMANUFACTURINGDLL_EXPORTS
#define CP210xDLL_API __declspec(dllexport)
#else
#define CP210xDLL_API __declspec(dllimport)
#endif

// GetProductString() function flags
#define      CP210x_RETURN_SERIAL_NUMBER         0x00
#define      CP210x_RETURN_DESCRIPTION         0x01
#define      CP210x_RETURN_FULL_PATH            0x02

#ifndef _CP210x_STANDARD_DEF_
#define _CP210x_STANDARD_DEF_
// GetDeviceVersion() return codes
#define      CP210x_CP2101_VERSION            0x01
#define      CP210x_CP2102_VERSION            0x02
#define      CP210x_CP2103_VERSION            0x03

// Return codes
#define      CP210x_SUCCESS                  0x00
#define      CP210x_DEVICE_NOT_FOUND            0xFF
#define      CP210x_INVALID_HANDLE            0x01
#define      CP210x_INVALID_PARAMETER         0x02
#define      CP210x_DEVICE_IO_FAILED            0x03
#define      CP210x_FUNCTION_NOT_SUPPORTED      0x04
#define      CP210x_GLOBAL_DATA_ERROR         0x05
#define      CP210x_FILE_ERROR               0x06
#define      CP210x_COMMAND_FAILED            0x08
#define      CP210x_INVALID_ACCESS_TYPE         0x09

// Type definitions
typedef      int      CP210x_STATUS;
#endif /*_CP210x_STANDARD_DEF_*/

// Buffer size limits
#define      CP210x_MAX_DEVICE_STRLEN         256
#define      CP210x_MAX_PRODUCT_STRLEN         126
#define      CP210x_MAX_SERIAL_STRLEN         63
#define      CP210x_MAX_MAXPOWER               250


// Type definitions
typedef      char   CP210x_DEVICE_STRING[CP210x_MAX_DEVICE_STRLEN];
typedef      char   CP210x_PRODUCT_STRING[CP210x_MAX_PRODUCT_STRLEN];
typedef      char   CP210x_SERIAL_STRING[CP210x_MAX_SERIAL_STRLEN];


//Baud Rate Aliasing definitions
#define      NUM_BAUD_CONFIGS   32

typedef      struct
{
   WORD   BaudGen;
   WORD   Timer0Reload;
   BYTE   Prescaler;
   DWORD   BaudRate;
} BAUD_CONFIG;

#define      BAUD_CONFIG_SIZE   10

typedef      BAUD_CONFIG      BAUD_CONFIG_DATA[NUM_BAUD_CONFIGS];


//Port Config definitions
typedef      struct
{
   WORD Mode;         // Push-Pull = 1, Open-Drain = 0
   WORD Reset_Latch;   // Logic High = 1, Logic Low = =0
   WORD Suspend_Latch;   // Logic High = 1, Logic Low = =0
   unsigned char EnhancedFxn;
} PORT_CONFIG;

// Define bit locations for Mode/Latch for Reset and Suspend structures
#define PORT_RI_ON            0x0001
#define PORT_DCD_ON            0x0002
#define PORT_DTR_ON            0x0004
#define PORT_DSR_ON            0x0008
#define PORT_TXD_ON            0x0010
#define PORT_RXD_ON            0x0020
#define PORT_RTS_ON            0x0040
#define PORT_CTS_ON            0x0080

#define PORT_GPIO_0_ON         0x0100
#define PORT_GPIO_1_ON         0x0200
#define PORT_GPIO_2_ON         0x0400
#define PORT_GPIO_3_ON         0x0800

#define PORT_SUSPEND_ON         0x4000   //  Can't configure latch value
#define PORT_SUSPEND_BAR_ON      0x8000   //  Can't configure latch value

// Define bit locations for EnhancedFxn
#define EF_GPIO_0_TXLED            0x01   //  Under device control
#define EF_GPIO_1_RXLED            0x02   //  Under device control
#define EF_GPIO_2_RS485            0x04   //  Under device control
#define EF_RESERVED_0            0x08   //   Reserved, leave bit 3 cleared
#define EF_WEAKPULLUP            0x10   //  Weak Pull-up on
#define EF_RESERVED_1            0x20   //   Reserved, leave bit 5 cleared
#define EF_SERIAL_DYNAMIC_SUSPEND   0x40   //  For 8 UART/Modem signals
#define EF_GPIO_DYNAMIC_SUSPEND      0x80   //  For 4 GPIO signals


#ifdef __cplusplus
extern "C" {
#endif

CP210xDLL_API
CP210x_STATUS WINAPI CP210x_GetNumDevices(
   LPDWORD   lpdwNumDevices
   );

CP210xDLL_API
CP210x_STATUS WINAPI CP210x_GetProductString(
   DWORD   dwDeviceNum,
   LPVOID   lpvDeviceString,
   DWORD   dwFlags
   );

CP210xDLL_API
CP210x_STATUS WINAPI CP210x_Open(
   DWORD   dwDevice,
   HANDLE*   cyHandle
   );

CP210xDLL_API
CP210x_STATUS WINAPI CP210x_Close(
   HANDLE   cyHandle
   );

CP210xDLL_API
CP210x_STATUS
WINAPI
CP210x_GetPartNumber(   HANDLE cyHandle,
                  LPBYTE   lpbPartNum
   );

CP210xDLL_API
CP210x_STATUS
WINAPI
CP210x_SetVid(
   HANDLE   cyHandle,
   WORD   wVid
   );

CP210xDLL_API
CP210x_STATUS
WINAPI
CP210x_SetPid(
   HANDLE   cyHandle,
   WORD   wPid
   );

CP210xDLL_API
CP210x_STATUS
WINAPI
CP210x_SetProductString(
   HANDLE   cyHandle,
   LPVOID   lpvProduct,
   BYTE   bLength,
   BOOL   bConvertToUnicode
         #if defined(__cplusplus)
            = TRUE
         #endif
   );

CP210xDLL_API
CP210x_STATUS
WINAPI
CP210x_SetSerialNumber(
   HANDLE   cyHandle,
   LPVOID   lpvSerialNumber,
   BYTE   bLength,
   BOOL   bConvertToUnicode
         #if defined(__cplusplus)
            = TRUE
         #endif
   );

CP210xDLL_API
CP210x_STATUS
WINAPI
CP210x_SetSelfPower(
   HANDLE cyHandle,
   BOOL bSelfPower
   );

CP210xDLL_API
CP210x_STATUS
WINAPI
CP210x_SetMaxPower(
   HANDLE cyHandle,
   BYTE bMaxPower
   );

CP210xDLL_API
CP210x_STATUS
WINAPI
CP210x_SetDeviceVersion(
   HANDLE cyHandle,
   WORD wVersion
   );

CP210xDLL_API
CP210x_STATUS
WINAPI
CP210x_SetBaudRateConfig(
   HANDLE   cyHandle,
   BAUD_CONFIG* baudConfigData
   );

CP210xDLL_API
CP210x_STATUS
WINAPI
CP210x_SetLockValue(
   HANDLE cyHandle
   );

CP210xDLL_API
CP210x_STATUS
WINAPI
CP210x_GetDeviceProductString(
   HANDLE   cyHandle,
   LPVOID   lpProduct,
   LPBYTE   lpbLength,
   BOOL   bConvertToASCII
         #if defined(__cplusplus)
            = TRUE
         #endif
   );

CP210xDLL_API
CP210x_STATUS
WINAPI
CP210x_GetDeviceSerialNumber(
   HANDLE   cyHandle,
   LPVOID   lpSerialNumber,
   LPBYTE   lpbLength,
   BOOL   bConvertToASCII
         #if defined(__cplusplus)
            = TRUE
         #endif
   );

CP210xDLL_API
CP210x_STATUS
WINAPI
CP210x_GetDeviceVid(
   HANDLE   cyHandle,
   LPWORD   lpwVid
   );

CP210xDLL_API
CP210x_STATUS
WINAPI
CP210x_GetDevicePid(
   HANDLE   cyHandle,
   LPWORD   lpwPid
   );

CP210xDLL_API
CP210x_STATUS
WINAPI
CP210x_GetSelfPower(
   HANDLE   cyHandle,
   LPBOOL   lpbSelfPower
   );

CP210xDLL_API
CP210x_STATUS
WINAPI
CP210x_GetMaxPower(
   HANDLE   cyHandle,
   LPBYTE   lpbPower
   );

CP210xDLL_API
CP210x_STATUS
WINAPI
CP210x_GetDeviceVersion(
   HANDLE   cyHandle,
   LPWORD   lpwVersion
   );

CP210xDLL_API
CP210x_STATUS
WINAPI
CP210x_GetBaudRateConfig(
   HANDLE   cyHandle,
   BAUD_CONFIG* baudConfigData
   );

CP210xDLL_API
CP210x_STATUS
WINAPI
CP210x_GetLockValue(
   HANDLE cyHandle,
   LPBYTE   lpbLockValue
   );

CP210xDLL_API
CP210x_STATUS
WINAPI
CP210x_CreateHexFile(
   HANDLE cyHandle,
   LPCSTR lpvFileName
   );

CP210xDLL_API
CP210x_STATUS
WINAPI
CP210x_Reset(
   HANDLE   cyHandle
   );

CP210xDLL_API
CP210x_STATUS
WINAPI
CP210x_GetPortConfig(
   HANDLE cyHandle,
   PORT_CONFIG*   PortConfig
   );

CP210xDLL_API
CP210x_STATUS
WINAPI
CP210x_SetPortConfig(
   HANDLE cyHandle,
   PORT_CONFIG*   PortConfig
   );

#ifdef __cplusplus
}
#endif

#endif // !defined(DSI_CP210XMANUFACTURINGDLL_3_1_H)

