#include "kickrant.h"


#include <QDebug>



KickrAnt::KickrAnt()
{

}

KickrAnt::~KickrAnt()
{

}




void KickrAnt::EncodeTrainerSetErgMode(uint8_t* pucBuffer, uint16_t usDeviceId, uint16_t usWatts, bool bSimSpeed)
{
    uint8_t ucOffset = 0;


    // frame the ANT message.
    pucBuffer[ucOffset++] = WF_KICKR_COMMAND_PAGE;  //0xF1
    pucBuffer[ucOffset++] = (uint8_t)WF_WCPMCP_OPCODE_TRAINER_SET_ERG_MODE; //0x42
    pucBuffer[ucOffset++] = (uint8_t)usDeviceId;
    pucBuffer[ucOffset++] = (uint8_t)(usDeviceId >> 8);
    pucBuffer[ucOffset++] = (uint8_t)usWatts;
    pucBuffer[ucOffset++] = (uint8_t)(usWatts >> 8);
    pucBuffer[ucOffset++] = (uint8_t)(bSimSpeed ? 1 : 0);
    pucBuffer[ucOffset++] = ANTPLUS_RESERVED;
}

void KickrAnt::EncodeTrainerSetFtpMode(uint8_t* pucBuffer, uint16_t usDeviceId, uint16_t usFtp, uint16_t usPercent)
{
    uint8_t ucOffset = 0;

    // frame the ANT message.
    pucBuffer[ucOffset++] = WF_KICKR_COMMAND_PAGE;
    //TOFIX
    //    pucBuffer[ucOffset++] = (uint8_t)WF_WCPMCP_OPCODE_TRAINER_SET_FTP_MODE;
    pucBuffer[ucOffset++] = (uint8_t)usDeviceId;
    pucBuffer[ucOffset++] = (uint8_t)(usDeviceId >> 8);
    pucBuffer[ucOffset++] = (uint8_t)usFtp;
    pucBuffer[ucOffset++] = (uint8_t)(usFtp >> 8);
    pucBuffer[ucOffset++] = (uint8_t)usPercent;
    pucBuffer[ucOffset++] = (uint8_t)(usPercent >> 8);
}

void KickrAnt::EncodeTrainerSetResistanceMode(uint8_t* pucBuffer, uint16_t usDeviceId, uint16_t usScale)
{
    uint8_t ucOffset = 0;

    // frame the ANT message.
    pucBuffer[ucOffset++] = WF_KICKR_COMMAND_PAGE;
    pucBuffer[ucOffset++] = (uint8_t)WF_WCPMCP_OPCODE_TRAINER_SET_RESISTANCE_MODE;
    pucBuffer[ucOffset++] = (uint8_t)usDeviceId;
    pucBuffer[ucOffset++] = (uint8_t)(usDeviceId >> 8);
    pucBuffer[ucOffset++] = (uint8_t)usScale;
    pucBuffer[ucOffset++] = (uint8_t)(usScale >> 8);
    pucBuffer[ucOffset++] = ANTPLUS_RESERVED;
    pucBuffer[ucOffset++] = ANTPLUS_RESERVED;
}

//void EncodeTrainerSetStandardMode(uint8_t* pucBuffer, uint16_t usDeviceId, WFBikeTrainerLevel eLevel)
void KickrAnt::EncodeTrainerSetStandardMode(uint8_t* pucBuffer, uint16_t usDeviceId, uint8_t eLevel)
{
    uint8_t ucOffset = 0;

    // frame the ANT message.
    pucBuffer[ucOffset++] = WF_KICKR_COMMAND_PAGE;
    pucBuffer[ucOffset++] = (uint8_t)WF_WCPMCP_OPCODE_TRAINER_SET_STANDARD_MODE;
    pucBuffer[ucOffset++] = (uint8_t)usDeviceId;
    pucBuffer[ucOffset++] = (uint8_t)(usDeviceId >> 8);
    pucBuffer[ucOffset++] = (uint8_t)eLevel;
    pucBuffer[ucOffset++] = ANTPLUS_RESERVED;
    pucBuffer[ucOffset++] = ANTPLUS_RESERVED;
    pucBuffer[ucOffset++] = ANTPLUS_RESERVED;
}

