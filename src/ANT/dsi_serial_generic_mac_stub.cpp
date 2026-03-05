// macOS stub for DSISerialGeneric.
// ANT USB stick serial I/O is not supported on macOS.
// Provides empty/failing implementations so the rest of the code links.
// TODO: remove together with the rest of ANT once a clean Mac build is stable.

#if defined(Q_OS_MAC) || defined(__APPLE__)

#include "dsi_serial_generic.hpp"
#include <string.h>

time_t DSISerialGeneric::lastUsbResetTime = 0;

DSISerialGeneric::DSISerialGeneric()
{
    pclDeviceHandle = nullptr;
    pclDevice       = nullptr;
    hReceiveThread  = nullptr;
    bStopReceiveThread = TRUE;
    ucDeviceNumber  = 0xFF;
    ulBaud          = 0;
}

DSISerialGeneric::~DSISerialGeneric()
{
}

BOOL DSISerialGeneric::AutoInit()              { return FALSE; }
BOOL DSISerialGeneric::Init(ULONG, UCHAR)      { return FALSE; }
BOOL DSISerialGeneric::Init(ULONG, const USBDevice&, UCHAR) { return FALSE; }
BOOL DSISerialGeneric::Open()                  { return FALSE; }
void DSISerialGeneric::Close(BOOL)             {}
BOOL DSISerialGeneric::WriteBytes(void*, USHORT) { return FALSE; }
UCHAR DSISerialGeneric::GetDeviceNumber()      { return 0xFF; }
ULONG DSISerialGeneric::GetDeviceSerialNumber(){ return 0; }
UCHAR DSISerialGeneric::GetNumberOfDevices()   { return 0; }
void  DSISerialGeneric::USBReset()             {}

BOOL DSISerialGeneric::GetDeviceUSBInfo(UCHAR, UCHAR* pProd, UCHAR* pSer, USHORT sz)
{
    if (pProd && sz) pProd[0] = '\0';
    if (pSer  && sz) pSer[0]  = '\0';
    return FALSE;
}

BOOL DSISerialGeneric::GetDevicePID(USHORT& pid) { pid = 0; return FALSE; }
BOOL DSISerialGeneric::GetDeviceVID(USHORT& vid) { vid = 0; return FALSE; }

BOOL DSISerialGeneric::GetDeviceSerialString(UCHAR* pucSerial, USHORT usSize)
{
    if (pucSerial && usSize) pucSerial[0] = '\0';
    return FALSE;
}

#endif // macOS stub
