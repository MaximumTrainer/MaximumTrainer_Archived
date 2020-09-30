/*
This software is subject to the license described in the License.txt file
included with this software distribution. You may not use this file except in compliance
with this license.

Copyright (c) Dynastream Innovations Inc. 2013
All rights reserved.
*/

#ifndef BPS_PAGES_H
#define BPS_PAGES_H

#include "types.h"


#define BPS_PAGE_RESERVE_BYTE               ((UCHAR) 0xFF)

#define BPS_PI_CONSTANT                     ((USHORT) 804)  // pi *256

#define PBS_CID_16							((UCHAR) 16)   // CID = 0x10
#define PBS_CID_16_CTFID_1					((UCHAR) 1)    // CTFID = 0x01
#define PBS_CID_16_CTFID_2					((UCHAR) 2)    // CTFID = 0x02
#define PBS_CID_16_CTFID_3					((UCHAR) 3)   // CTFID = 0x03
#define PBS_CID_16_CTFID_172				((UCHAR) 172)  // CTFID = 0xAC
#define PBS_CID_18							((UCHAR) 18)   // CID = 0x12
#define PBS_CID_170							((UCHAR) 170)  // CID = 0xAA
#define PBS_CID_171							((UCHAR) 171)  // CID = 0xAB
#define PBS_CID_172							((UCHAR) 172)  // CID = 0xAC
#define PBS_CID_175							((UCHAR) 175)  // CID = 0xAF
//--Custom added by Max++ inc.
#define PBS_CID_186							((UCHAR) 186)  // CID = 0xBA
#define PBS_CID_187							((UCHAR) 187)  // CID = 0xBB
#define PBS_CID_188							((UCHAR) 188)  // CID = 0xBC
#define PBS_CID_189							((UCHAR) 189)  // CID = 0xBD



#define BPSRX_MAX_REPEATS                    ((USHORT) 12)     // Set Max # of no updates before zero speed set
#define BPSRX_INITIAL_OFFSET	             ((USHORT)100)
#define BPSRX_ZERO_SPEED		             ((UCHAR)   0)
// BPS RX constants for calibration
#define BPSRX_DEFINED_SLOPE		             ((USHORT) 200)    // Default Bike wheel circumference (m)
#define BPSRX_DEFINED_SERIAL                 ((USHORT) 67890)  // Set Max # of no updates (event count for event sync or no increase in
#define BPSRX_CALIBRATION_TIMEOUT            ((USHORT) 480)

// BPS Subevent defines
#define BPSRX_SUBEVENT_UPDATE         ((USHORT) 0)
#define BPSRX_SUBEVENT_NOUPDATE       ((USHORT) 1)
#define BPSRX_SUBEVENT_ZERO_SPEED     ((USHORT) 2)
#define BPSRX_SUBEVENT_COASTING       ((USHORT) 3)
#define BPSRX_SUBEVENT_FIRSTPAGE      ((USHORT) 4)




// Page 1 structure types
typedef struct
{
    UCHAR ucCID170_ReserveByte2;
    UCHAR ucCID170_ReserveByte3;
    UCHAR ucCID170_ReserveByte4;
    UCHAR ucCID170_ReserveByte5;
    UCHAR ucCID170_ReserveByte6;
    UCHAR ucCID170_ReserveByte7;
} BPSPage1_CID170;							// Calibration ID (CID) 0xAA

typedef struct
{
    UCHAR ucCID171_AutoZeroStatus;
    UCHAR ucCID171_ReserveByte3;
    UCHAR ucCID171_ReserveByte4;
    UCHAR ucCID171_ReserveByte5;
    UCHAR ucCID171_ReserveByte6;
    UCHAR ucCID171_ReserveByte7;
} BPSPage1_CID171;							// Calibration ID (CID) 0xAB

typedef struct
{
    UCHAR ucCID172_AutoZeroStatus;
    UCHAR ucCID172_ReserveByte3;
    UCHAR ucCID172_ReserveByte4;
    UCHAR ucCID172_ReserveByte5;
    USHORT usCID172_CalibrationData;
} BPSPage1_CID172;							// Calibration ID (CID) 0xAC

typedef struct
{
    UCHAR ucCID175_AutoZeroStatus;
    UCHAR ucCID175_ReserveByte3;
    UCHAR ucCID175_ReserveByte4;
    UCHAR ucCID175_ReserveByte5;
    USHORT usCID175_CalibrationData;
} BPSPage1_CID175;							// Calibration ID (CID) 0xAF

typedef struct
{
//	UCHAR ucCID16_CTFID1;
    UCHAR ucCID16_CTFID1_ReserveByte3;
    UCHAR ucCID16_CTFID1_ReserveByte4;
    UCHAR ucCID16_CTFID1_ReserveByte5;
    USHORT usCID16_CTFID1_OffsetData;
} BPSPage1_CID16_CTFID1;					// Calibration ID (CID) 0x10, CTF ID 0x01

typedef struct
{
//	UCHAR ucCID16_CTFID2;
    UCHAR ucCID16_CTFID2_ReserveByte3;
    UCHAR ucCID16_CTFID2_ReserveByte4;
    UCHAR ucCID16_CTFID2_ReserveByte5;
    USHORT usCID16_CTFID2_SlopeData;
} BPSPage1_CID16_CTFID2;					// Calibration ID (CID) 0x10, CTF ID 0x02

