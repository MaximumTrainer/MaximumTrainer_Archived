#include "cadence_controller.h"
#include <QDebug>
#include "antplus.h"

#define MESSAGE_BUFFER_DATA2_INDEX ((UCHAR) 1)





////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// decodeCadenceMessage
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void Cadence_Controller::decodeCadenceMessage(ANT_MESSAGE stMessage) {

    UCHAR ucDataOffset = MESSAGE_BUFFER_DATA2_INDEX;   // For most data messages
    UCHAR ucPage = stMessage.aucData[ucDataOffset];


    switch(eThePageState)
    {
    case STATE_INIT_PAGE:
    {
        eThePageState = STATE_STD_PAGE;
        break;
    }
    case STATE_STD_PAGE:
    {
        // Check if the page if changing, if yes
        // then move to the next state, otherwise
        // only interpret page 0
        if(ucOldPage == ucPage)
            break;
        else
            eThePageState = STATE_EXT_PAGE;

        //intentional fallthrough
    }
    case STATE_EXT_PAGE:
    {
        switch(ucPage & ~TOGGLE_MASK)
        {
        //both BS and BC use the same background pages
        case BSC_PAGE_1:
        {
            stPage1Data.ulOperatingTime  = (ULONG)stMessage.aucData[ucDataOffset+1];
            stPage1Data.ulOperatingTime |= (ULONG)stMessage.aucData[ucDataOffset+2] << 8;
            stPage1Data.ulOperatingTime |= (ULONG)stMessage.aucData[ucDataOffset+3] << 16;
            stPage1Data.ulOperatingTime *= 2;

            //                   qDebug() << "PAGE1 : OperatingTime:" << stPage1Data.ulOperatingTime;
            break;
        }
        case BSC_PAGE_2:
        {
            stPage2Data.ucManId = stMessage.aucData[ucDataOffset + 1];
            //                   stPage2Data.ulSerialNumber  = (ULONG)usDeviceNumber;
            stPage2Data.ulSerialNumber |= (ULONG)stMessage.aucData[ucDataOffset+2] << 16;
            stPage2Data.ulSerialNumber |= (ULONG)stMessage.aucData[ucDataOffset+3] << 24;

            //                   qDebug() << "PAGE2 : ManId:" << stPage2Data.ucManId;
            //                   qDebug() << "Serial Number:" << stPage2Data.ulSerialNumber;
            break;
        }
        case BSC_PAGE_3:
        {
            stPage3Data.ucHwVersion   = (ULONG)stMessage.aucData[ucDataOffset+1];
            stPage3Data.ucSwVersion   = (ULONG)stMessage.aucData[ucDataOffset+2];
            stPage3Data.ucModelNumber = (ULONG)stMessage.aucData[ucDataOffset+3];

            //                   qDebug() << "PAGE3 : HardwareVersion:" << stPage3Data.ucHwVersion;
            //                   qDebug() << "SoftwareVersion::" << stPage3Data.ucSwVersion;
            //                   qDebug() << "model Number:" <<  stPage3Data.ucModelNumber;
            break;
        }
        case BSC_PAGE_0:
        {
            // Handled above and below, so don't fall-thru to default case
            break;
        }
        default:
        {
            /// Unknow page
            break;
        }
        }
        break;
    }
    }
    ucOldPage = ucPage;



    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// DATA PRESENT ON ALL PAGES  (BYTES 4-5-6-7)
    currentPage0.usLastCadence1024     = (USHORT)stMessage.aucData[ucDataOffset+4];
    currentPage0.usLastCadence1024    |= (USHORT)(stMessage.aucData[ucDataOffset+5] << 8);
    currentPage0.usCumCadenceRevCount  = (USHORT)stMessage.aucData[ucDataOffset+6];
    currentPage0.usCumCadenceRevCount |= (USHORT)(stMessage.aucData[ucDataOffset+7] << 8);



    /// Check if sensor cadence stopped using
    if (currentPage0.usLastCadence1024 == pastPage0.usLastCadence1024 && currentPage0.usCumCadenceRevCount  == pastPage0.usCumCadenceRevCount) {
        dataNotChanged++;
        if (dataNotChanged > MAX_MSG_REPEAT_CADENCE) {
            emit cadenceChanged(userNb, 0);
        }
    }
    else {
        dataNotChanged = 0;
        /// Compute new cadence
        computeCadence();
    }



    /// Update accumulated values, handles rollovers
    ulBCAccumCadence += (ULONG)((currentPage0.usCumCadenceRevCount - pastPage0.usCumCadenceRevCount) & MAX_USHORT);

    /// Move current data to the past
    pastPage0.usCumCadenceRevCount = currentPage0.usCumCadenceRevCount;
    pastPage0.usLastCadence1024    = currentPage0.usLastCadence1024;
}



////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// computeCadence
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void Cadence_Controller::computeCadence() {


    ULONG  ulFinalCadence;
    USHORT usDeltaTime;

    //rollover protection
    usDeltaTime = (currentPage0.usLastCadence1024 - pastPage0.usLastCadence1024) & MAX_USHORT;

    if (usDeltaTime > 0) //divide by zero
    {
        //rollover protection
        ulFinalCadence = (ULONG)((currentPage0.usCumCadenceRevCount - pastPage0.usCumCadenceRevCount) & MAX_USHORT);
        ulFinalCadence *= (ULONG)(60); //60 s/min for numerator

        stCadenceData.usFracValue = (USHORT)((((ulFinalCadence * 1024) % (ULONG)usDeltaTime) * CBSC_PRECISION) / usDeltaTime);
        ulFinalCadence = (ULONG)(ulFinalCadence * (ULONG)1024 / usDeltaTime); //1024/((1/1024)s) in the denominator -. RPM
        //...split up from s/min due to ULONG size limit

        stCadenceData.ulIntValue   = ulFinalCadence;

        if (!firstMessageNow)
            emit cadenceChanged(userNb, stCadenceData.ulIntValue);

        if (firstMessageNow)
            firstMessageNow = false;

    }

    //maintain old data if time change not detected, but update delta time
    stCadenceData.usDeltaValue = usDeltaTime;

}




/////////////////////////////////////////////////////////////////////////////////
/// Constructor
/////////////////////////////////////////////////////////////////////////////////
Cadence_Controller::Cadence_Controller(int userNb, QObject *parent) : QObject(parent)  {

    this->userNb = userNb;
    // Initialize values
    currentPage0.usCumCadenceRevCount = 0;
    currentPage0.usLastCadence1024 = 0;
    pastPage0.usCumCadenceRevCount = 0;
    pastPage0.usLastCadence1024 = 0;

    stCadenceData.ulIntValue = 0;
    stCadenceData.usDeltaValue = 0;
    stCadenceData.usFracValue = 0;

    stPage1Data.ulOperatingTime = 0;
    stPage2Data.ucManId = 0;
    stPage2Data.ulSerialNumber = 0;
    stPage3Data.ucHwVersion = 0;
    stPage3Data.ucSwVersion = 0;
    stPage3Data.ucModelNumber = 0;


    ulBCAccumCadence = 0;

    dataNotChanged = 0;
    eThePageState = STATE_INIT_PAGE;
    firstMessageNow = true;
}


