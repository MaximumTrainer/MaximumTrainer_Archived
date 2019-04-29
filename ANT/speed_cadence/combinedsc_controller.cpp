#include "combinedsc_controller.h"
#include <QDebug>
#include "antplus.h"

#define MESSAGE_BUFFER_DATA2_INDEX ((UCHAR) 1)




////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// decodeSpeedCadenceMessage
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CombinedSC_Controller::decodeSpeedCadenceMessage(ANT_MESSAGE stMessage) {

    UCHAR ucDataOffset = MESSAGE_BUFFER_DATA2_INDEX;   // For most data messages


    currentPage0.usLastCadence1024     = (USHORT)stMessage.aucData[ucDataOffset];
    currentPage0.usLastCadence1024    |= (USHORT)(stMessage.aucData[ucDataOffset+1] << 8);
    currentPage0.usCumCadenceRevCount  = (USHORT)stMessage.aucData[ucDataOffset+2];
    currentPage0.usCumCadenceRevCount |= (USHORT)(stMessage.aucData[ucDataOffset+3] << 8);
    currentPage0.usLastTime1024        = (USHORT)stMessage.aucData[ucDataOffset+4];
    currentPage0.usLastTime1024       |= (USHORT)(stMessage.aucData[ucDataOffset+5] << 8);
    currentPage0.usCumSpeedRevCount    = (USHORT)stMessage.aucData[ucDataOffset+6];
    currentPage0.usCumSpeedRevCount   |= (USHORT)(stMessage.aucData[ucDataOffset+7] << 8);


    /// Check if sensor speed stopped using
    if (currentPage0.usLastTime1024 == pastPage0.usLastTime1024 && currentPage0.usCumSpeedRevCount  == pastPage0.usCumSpeedRevCount) {
        speedDataNotChanged++;
        if (!ignoreSpeed && speedDataNotChanged > MAX_MSG_REPEAT_SPEED) {
            emit speedChanged (userNb, 0);
            if (powerCurve.getId() > 0)
                emit powerChanged(userNb, 0);
        }
    }
    else {
        speedDataNotChanged = 0;
        /// Compute new Speed
        computeSpeed();
    }


    /// Check if sensor cadence stopped using
    if (currentPage0.usLastCadence1024 == pastPage0.usLastCadence1024 && currentPage0.usCumCadenceRevCount  == pastPage0.usCumCadenceRevCount) {
        cadenceDataNotChanged++;
        if (!ignoreCadence && cadenceDataNotChanged > MAX_MSG_REPEAT_CADENCE) {
            emit cadenceChanged(userNb, 0);
        }
    }
    else {
        cadenceDataNotChanged = 0;
        /// Compute new cadence
        computeCadence();
    }




    /// Update accumulated values, handles rollovers
    ulBSAccumRevs += (ULONG)((currentPage0.usCumSpeedRevCount - pastPage0.usCumSpeedRevCount) & MAX_USHORT);
    ulBCAccumCadence += (ULONG)((currentPage0.usCumCadenceRevCount - pastPage0.usCumCadenceRevCount) & MAX_USHORT);

    /// Move current data to the past
    pastPage0.usCumCadenceRevCount = currentPage0.usCumCadenceRevCount;
    pastPage0.usLastCadence1024    = currentPage0.usLastCadence1024;
    pastPage0.usCumSpeedRevCount   = currentPage0.usCumSpeedRevCount;
    pastPage0.usLastTime1024       = currentPage0.usLastTime1024;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// computeCadence
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CombinedSC_Controller::computeCadence() {

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


        /// EMIT CADENCE
        if (!firstMessageCadenceNow && !ignoreCadence  && stCadenceData.ulIntValue < MAX_CAD) {
            emit cadenceChanged(userNb, stCadenceData.ulIntValue);
        }
        if (firstMessageCadenceNow)
            firstMessageCadenceNow = false;
    }

    //maintain old data if time change not detected, but update delta time
    stCadenceData.usDeltaValue = usDeltaTime;

}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// computeSpeed
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CombinedSC_Controller::computeSpeed() {

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
void CombinedSC_Controller::computeSpeedHelper() {


    double frac = stSpeedData.usFracValue * 0.001;
    double total = stSpeedData.ulIntValue + frac;

    //    double speedKMH = total * wheelSize * 3600 /1000000; //Vrai formule = tourRoue/sec * wheelSize_cm * 10^-6 * 3600
    double speedKMH = total * wheelSize * 0.0036;


    // EMIT Speed -  ingore invalids spike
    if (!firstMessageSpeedNow && !ignoreSpeed && speedKMH < MAX_SPEED) {

        emit speedChanged(userNb, speedKMH);

        if (powerCurve.getId() > 0) {
            double power = powerCurve.calculatePower(speedKMH);
            if (power < MAX_POWER) {
              emit powerChanged(userNb, power);
            }
        }
    }

    firstMessageSpeedNow = false;
}








/////////////////////////////////////////////////////////////////////////////////
/// Constructor
/////////////////////////////////////////////////////////////////////////////////
CombinedSC_Controller::CombinedSC_Controller(int userNb, PowerCurve powerCurve, int wheel_circ, QObject *parent) : QObject(parent)  {

    this->userNb = userNb;
    // Initialize values
    stSpeedData.ulIntValue = 0;
    stSpeedData.usFracValue = 0;
    stSpeedData.usDeltaValue = 0;
    stCadenceData.ulIntValue = 0;
    stCadenceData.usFracValue = 0;
    stCadenceData.usDeltaValue = 0;

    pastPage0.usLastCadence1024 = 0;
    pastPage0.usCumCadenceRevCount = 0;
    pastPage0.usLastTime1024 = 0;
    pastPage0.usCumSpeedRevCount = 0;
    currentPage0.usLastCadence1024 = 0;
    currentPage0.usCumCadenceRevCount = 0;
    currentPage0.usLastTime1024 = 0;
    currentPage0.usCumSpeedRevCount = 0;

    speedDataNotChanged = 0;
    cadenceDataNotChanged = 0;
    firstMessageSpeedNow = true;
    firstMessageCadenceNow = true;


    this->powerCurve = powerCurve;
    this->wheelSize = wheel_circ;


    ignoreSpeed = false;
    ignoreCadence = false;


}