void KickrAnt::EncodeTrainerSetSimMode(uint8_t* pucBuffer, uint16_t usDeviceId, float fWeight)
{
    uint8_t ucOffset = 0;

    // the weight is sent in 1/100 kg resolution.
    uint16_t usWeight = (uint16_t)(fWeight * 100.0);

    // frame the ANT message.
    pucBuffer[ucOffset++] = WF_KICKR_COMMAND_PAGE;
    pucBuffer[ucOffset++] = (uint8_t)WF_WCPMCP_OPCODE_TRAINER_SET_SIM_MODE;
    pucBuffer[ucOffset++] = (uint8_t)usDeviceId;
    pucBuffer[ucOffset++] = (uint8_t)(usDeviceId >> 8);
    pucBuffer[ucOffset++] = (uint8_t)usWeight;
    pucBuffer[ucOffset++] = (uint8_t)(usWeight >> 8);
    pucBuffer[ucOffset++] = ANTPLUS_RESERVED;
    pucBuffer[ucOffset++] = ANTPLUS_RESERVED;
}

void KickrAnt::EncodeTrainerSetWindResistance(uint8_t* pucBuffer, uint16_t usDeviceId, float fC)
{
    uint8_t ucOffset = 0;

    // encode the value.
    uint16_t usC = (uint16_t)(fC * 1000.0);

    // frame the ANT message.
    pucBuffer[ucOffset++] = WF_KICKR_COMMAND_PAGE;
    pucBuffer[ucOffset++] = (uint8_t)WF_WCPMCP_OPCODE_TRAINER_SET_C;
    pucBuffer[ucOffset++] = (uint8_t)usDeviceId;
    pucBuffer[ucOffset++] = (uint8_t)(usDeviceId >> 8);
    pucBuffer[ucOffset++] = (uint8_t)usC;
    pucBuffer[ucOffset++] = (uint8_t)(usC >> 8);
    pucBuffer[ucOffset++] = ANTPLUS_RESERVED;
    pucBuffer[ucOffset++] = ANTPLUS_RESERVED;
}

void KickrAnt::EncodeTrainerSetRollingResistance(uint8_t* pucBuffer, uint16_t usDeviceId, float fCrr)
{
    uint8_t ucOffset = 0;

    // encode the value.
    uint16_t usCrr = (uint16_t)(fCrr * 10000.0);

    // frame the ANT message.
    pucBuffer[ucOffset++] = WF_KICKR_COMMAND_PAGE;
    pucBuffer[ucOffset++] = (uint8_t)WF_WCPMCP_OPCODE_TRAINER_SET_CRR;
    pucBuffer[ucOffset++] = (uint8_t)usDeviceId;
    pucBuffer[ucOffset++] = (uint8_t)(usDeviceId >> 8);
    pucBuffer[ucOffset++] = (uint8_t)usCrr;
    pucBuffer[ucOffset++] = (uint8_t)(usCrr >> 8);
    pucBuffer[ucOffset++] = ANTPLUS_RESERVED;
    pucBuffer[ucOffset++] = ANTPLUS_RESERVED;
}