typedef struct
{
//	UCHAR ucCID16_CTFID3;
    UCHAR ucCID16_CTFID3_ReserveByte3;
    UCHAR ucCID16_CTFID3_ReserveByte4;
    UCHAR ucCID16_CTFID3_ReserveByte5;
    USHORT usCID16_CTFID3_SerialNumber;
} BPSPage1_CID16_CTFID3;					// Calibration ID (CID) 0x10, CTF ID 0x03

typedef struct
{
//	UCHAR ucCID16_CTFID172;
    UCHAR ucCID16_CTFID172_CTFAckMessage;
    UCHAR ucCID16_CTFID172_ReserveByte4;
    UCHAR ucCID16_CTFID172_ReserveByte5;
    UCHAR ucCID16_CTFID172_ReserveByte6;
    UCHAR ucCID16_CTFID172_ReserveByte7;
} BPSPage1_CID16_CTFID172;					// Calibration ID (CID) 0x10, CTF ID 0xAC

typedef struct
{
    UCHAR  ucCID18_SensorConfiguration_EN;
    UCHAR  ucCID18_SensorConfiguration_STATUS;
    UCHAR  ucCID18_SensorConfiguration_RESERVED;
    SSHORT ssCID18_RawTorqueCount;
    SSHORT ssCID18_OffsetTorque;
    UCHAR  ucCID18_ReserveByte7;
} BPSPage1_CID18;							// Calibration ID (CID) 0x12

typedef union
{
    BPSPage1_CID170 		stBPSPage1_CID170_Data;				// Calibration ID (CID) 0xAA
    BPSPage1_CID171 		stBPSPage1_CID171_Data;				// Calibration ID (CID) 0xAB
    BPSPage1_CID172 		stBPSPage1_CID172_Data;				// Calibration ID (CID) 0xAC
    BPSPage1_CID175 		stBPSPage1_CID175_Data;				// Calibration ID (CID) 0xAF
    BPSPage1_CID16_CTFID1	stBPSPage1_CID16_CTFID1_Data;		// Calibration ID (CID) 0x10, CTF ID 0x01
    BPSPage1_CID16_CTFID2 	stBPSPage1_CID16_CTFID2_Data;		// Calibration ID (CID) 0x10, CTF ID 0x02
    BPSPage1_CID16_CTFID3 	stBPSPage1_CID16_CTFID3_Data;		// Calibration ID (CID) 0x10, CTF ID 0x03
    BPSPage1_CID16_CTFID172 stBPSPage1_CID16_CTFID172_Data;		// Calibration ID (CID) 0x10, CTF ID 0xAC
    BPSPage1_CID18 			stBPSPage1_CID18_Data;				// Calibration ID (CID) 0x12
} BPSPage1_Struct;

typedef struct
{
    UCHAR	ucCalibrationID;
    BPSPage1_Struct stBPSCalibrationData;
}BPSPage1_Data;										//Page 1

typedef struct
{
   UCHAR ucPOEventCount;
   UCHAR ucReserveByte2;
   UCHAR ucPOInstantCadence;
   USHORT usPOAccumulatedPower;
   USHORT usPOInstantPower;							// Page 16
} BPSPage16_Data;

typedef struct
{
   UCHAR ucWTEventCount;
   UCHAR ucWTWheelTicks;
   UCHAR ucWTInstantCadence;
   USHORT usWTAccumulatedWheelPeriod;
   USHORT usWTAccumulatedTorque;
} BPSPage17_Data;

typedef struct
{
   UCHAR ucCTEventCount;
   UCHAR ucCTCrankTicks;
   UCHAR ucCTInstantCadence;
   USHORT usCTAccumulatedCrankPeriod;
   USHORT usCTAccumulatedTorque;
} BPSPage18_Data;

typedef struct
{
   UCHAR ucCTFEventCount;
   USHORT usCTFSlope;
   USHORT usCTFTimeStamp;
   USHORT usCTFTorqueTickStamp;
} BPSPage32_Data;



////////////////////////////////////////////////////////////////////////////////////////////////
//calculated data structures
typedef struct
{
    ULONG ulAvgStandardPower;
}CalculatedStdPower;

typedef struct
{
    ULONG ulWTAvgSpeed;
    ULONG ulWTDistance;
    ULONG ulWTAngularVelocity;
    ULONG ulWTAverageTorque;
    ULONG ulWTAveragePower;
}CalculatedStdWheelTorque;

typedef struct
{
    ULONG ulCTAverageCadence;
    ULONG ulCTAngularVelocity;
    ULONG ulCTAverageTorque;
    ULONG ulCTAveragePower;
}CalculatedStdCrankTorque;

typedef struct
{
    ULONG ulCTFAverageCadence;
    ULONG ulCTFTorqueFrequency;
    ULONG ulCTFAverageTorque;
    ULONG ulCTFAveragePower;
}CalculatedStdCTF;

typedef enum
{
   CALIBRATION_TYPE_MANUAL,
   CALIBRATION_TYPE_AUTO,
   CALIBRATION_TYPE_SLOPE,
   CALIBRATION_TYPE_SERIAL
} CalibrationType;



#endif // BPS_PAGES_H






