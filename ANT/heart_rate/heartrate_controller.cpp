#include <QDebug>
#include "heartrate_controller.h"
#include "antplus.h"


#define MESSAGE_BUFFER_DATA2_INDEX ((UCHAR) 1)





HeartRate_Controller::~HeartRate_Controller() {
}




HeartRate_Controller::HeartRate_Controller(int userNb, QObject *parent) : QObject(parent)  {

    this->userNb = userNb;

    // Intialize app variables
    stPage0Data.usBeatTime = 0;
    stPage0Data.ucBeatCount = 0;
    stPage0Data.ucComputedHeartRate = 0;
    stPage1Data.ulOperatingTime = 0;
    stPage2Data.ucManId = 0;
    stPage2Data.ulSerialNumber = 0;
    stPage3Data.ucHwVersion = 0;
    stPage3Data.ucSwVersion = 0;
    stPage3Data.ucModelNumber = 0;
    stPage4Data.usPreviousBeat = 0;


    eThePageState = STATE_INIT_PAGE;
    previousHeartBeatCount = 0;
    previousHeartBeatTime = 0;
    dataNotChanged = 0;
    alreadyShownBatteryWarning = false;
}







////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void HeartRate_Controller::decodeHeartRateMessage(ANT_MESSAGE stMessage) {



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

        // INTENTIONAL FALLTHROUGH !!!
    }
    case STATE_EXT_PAGE:
    {
        switch(ucPage & ~TOGGLE_MASK)
        {
        case HRM_PAGE_1:
        {
            stPage1Data.ulOperatingTime  = (ULONG)stMessage.aucData[ucDataOffset+1];
            stPage1Data.ulOperatingTime |= (ULONG)stMessage.aucData[ucDataOffset+2] << 8;
            stPage1Data.ulOperatingTime |= (ULONG)stMessage.aucData[ucDataOffset+3] << 16;
            stPage1Data.ulOperatingTime *= 2;


            if (stPage1Data.ulOperatingTime > 33554432 && !alreadyShownBatteryWarning) {

                USHORT usDeviceNumber = stMessage.aucData[10] | (stMessage.aucData[11] << 8);
                qDebug() << "Emit signal batterylow" << usDeviceNumber;
                emit batteryLow(tr("Heart Rate"), 1, usDeviceNumber);
                alreadyShownBatteryWarning = true;
            }


            break;
        }
        case HRM_PAGE_2:
        {
            stPage2Data.ucManId = stMessage.aucData[ucDataOffset + 1];
            //                stPage2Data.ulSerialNumber  = (ULONG)usDeviceNumber;
            stPage2Data.ulSerialNumber |= (ULONG)stMessage.aucData[ucDataOffset+2] << 16;
            stPage2Data.ulSerialNumber |= (ULONG)stMessage.aucData[ucDataOffset+3] << 24;

            //                qDebug() << "PAGE2 : ManId:" << stPage2Data.ucManId;
            //                qDebug() << "Serial Number:" << stPage2Data.ulSerialNumber;
            break;
        }
        case HRM_PAGE_3:
        {
            stPage3Data.ucHwVersion   = (ULONG)stMessage.aucData[ucDataOffset+1];
            stPage3Data.ucSwVersion   = (ULONG)stMessage.aucData[ucDataOffset+2];
            stPage3Data.ucModelNumber = (ULONG)stMessage.aucData[ucDataOffset+3];

            //                qDebug() << "PAGE3 : HardwareVersion:" << stPage3Data.ucHwVersion;
            //                qDebug() << "SoftwareVersion::" << stPage3Data.ucSwVersion;
            //                qDebug() << "model Number:" <<  stPage3Data.ucModelNumber;
            break;
        }
        case HRM_PAGE_4:
        {
            stPage4Data.usPreviousBeat  = (ULONG)stMessage.aucData[ucDataOffset+2];
            stPage4Data.usPreviousBeat |= (ULONG)stMessage.aucData[ucDataOffset+3] << 8;

            //                qDebug() << "Page4: PreviousBeat time:" << stPage4Data.usPreviousBeat;
            break;
        }
        case HRM_PAGE_0:
        {
            /// Handled above and below, so don't fall-thru to default case
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
    stPage0Data.usBeatTime = (USHORT)stMessage.aucData[ucDataOffset+4];                  // Measurement time
    stPage0Data.usBeatTime |= (USHORT)stMessage.aucData[ucDataOffset+5] << 8;
    stPage0Data.ucBeatCount = (UCHAR)stMessage.aucData[ucDataOffset+6];                  // Measurement count
    stPage0Data.ucComputedHeartRate = (USHORT)stMessage.aucData[ucDataOffset+7];         // Computed heart rate


    //    qDebug() << "Time:" << stPage0Data.usBeatTime;
    //    qDebug() << "CountHR:" << hex <<  stPage0Data.ucBeatCount;
    //    qDebug() << "InstantHR:" << stPage0Data.ucComputedHeartRate;


    /// Check if user has removed the strap or incorrect strap placement on user.
    if ((stPage0Data.ucBeatCount  == previousHeartBeatCount) && (stPage0Data.usBeatTime == previousHeartBeatTime)) {
        dataNotChanged++;
        if (dataNotChanged > MAX_MSG_REPEAT_HR) {
            /// EMIT HR, do not emit 0 in this case, since HR=0 equals death...
            emit HeartRateChanged(userNb, -1);
        }
    }
    else if (stPage0Data.ucComputedHeartRate < MAX_HR && stPage0Data.ucComputedHeartRate > 2) {
        dataNotChanged = 0;
        /// EMIT HR
        emit HeartRateChanged(userNb, stPage0Data.ucComputedHeartRate);
    }

    /// Move current data to the past
    previousHeartBeatCount = stPage0Data.ucBeatCount;
    previousHeartBeatTime = stPage0Data.usBeatTime;




}













