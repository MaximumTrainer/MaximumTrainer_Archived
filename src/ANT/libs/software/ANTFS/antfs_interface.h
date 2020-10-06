/*
This software is subject to the license described in the License.txt file
included with this software distribution. You may not use this file except
in compliance with this license.

Copyright (c) Dynastream Innovations Inc. 2016
All rights reserved.
*/
#if !defined(ANTFS_INTERFACE_H)
#define ANTFS_INTERFACE_H

//////////////////////////////////////////////////////////////////////////////////
// Public ANT-FS Definitions
//////////////////////////////////////////////////////////////////////////////////

// String lengths
#define FRIENDLY_NAME_MAX_LENGTH       255
#define TX_PASSWORD_MAX_LENGTH         255
#define PASSWORD_MAX_LENGTH            255

#define ANTFS_AUTO_FREQUENCY_SELECTION    ((UCHAR) MAX_UCHAR)
#define ANTFS_DEFAULT_TRANSPORT_FREQ      ANTFS_AUTO_FREQUENCY_SELECTION

typedef enum
{
   ANTFS_RETURN_FAIL = 0,
   ANTFS_RETURN_PASS,
   ANTFS_RETURN_BUSY
} ANTFS_RETURN;

#endif // ANTFS_INTERFACE_H