#ifndef KICKRANT_H
#define KICKRANT_H



#include <stdint.h>
#include "types.h"



typedef enum
{
    // WF Trainer 0x40-0x5F
    WF_WCPMCP_OPCODE_READ_TEMPERATURE                      = 0x08,
    WF_WCPMCP_OPCODE_TRAINER_SET_RESISTANCE_MODE           = 0x40,
    WF_WCPMCP_OPCODE_TRAINER_SET_STANDARD_MODE             = 0x41,
    WF_WCPMCP_OPCODE_TRAINER_SET_ERG_MODE                  = 0x42,
    WF_WCPMCP_OPCODE_TRAINER_SET_SIM_MODE                  = 0x43,
    WF_WCPMCP_OPCODE_TRAINER_SET_CRR                       = 0x44,
    WF_WCPMCP_OPCODE_TRAINER_SET_C                         = 0x45,
    WF_WCPMCP_OPCODE_TRAINER_SET_GRADE                     = 0x46,
    WF_WCPMCP_OPCODE_TRAINER_SET_WIND_SPEED                = 0x47,
    WF_WCPMCP_OPCODE_TRAINER_SET_WHEEL_CIRCUMFERENCE       = 0x48,
    WF_WCPMCP_OPCODE_TRAINER_INIT_SPINDOWN                 = 0x49,
    WF_WCPMCP_OPCODE_TRAINER_READ_MODE                     = 0x4A,
    WF_WCPMCP_OPCODE_TRAINER_SET_FTP_MODE                  = 0x4B,
    WF_WCPMCP_OPCODE_TRAINER_CONNECT_ANT_SENSOR            = 0x4F,
    WF_WCPMCP_OPCODE_TRAINER_TEST_OP                       = 0x50,
    WF_WCPMCP_OPCODE_TRAINER_SET_CONTROL_PASSCODE          = 0x51,
    WF_WCPMCP_OPCODE_TRAINER_RESET_CONTROL_PASSCODE        = 0x52,
    WF_WCPMCP_OPCODE_TRAINER_ENABLE_CONTROL                = 0x53,
    WF_WCPMCP_OPCODE_TRAINER_SET_FEATURE_ENABLED_STATE     = 0x54,
    WF_WCPMCP_OPCODE_TRAINER_READ_FEATURE_ENABLED_STATE    = 0x55,
    WF_WCPMCP_OPCODE_TRAINER_SPINDOWN_RESULT               = 0x5A,

} WFKICKRCommands_t;



// NOTE:  to create a KICKR-specific wildcard search (as opposed to just any
// power meter type), use the KICKR_TRANSMISSION_TYPE in the SetChannelId message.
// e.g. ANT_SetChannelId(0, 0, 0x0B, KICKR_TRANSMISSION_TYPE);
#define KICKR_TRANSMISSION_TYPE             ((UCHAR)0xA5)

#define ACK_MESSAGE_TIMEOUT                 2.0  // 2 sec timeout.
#define BURST_RETRY_COUNT                   5

// Command Burst
#define GLOBAL_PAGE_72                       ((UCHAR) 72)
#define ANT_COMMAND_BURST                    GLOBAL_PAGE_72

// Manufacturer-Specific pages (0xF0 - 0xFF).
#define WF_ANT_RESPONSE_PAGE_ID              ((UCHAR) 0xF0)
//
#define WF_ANT_RESPONSE_FLAG                 ((UCHAR) 0x80)

#define WF_KICKR_COMMAND_PAGE                ((UCHAR) 0xF1)
#define DATA_PAGE_49                         ((UCHAR) 0x31)

#define ANTPLUS_RESERVED                     ((UCHAR) 0xFF)


typedef struct
{
    UCHAR dataPage;
    UCHAR commandId;
    UCHAR responseStatus;
    UCHAR commandSequence;
    UCHAR responseData0;
    UCHAR responseData1;
    UCHAR responseData2;
    UCHAR responseData3;

} ANTMsgWahoo240_t;



class KickrAnt
{
public:
    KickrAnt();
    ~KickrAnt();



    static void EncodeTrainerSetErgMode(uint8_t* pucBuffer, uint16_t usDeviceId, uint16_t usWatts, bool bSimSpeed);
    static void EncodeTrainerSetFtpMode(uint8_t* pucBuffer, uint16_t usDeviceId, uint16_t usFtp, uint16_t usPercent);
    static void EncodeTrainerSetResistanceMode(uint8_t* pucBuffer, uint16_t usDeviceId, uint16_t usScale);
    static void EncodeTrainerSetStandardMode(uint8_t* pucBuffer, uint16_t usDeviceId, uint8_t eLevel);
    static void EncodeTrainerSetSimMode(uint8_t* pucBuffer, uint16_t usDeviceId, float fWeight);
    static void EncodeTrainerSetWindResistance(uint8_t* pucBuffer, uint16_t usDeviceId, float fC);
    static void EncodeTrainerSetRollingResistance(uint8_t* pucBuffer, uint16_t usDeviceId, float fCrr);
    static void EncodeTrainerSetGrade(uint8_t* pucBuffer, uint16_t usDeviceId, float fGrade);
    static void EncodeTrainerSetWindSpeed(uint8_t* pucBuffer, uint16_t usDeviceId, float mpsWindSpeed);
    static void EncodeTrainerSetWheelCircumference(uint8_t* pucBuffer, uint16_t usDeviceId, float mmCircumference);
    static void EncodeTrainerReadMode(uint8_t* pucBuffer, uint16_t usDeviceId);
    static void EncodeTrainerInitSpindown(uint8_t* pucBuffer, uint16_t usDeviceId);





};

#endif // KICKRANT_H
