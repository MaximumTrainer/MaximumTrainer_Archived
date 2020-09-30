#include "speed_controller.h"
#include <QDebug>
#include "antplus.h"

#define MESSAGE_BUFFER_DATA2_INDEX ((UCHAR) 1)




////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// decodeSpeedMessage
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void Speed_Controller::decodeSpeedMessage(ANT_MESSAGE stMessage) {

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
        case BSC_PAGE_1:
        {
            stPage1Data.ulOperatingTime  = (ULONG)stMessage.aucData[ucDataOffset+1];
            stPage1Data.ulOperatingTime |= (ULONG)stMessage.aucData[ucDataOffset+2] << 8;
            stPage1Data.ulOperatingTime |= (ULONG)stMessage.aucData[ucDataOffset+3] << 16;
            stPage1Data.ulOperatingTime *= 2;

            //            qDebug() << "PAGE1 : OperatingTime:" << stPage1Data.ulOperatingTime;
            break;
        }
        case BSC_PAGE_2:
        {
            stPage2Data.ucManId = stMessage.aucData[ucDataOffset + 1];
            //            stPage2Data.ulSerialNumber  = (ULONG)usDeviceNumber;
            stPage2Data.ulSerialNumber |= (ULONG)stMessage.aucData[ucDataOffset+2] << 16;
            stPage2Data.ulSerialNumber |= (ULONG)stMessage.aucData[ucDataOffset+3] << 24;

            //            qDebug() << "PAGE2 : ManId:" << stPage2Data.ucManId;
            //            qDebug() << "Serial Number:" << stPage2Data.ulSerialNumber;
            break;
        }
        case BSC_PAGE_3:
        {
            stPage3Data.ucHwVersion   = (ULONG)stMessage.aucData[ucDataOffset+1];
            stPage3Data.ucSwVersion   = (ULONG)stMessage.aucData[ucDataOffset+2];
            stPage3Data.ucModelNumber = (ULONG)stMessage.aucData[ucDataOffset+3];
            break;
        }

            /*
        case BSC_PAGE_4:  //Battery Status
        {
            qDebug() << "BATTERY STATUS NOW!";
            ULONG ucBatteryStatus;

            ucBatteryStatus   = (ULONG)stMessage.aucData[ucDataOffset+2];

            //battery status is bit 4-6 of ucBatteryStatus


            if (ucBatteryStatus == 4) {
                qDebug() << "Battery status low!";
            }
            else if (ucBatteryStatus == 5) {
                qDebug() << "Battery status critical!";
            }
            break;
        }
        */

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
    currentPage0.usLastTime1024        = (USHORT)stMessage.aucData[ucDataOffset+4];
    currentPage0.usLastTime1024       |= (USHORT)(stMessage.aucData[ucDataOffset+5] << 8);
    currentPage0.usCumSpeedRevCount    = (USHORT)stMessage.aucData[ucDataOffset+6];
    currentPage0.usCumSpeedRevCount   |= (USHORT)(stMessage.aucData[ucDataOffset+7] << 8);


    /// Check if sensor stopped using
    if (currentPage0.usLastTime1024 == pastPage0.usLastTime1024 && currentPage0.usCumSpeedRevCount  == pastPage0.usCumSpeedRevCount) {
        dataNotChanged++;
        if (dataNotChanged > MAX_MSG_REPEAT_SPEED) {
            emit speedChanged(userNb, 0);
            if (powerCurve.getId() > 0) {
                emit powerChanged(userNb, 0);
            }
        }
    }
    else {
        dataNotChanged = 0;
        /// Compute new Speed
        computeSpeed();
    }


    /// Update accumulated values, handles rollovers
    ulBSAccumRevs += (ULONG)((currentPage0.usCumSpeedRevCount - pastPage0.usCumSpeedRevCount) & MAX_USHORT);

    /// Move current data to the past
    pastPage0.usCumSpeedRevCount   = currentPage0.usCumSpeedRevCount;
    pastPage0.usLastTime1024       = currentPage0.usLastTime1024;
}



////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// computeSpeed
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void Speed_Controller::computeSpeed() {


    ULONG ulFinalSpeed;
    USHORT usDeltaTime;

    //rollover protection
    usDeltaTime = (currentPage0.usLastTime1024 - pastPage0.usLastTime1024) & MAX_USHORT;

    if (usDeltaTime > 0) //divide by zero
    {
        //rollover protection
        ulFinalSpeed = (ULONG)((currentPage0.usCumSpeedRevCount - pastPage0.usCumSpeedRevCount) & MAX_USHORT);
        ulFinalSpeed *= (ULONG)(1024); //circumference for numerator, 1024/((1/1024)s) in the denominator

        stSpeedData.usFracValue = (USHORT)(((ulFinalSpeed % (ULONG)usDeltaTime) * CBSC_PRECISION) / usDeltaTime);
        ulFinalSpeed /= (ULONG)(usDeltaTime);

        stSpeedData.ulIntValue   = ulFinalSpeed;

        computeSpeedHelper();
    }

    //maintain old data if time change not detected, but update delta time
    stSpeedData.usDeltaValue = usDeltaTime;

}


/// computerSpeedHelper
void Speed_Controller::computeSpeedHelper() {


    double frac = stSpeedData.usFracValue * 0.001;
    double total = stSpeedData.ulIntValue + frac;

    //    double speedKMH = total * wheelSize * 3600 /1000000; //Vrai formule = tourRoue/sec * wheelSize_cm * 10^-6 * 3600
    double speedKMH = total * wheelSize * 0.0036;


    // EMIT Speed -  ingore invalids spike
    if (!firstMessageNow && speedKMH < MAX_SPEED) {

        emit speedChanged(userNb, speedKMH);

        if (powerCurve.getId() > 0) {
            double power = powerCurve.calculatePower(speedKMH);
            if (power < MAX_POWER) {
              emit powerChanged(userNb, power);
            }
        }
    }

    firstMessageNow = false;
}







/////////////////////////////////////////////////////////////////////////////////
/// Constructor
/////////////////////////////////////////////////////////////////////////////////
Speed_Controller::Speed_Controller(int userNb, PowerCurve powerCurve, int wheel_circ, QObject *parent) : QObject(parent)  {

    this->userNb = userNb;

    // Initialize app variables
    currentPage0.usCumSpeedRevCount = 0;
    currentPage0.usLastTime1024 = 0;
    pastPage0.usCumSpeedRevCount = 0;
    pastPage0.usLastTime1024 = 0;

    stSpeedData.ulIntValue = 0;
    stSpeedData.usDeltaValue = 0;
    stSpeedData.usFracValue = 0;

    stPage1Data.ulOperatingTime = 0;
    stPage2Data.ucManId = 0;
    stPage2Data.ulSerialNumber = 0;
    stPage3Data.ucHwVersion = 0;
    stPage3Data.ucSwVersion = 0;
    stPage3Data.ucModelNumber = 0;

    ulBSAccumRevs = 0;

    eThePageState = STATE_INIT_PAGE;
    firstMessageNow = true;
    dataNotChanged = 0;


    this->powerCurve = powerCurve;
    this->wheelSize = wheel_circ;




}