//////////////////////////////////////////////////////////////////////////////////////////////////////
void KickrAnt::EncodeTrainerSetGrade(uint8_t* pucBuffer, uint16_t usDeviceId, float fGrade)
{

    qDebug() << "encore set grade.." << fGrade;

    uint8_t ucOffset = 0;

    // convert values to 16-bit unsigned integer.
    if (fGrade > 1)
    {
        fGrade = 1;
    }
    if (fGrade < -1)
    {
        fGrade = -1;
    }
    //
    fGrade = fGrade + 1;
    uint16_t usGrade = (uint16_t)(fGrade * 65536 / 2);


    // frame the ANT message.
    pucBuffer[ucOffset++] = WF_KICKR_COMMAND_PAGE;
    pucBuffer[ucOffset++] = (uint8_t)WF_WCPMCP_OPCODE_TRAINER_SET_GRADE;
    pucBuffer[ucOffset++] = (uint8_t)usDeviceId;
    pucBuffer[ucOffset++] = (uint8_t)(usDeviceId >> 8);
    pucBuffer[ucOffset++] = (uint8_t)usGrade;
    pucBuffer[ucOffset++] = (uint8_t)(usGrade >> 8);
    pucBuffer[ucOffset++] = ANTPLUS_RESERVED;
    pucBuffer[ucOffset++] = ANTPLUS_RESERVED;
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////
void KickrAnt::EncodeTrainerSetWindSpeed(uint8_t* pucBuffer, uint16_t usDeviceId, float mpsWindSpeed)
{
    uint8_t ucOffset = 0;

    // convert values to 16-bit unsigned integer.
    //
    if (mpsWindSpeed > 32.768f)
    {
        mpsWindSpeed = 32.768f;
    }
    if (mpsWindSpeed < -32.768f)
    {
        mpsWindSpeed = -32.768f;
    }

    // the wind speed is transmitted in 1/1000 mps resolution.
    mpsWindSpeed = mpsWindSpeed + 32.768f;
    uint16_t usSpeed = (uint16_t)(mpsWindSpeed * 1000);

    // frame the ANT message.
    pucBuffer[ucOffset++] = WF_KICKR_COMMAND_PAGE;
    pucBuffer[ucOffset++] = (uint8_t)WF_WCPMCP_OPCODE_TRAINER_SET_WIND_SPEED;
    pucBuffer[ucOffset++] = (uint8_t)usDeviceId;
    pucBuffer[ucOffset++] = (uint8_t)(usDeviceId >> 8);
    pucBuffer[ucOffset++] = (uint8_t)usSpeed;
    pucBuffer[ucOffset++] = (uint8_t)(usSpeed >> 8);
    pucBuffer[ucOffset++] = ANTPLUS_RESERVED;
    pucBuffer[ucOffset++] = ANTPLUS_RESERVED;
}

void KickrAnt::EncodeTrainerSetWheelCircumference(uint8_t* pucBuffer, uint16_t usDeviceId, float mmCircumference)
{
    uint8_t ucOffset = 0;

    // convert values to 16-bit unsigned integer.
    //
    // wheel circmference is transmitted in 1/10 mm resolution.
    uint16_t usCirc = (uint16_t)(mmCircumference * 10.0);

    // frame the ANT message.
    pucBuffer[ucOffset++] = WF_KICKR_COMMAND_PAGE;
    pucBuffer[ucOffset++] = (uint8_t)WF_WCPMCP_OPCODE_TRAINER_SET_WHEEL_CIRCUMFERENCE;
    pucBuffer[ucOffset++] = (uint8_t)usDeviceId;
    pucBuffer[ucOffset++] = (uint8_t)(usDeviceId >> 8);
    pucBuffer[ucOffset++] = (uint8_t)usCirc;
    pucBuffer[ucOffset++] = (uint8_t)(usCirc >> 8);
    pucBuffer[ucOffset++] = ANTPLUS_RESERVED;
    pucBuffer[ucOffset++] = ANTPLUS_RESERVED;
}

void KickrAnt::EncodeTrainerReadMode(uint8_t* pucBuffer, uint16_t usDeviceId)
{
    uint8_t ucOffset = 0;

    // frame the ANT message.
    pucBuffer[ucOffset++] = WF_KICKR_COMMAND_PAGE;
    pucBuffer[ucOffset++] = (uint8_t)WF_WCPMCP_OPCODE_TRAINER_READ_MODE;
    pucBuffer[ucOffset++] = (uint8_t)usDeviceId;
    pucBuffer[ucOffset++] = (uint8_t)(usDeviceId >> 8);
    pucBuffer[ucOffset++] = ANTPLUS_RESERVED;
    pucBuffer[ucOffset++] = ANTPLUS_RESERVED;
    pucBuffer[ucOffset++] = ANTPLUS_RESERVED;
    pucBuffer[ucOffset++] = ANTPLUS_RESERVED;
}

void KickrAnt::EncodeTrainerInitSpindown(uint8_t* pucBuffer, uint16_t usDeviceId)
{
    uint8_t ucOffset = 0;

    // frame the ANT message.
    pucBuffer[ucOffset++] = WF_KICKR_COMMAND_PAGE;
    pucBuffer[ucOffset++] = (uint8_t)WF_WCPMCP_OPCODE_TRAINER_INIT_SPINDOWN;
    pucBuffer[ucOffset++] = (uint8_t)usDeviceId;
    pucBuffer[ucOffset++] = (uint8_t)(usDeviceId >> 8);
    pucBuffer[ucOffset++] = ANTPLUS_RESERVED;
    pucBuffer[ucOffset++] = ANTPLUS_RESERVED;
    pucBuffer[ucOffset++] = ANTPLUS_RESERVED;
    pucBuffer[ucOffset++] = ANTPLUS_RESERVED;
}


