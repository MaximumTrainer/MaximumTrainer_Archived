#include "fec_controller.h"

#include <QDebug>
#include "antplus.h"
#include "myconstants.h"


#define CALIBRATION_PAGE_1                 0x01
#define CALIBRATION_PAGE_2                 0x02

#define ANT_GENERAL_FE_DATA                0x10
#define ANT_SPECIFIC_TRAINER_DATA          0x19
#define ANT_SPECIFIC_TRAINER_TORQUE_DATA   0x1A


#define PI_CONST                 3.141592653589793


FEC_Controller::~FEC_Controller() {
}



FEC_Controller::FEC_Controller(int userNb, QObject *parent) : QObject(parent) {

    this->userNb = userNb;

    alreadyShownBatteryWarning = false;
    numberLap = 0;
    lapToogleSet = false;
    wheelSizeMeters = 2.12;


    firstValueAccumulatedPower = true;
    lastAccumulatedPower = 0;
    lastEventCount0x19 = 0;
    accumulatedEventCount = 0;
    totalAccumulatedPower = 0;


    firstPage0x1A = true;
    lastEventCount0x1A = 0;
    lastWheelTick = 0;
    lastWheelPeriod = 0;
    lastAccumulatedTorque = 0;
    noVelocityOccured = 0;

}




/// Send FE-C Data Page 48 (0x30) – Basic Resistance (p.66 D000001231_-_ANT+_Device_Profile_-_Fitness_Equipment_-_Rev_4.1.pdf)
///////////////////////////////////////////////////////////////////////////////////////////////////
void FEC_Controller::EncodeTrainerBasicResistance(uint8_t* pucBuffer, uint8_t percentage)  {


    //convert watts to 0.25W units
    uint8_t percentageUnit = percentage*2;

    uint8_t ucOffset = 0;

    //     qDebug() << "ok set trainer load to" << percentage << "or" << percentageUnit << "0.5%";

    // frame the ANT message.
    pucBuffer[ucOffset++] =  0x30;
    pucBuffer[ucOffset++] =  0xFF;
    pucBuffer[ucOffset++] =  0xFF;
    pucBuffer[ucOffset++] =  0xFF;
    pucBuffer[ucOffset++] =  0xFF;
    pucBuffer[ucOffset++] =  0xFF;
    pucBuffer[ucOffset++] =  0xFF;
    pucBuffer[ucOffset++] =  percentageUnit;

}


/// Send FE-C Data Page 49 (0x31) – Target Power (p.67 D000001231_-_ANT+_Device_Profile_-_Fitness_Equipment_-_Rev_4.1.pdf)
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void FEC_Controller::EncodeTrainerTargetPower(uint8_t* pucBuffer, uint16_t watts)  {


    //convert watts to 0.25W units
    uint16_t wattsUnit = watts*4;

    uint8_t ucOffset = 0;

    //     qDebug() << "ok set trainer load to" << watts << "or" << wattsUnit << "0.25w";

    // frame the ANT message.
    pucBuffer[ucOffset++] =  0x31;
    pucBuffer[ucOffset++] =  0xFF;
    pucBuffer[ucOffset++] =  0xFF;
    pucBuffer[ucOffset++] =  0xFF;
    pucBuffer[ucOffset++] =  0xFF;
    pucBuffer[ucOffset++] =  0xFF;
    pucBuffer[ucOffset++] = (uint8_t)wattsUnit;
    pucBuffer[ucOffset++] = (uint8_t)(wattsUnit >> 8);

}




//Data Page 50 (0x32) – Wind Resistance
//windResistCoef == CdA * Air density (1.225)  - Unit : 0.01 kg/m
//windSpeed (kmh)  0x7F = 0
//draftingFactor = 1 (no drafting) default value 0xFF
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void FEC_Controller::EncodeTrainerWindResistance(uint8_t* pucBuffer, uint8_t windResistCoef, uint8_t windSpeed, uint8_t draftingFactor) {



    uint8_t ucOffset = 0;
    //     qDebug() << "ok set trainer load to" << watts << "or" << wattsUnit << "0.25w";

    // frame the ANT message.
    pucBuffer[ucOffset++] =  0x32;
    pucBuffer[ucOffset++] =  0xFF;
    pucBuffer[ucOffset++] =  0xFF;
    pucBuffer[ucOffset++] =  0xFF;
    pucBuffer[ucOffset++] =  0xFF;

    //Wind Resistance Coefficient - Product of Frontal Surface Area, Drag Coefficient and Air Density. Use default value: 0xFF - 0.01 kg/m 0.00 – 1.86
    pucBuffer[ucOffset++] =  windResistCoef;
    //Speed of simulated wind acting on the cyclist. (+) – Head Wind (–) – Tail Wind Use default value: 0xFF km/h -127 – +127 km/h
    pucBuffer[ucOffset++] = windSpeed; //default 0xFF;
    //Drafting Factor 1 Byte Simulated drafting scale factor Use default value: 0xFF 0.01 0 – 1.00
    pucBuffer[ucOffset++] = draftingFactor; //default 0xFF;

}


//Data Page 51 (0x33) – Track Resistance
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void FEC_Controller::EncodeTrainerTrackResistancePercentage(uint8_t* pucBuffer, double percSlope) {


    uint16_t encodedSlope = percSlope * 100 + 20000;



    uint8_t ucOffset = 0;
    //     qDebug() << "ok set trainer load to" << watts << "or" << wattsUnit << "0.25w";

    // frame the ANT message.
    pucBuffer[ucOffset++] =  0x33;
    pucBuffer[ucOffset++] =  0xFF;
    pucBuffer[ucOffset++] =  0xFF;
    pucBuffer[ucOffset++] =  0xFF;
    pucBuffer[ucOffset++] =  0xFF;

    //Grade of simulated track Invalid, use default value: 0xFFFF 0.01 % -200.00% – 200.00%
    pucBuffer[ucOffset++] = (uint8_t)encodedSlope;
    pucBuffer[ucOffset++] = (uint8_t)(encodedSlope >> 8);

    //Coefficient of rolling resistance between bicycle tires and track terrain (dimensionless) Use default value: 0xFF - 5x10-5
    pucBuffer[ucOffset++] =  0xFF; //default to  constants::GLOBAL_CONST_DEFAULT_ROLLING_RESISTANCE

}







////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void FEC_Controller::decodeFecMessage(ANT_MESSAGE stMessage) {


    //UCHAR ucDataOffset = MESSAGE_BUFFER_DATA2_INDEX;   // For most data messages

    //qDebug() << "FEC_MESSAGE: [" << hex << "[" << hex << stMessage.aucData[1] << "]" << "[" << hex << stMessage.aucData[2] << "]" << "[" << hex << stMessage.aucData[3] << "]" << "[" << hex << stMessage.aucData[4] << "]" << "[" << hex << stMessage.aucData[5] << "]" << "[" << hex << stMessage.aucData[6] << "]" << "[" << hex << stMessage.aucData[7]  << "]" << "[" << hex << stMessage.aucData[8] << "]";





    UCHAR data_page = stMessage.aucData[1];

    switch (data_page) {

    //////////////////////////////////////////////////////////////////////////////////////////////////////////////
    case CALIBRATION_PAGE_1:
    {

        //        CALIBRATION_PAGE_1: [ [ 1 ] [ 40 ] [ 0 ] [ 32 ] [ f4 ] [ 1 ] [ f4 ] [ 1 ]
        qDebug() << "CALIBRATION_PAGE_1: [" << hex << "[" << hex << stMessage.aucData[1] << "]" << "[" << hex << stMessage.aucData[2] << "]" << "[" << hex << stMessage.aucData[3] << "]" << "[" << hex << stMessage.aucData[4] << "]" << "[" << hex << stMessage.aucData[5] << "]" << "[" << hex << stMessage.aucData[6] << "]" << "[" << hex << stMessage.aucData[7]  << "]" << "[" << hex << stMessage.aucData[8] << "]";

        uint8_t calibrationResponse = stMessage.aucData[2];

        //Unit temperature in degrees Celsius with an offset of -25degC 0xFF indicates invalid
        uint8_t temperature = stMessage.aucData[4];
        //Zero offset indication 0xFFFF indicates invalid
        uint16_t zeroOffset = stMessage.aucData[5] + (stMessage.aucData[6]<<8);
        //Spin-Down time in ms 0xFFFF indicates invalid
        uint16_t spinDownTimeMs = stMessage.aucData[7] + (stMessage.aucData[8]<<8);


        bool usingZeroOffset = false;
        bool usingSpinDown = false;

        //ZERO-OFFSET
        if (calibrationResponse == 64) {
            usingZeroOffset = true;
        }
        //SPINDOWN
        else if (calibrationResponse == 128) {
            usingSpinDown = true;
        }
        // BOTH (ZERO-OFFSET AND SPINDOWN)
        else if (calibrationResponse == 192) {
            usingZeroOffset = true;
            usingSpinDown = true;
        }
        // INVALID
        else {
            usingZeroOffset = false;
            usingSpinDown = false;
        }


        double realTemperature = -1; //INVALID FLAG = NOT USED
        if (temperature != 0xFF) {
            realTemperature = (temperature/2.0) -25;
        }
        double zeroOffset2 = -1;
        if (zeroOffset != 0xFFFF) {
            zeroOffset2 = zeroOffset;
        }
        double spinDownTimeMs2 = -1;
        if (spinDownTimeMs != 0xFFFF) {
            spinDownTimeMs2 = spinDownTimeMs;
        }


        //qDebug() << "Temperature:" << temperature << "realTemperature" << realTemperature << "zeroOffset2:" << zeroOffset2 << "spinDownTimeMs2:" << spinDownTimeMs2;
        emit calibrationOver(usingZeroOffset, usingSpinDown, realTemperature, zeroOffset2, spinDownTimeMs2);

        break;
    }


        //////////////////////////////////////////////////////////////////////////////////////////////////////////////
    case CALIBRATION_PAGE_2:
    {

        //        CALIBRATION_PAGE_1: [ [ 1 ] [ 40 ] [ 0 ] [ 32 ] [ f4 ] [ 1 ] [ f4 ] [ 1 ]
        qDebug() << "CALIBRATION_PAGE_2: [" << hex << "[" << hex << stMessage.aucData[1] << "]" << "[" << hex << stMessage.aucData[2] << "]" << "[" << hex << stMessage.aucData[3] << "]" << "[" << hex << stMessage.aucData[4] << "]" << "[" << hex << stMessage.aucData[5] << "]" << "[" << hex << stMessage.aucData[6] << "]" << "[" << hex << stMessage.aucData[7]  << "]" << "[" << hex << stMessage.aucData[8] << "]";

        uint8_t calibrationStatus = stMessage.aucData[2];
        uint8_t calibrationCondition = stMessage.aucData[3];
        // Unit temperature in degrees Celsius with an offset of -25degC 0xFF indicates invalid
        uint8_t currentTemperature = stMessage.aucData[4];
        //Minimum speed required to begin spin-down calibration. 0xFFFF indicates invalid
        uint16_t targetSpeed = stMessage.aucData[5] + (stMessage.aucData[6]<<8);
        //Spin-Down time in ms 0xFFFF indicates invalid
        uint16_t targetSpinDownTimeMs = stMessage.aucData[7] + (stMessage.aucData[8]<<8);


        // -------- calibrationStatus
        bool pendingZeroOffset = false;
        bool pendingSpinDown = false;

        //ZERO-OFFSET
        if (calibrationStatus == 64) {
            pendingZeroOffset = true;
        }
        //SPINDOWN
        else if (calibrationStatus == 128) {
            pendingSpinDown = true;
        }
        // BOTH (ZERO-OFFSET AND SPINDOWN)
        else if (calibrationStatus == 192) {
            pendingZeroOffset = true;
            pendingSpinDown = true;
        }

        // -------- calibrationCondition (Page 41 of 96)
        TEMPERATURE_CONDITION tempConditon = TEMPERATURE_CONDITION::TEMP_NOT_APPLICABLE;
        SPEED_CONDITION speedCondition = SPEED_CONDITION::SPEED_NOT_APPLICABLE;

        //4-5 Temperature Condition, 6-7 Speed Condition
        bool tempCondition1   = ((calibrationCondition >> 4)  & 0x01);
        bool tempCondition0   = ((calibrationCondition >> 5)  & 0x01);
        bool speedCondition1  = ((calibrationCondition >> 6)  & 0x01);
        bool speedCondition0  = ((calibrationCondition >> 7)  & 0x01);

        //11 Current temperature too high
        if (tempCondition0 && tempCondition1) {
            tempConditon = TEMPERATURE_CONDITION::TEMP_TOO_HIGH;
            qDebug() << "11 Current temperature too high";
        }
        //10 Temperature OK
        else if (tempCondition0 && !tempCondition1) {
            tempConditon = TEMPERATURE_CONDITION::TEMP_OK;
            qDebug() << "10 Temperature OK";
        }
        //01 Current temperature too low
        else if (!tempCondition0 && tempCondition1) {
            tempConditon = TEMPERATURE_CONDITION::TEMP_TOO_LOW;
            qDebug() << "01 Current temperature too low";
        }
        //00 Not Applicable

        //01 Current speed too low
        if (!speedCondition0 && speedCondition1) {
            speedCondition  = SPEED_CONDITION::SPEED_TOO_LOW;
            qDebug() << "01 Current speed too low";
        }
        //10 Speed OK
        else if (speedCondition0 && !speedCondition1) {
            speedCondition  = SPEED_CONDITION::SPEED_OK;
            qDebug() << "10 Speed OK";
        }
        //00 Not Applicable, 11 Reserved. Do not use.


        // -----------currentTemperature
        double realTemperature = -100; //INVALID FLAG = NOT USED
        if (currentTemperature != 0xFF) {
            realTemperature = (currentTemperature/2.0) -25;
            qDebug() << "CUrrent Temperature is" << currentTemperature << "realTemperature:" << realTemperature;
        }
        double targetSpeedKMH = -1; //INVALID FLAG = NOT USED
        if (targetSpeed != 0xFFFF) {
            targetSpeedKMH = targetSpeed * 0.0036;
            qDebug() << "Target Speed is " << targetSpeed << "inKPH:" << targetSpeedKMH;
        }
        double targetSpinDownMs = -1;
        if (targetSpinDownTimeMs != 0xFFFF) {
            targetSpinDownMs = targetSpinDownTimeMs;
            qDebug() << "targetSpinDownTimeMs is " << targetSpinDownTimeMs;
        }


        emit calibrationInProgress(pendingSpinDown, pendingZeroOffset,
                                   tempConditon, speedCondition,
                                   realTemperature, targetSpeedKMH, targetSpinDownMs);

        qDebug() << "pendingSpinDown" << pendingSpinDown << "pendingZeroOffset" << pendingZeroOffset << "tempConditon" << tempConditon <<
                    "speedCondition" << speedCondition << "realTemperature" << realTemperature << "targetSpeedKMH" << targetSpeedKMH << "targetSpinDownMs" << targetSpinDownMs;


        //        void calibrationInProgress(bool pendingSpindown, bool pendingOffset,
        //                                   FEC_Controller::TEMPERATURE_CONDITION tempCond, FEC_Controller::SPEED_CONDITION speedCond,
        //                                   double currTemperature, double targetSpeed, double targetSpinDownMs);


        break;
    }

        //////////////////////////////////////////////////////////////////////////////////////////////////////////////
    case ANT_GENERAL_FE_DATA:
    {
        checkFEStateBitField(stMessage);

        //        qDebug() << "ANT_GENERAL_FE_DATA";

        uint16_t instantSpeed = stMessage.aucData[5] + (stMessage.aucData[6]<<8);
        uint8_t instantHeartRate = stMessage.aucData[7];


        //SPEED (M/S)
        if (instantSpeed != 0xFFFF) {
            emit speedChanged(userNb, instantSpeed * 0.0036);
        }
        else {
            emit speedChanged(userNb, -1);
        }

        //HR
        if (instantHeartRate != 0xFF && instantHeartRate > 0) {
            emit hrChanged(userNb, instantHeartRate);
        }
        else {
            emit hrChanged(userNb, -1);
        }

        //        qDebug() << "decodeFecMessage General page data 0x10, usSpeed:" << instantSpeed  << " HR:" << instantHeartRate;
        break;
    }

        //////////////////////////////////////////////////////////////////////////////////////////////////////////////
    case ANT_SPECIFIC_TRAINER_DATA:  //0x19
    {
        checkFEStateBitField(stMessage);
        //        qDebug() << "ANT_SPECIFIC_TRAINER_DATA";

        uint8_t eventCount0x19 = stMessage.aucData[2];
        uint8_t instantCadence = stMessage.aucData[3];
        uint16_t accumulatedPower = stMessage.aucData[4] + (stMessage.aucData[5]<<8);

        uint16_t diffAccumulatedPower = 0;
        uint16_t diffEventCount = 0;


        //--- Check for rollover event
        if (lastAccumulatedPower > accumulatedPower) {
            diffAccumulatedPower = 65536-lastAccumulatedPower + accumulatedPower;
        }
        else {
            diffAccumulatedPower = accumulatedPower - lastAccumulatedPower;
        }
        if (lastEventCount0x19 > eventCount0x19) {
            diffEventCount = 256-lastEventCount0x19 + eventCount0x19;
        }
        else {
            diffEventCount = eventCount0x19 - lastEventCount0x19;
        }


        // --- Trainer Status
        bool powerMeasurementCalibrationRequired = ((stMessage.aucData[7] >> 4)  & 0x01);  //Bicycle power measurement (i.e. Zero Offset) calibration required
        bool resistanceCalibrationRequired = ((stMessage.aucData[7] >> 5)  & 0x01);  //Resistance calibration (i.e. Spin-Down Time) required
        bool userConfigurationRequired = ((stMessage.aucData[7] >> 6)  & 0x01);  //User configuration required

        //qDebug() <<  "powerMeasurementCalibrationRequired" << powerMeasurementCalibrationRequired << "resistanceCalibrationRequired" << resistanceCalibrationRequired << "userConfigurationRequired" << userConfigurationRequired;


        // --- Trainer Flag
        // 0 – Trainer operating at the target power, or no target power set.
        // 1 – User’s cycling speed is too low to achieve target power.
        // 2 – User’s cycling speed is too high to achieve target power.
        // 3 – Undetermined (maximum or minimum) target power limit reached.
        int trainerFlag = 0;
        bool twoValue  = ((stMessage.aucData[8] >> 1)  & 0x01); //2^1
        bool oneValue  = ((stMessage.aucData[8] >> 0)  & 0x01); //2^0

        if (twoValue)
            trainerFlag += 2;
        if (oneValue)
            trainerFlag += 1;

        //        qDebug() << "trainerFlag" << trainerFlag;



        if (instantCadence != 0xFF) {
            instantCadence = stMessage.aucData[3];
            emit cadenceChanged(userNb, instantCadence);
        }
        else {
            emit cadenceChanged(userNb, -1);
        }


        if (!firstValueAccumulatedPower && diffEventCount > 0) {
            // Check if Instant power is not 0xFFF (invalid)
            uint16_t total4bits = 0;

            bool height2 = ((stMessage.aucData[7] >> 0)  & 0x01);   //2^8
            bool neight2  = ((stMessage.aucData[7] >> 1)  & 0x01);  //2^9
            bool ten2  = ((stMessage.aucData[7] >> 2)  & 0x01);     //2^10
            bool eleven2  = ((stMessage.aucData[7] >> 3)  & 0x01);  //2^11

            if (height2)
                total4bits += 256;
            if (neight2)
                total4bits += 512;
            if (ten2)
                total4bits += 1024;
            if (eleven2)
                total4bits += 2048;

            //                    uint16_t instantPower4degit = (stMessage.aucData[7]<<8);
            uint32_t instantPower = stMessage.aucData[6] + total4bits;


            //Calculate AVG power
            double avgPower = diffAccumulatedPower / diffEventCount;

            if (instantPower == 0xFFF || avgPower == 0xFFF) {
                emit powerChanged(userNb, -1);
            }
            else {
                emit powerChanged(userNb, qRound(avgPower));
            }

            //            qDebug () << "INSTANT POWER IS" << instantPower << "AVG POWER IS" << avgPower;
        }



        lastEventCount0x19 = eventCount0x19;
        lastAccumulatedPower = accumulatedPower;
        firstValueAccumulatedPower = false;

        break;
    }
        //////////////////////////////////////////////////////////////////////////////////////////////////////////////
    case ANT_SPECIFIC_TRAINER_TORQUE_DATA:
    {
        checkFEStateBitField(stMessage);

        qDebug() << "ANT_SPECIFIC_TRAINER_TORQUE_DATA";

        uint8_t eventCount0x1A = stMessage.aucData[2];
        uint8_t wheelTick = stMessage.aucData[3];
        uint16_t wheelPeriod = stMessage.aucData[4] + (stMessage.aucData[5]<<8);
        uint16_t accumulatedTorque = stMessage.aucData[6] + (stMessage.aucData[7]<<8);


        //        qDebug () << "accumulated eventCount0x1A.." << eventCount0x1A;

        uint16_t diffAccumulatedTorque = 0;
        uint16_t diffAccumulatedWheelPeriod = 0;
        uint16_t diffEventCount = 0;


        //--- Check for rollover event
        if (lastAccumulatedTorque > accumulatedTorque) {
            diffAccumulatedTorque = 65536-lastAccumulatedTorque + accumulatedTorque;
        }
        else {
            diffAccumulatedTorque = accumulatedTorque - lastAccumulatedTorque;
        }
        if (lastWheelPeriod > wheelPeriod) {
            diffAccumulatedWheelPeriod = 65536-lastWheelPeriod + wheelPeriod;
        }
        else {
            diffAccumulatedWheelPeriod = wheelPeriod - lastWheelPeriod;
        }
        if (lastEventCount0x1A > eventCount0x1A) {
            diffEventCount = 256-lastEventCount0x1A + eventCount0x1A;
        }
        else {
            diffEventCount = eventCount0x1A - lastEventCount0x1A;
        }




        // nothing changed
        if (wheelTick == lastWheelTick && wheelPeriod == lastWheelPeriod) {
            noVelocityOccured ++;
            // Shows 0 cadence, 0 speed, 0 power
            if (noVelocityOccured > 10) {
                emit speedChanged(userNb, 0);
                emit cadenceChanged(userNb, 0);
                emit powerChanged(userNb, 0);
            }
        }
        else if (!firstPage0x1A) {
            noVelocityOccured = 0;
            // Calculate Speed
            double speedKPH = 3.6 * ( (wheelSizeMeters * diffEventCount)/(diffAccumulatedWheelPeriod/2048.0) );
            qDebug () << "speedKPH" << speedKPH;
            emit speedChanged(userNb, speedKPH);

            // Compute Power
            double powerAverage = (128*PI_CONST *diffAccumulatedTorque)/diffAccumulatedWheelPeriod;
            emit powerChanged(userNb, powerAverage);
        }



        lastEventCount0x1A = eventCount0x1A;
        lastWheelTick = wheelTick;
        lastWheelPeriod = wheelPeriod;
        lastAccumulatedTorque = accumulatedTorque;
        firstPage0x1A = false;
        break;
    }
        ////////////////////////////////////////////////////////////////////
        //    case GLOBAL_PAGE_82:
        //    {

        //        uint8_t batteryStatus = COMMON82_BATT_STATUS(stMessage.aucData[8]);

        //        //---------------------------------
        //        switch (batteryStatus)
        //        {
        //        case GBL82_BATT_STATUS_NEW:
        //        {
        //            break;
        //        }
        //        case GBL82_BATT_STATUS_GOOD:
        //        {
        //            break;
        //        }
        //        case GBL82_BATT_STATUS_OK:
        //        {
        //            break;
        //        }
        //        case GBL82_BATT_STATUS_LOW:
        //        {
        //            if (!alreadyShownBatteryWarning) {
        //                emit batteryLow(tr("FE-C Trainer"), 1);
        //                alreadyShownBatteryWarning = true;
        //            }
        //            break;
        //        }
        //        case GBL82_BATT_STATUS_CRITICAL:
        //        {
        //            if (!alreadyShownBatteryWarning) {
        //                emit batteryLow(tr("FE-C Trainer"), 0);
        //                alreadyShownBatteryWarning = true;
        //            }
        //            break;
        //        }
        //        default:
        //        {
        //            qDebug() << "battery invalid status";
        //            break;
        //        }
        //        }
        //        //--------------------

        //    }
    default:
    {
        //        qDebug() << "decodeFecMessage Default break";
        break;
    }


    }





}


/////////////////////////////////////////////////////////////////////////
void FEC_Controller::checkFEStateBitField(ANT_MESSAGE stMessage) {

    /// FE State Bit Field - Present in all FE message:
    /// 0 Reserved
    /// 1 ASLEEP (OFF)
    /// 2 READY
    /// 3 IN_USE
    /// 4 FINISHED (PAUSED)
    /// 5-7 Reserved. Do not send or interpret
    int feState = 0;

    bool fourValue = ((stMessage.aucData[8] >> 6)  & 0x01); //2^2
    bool twoValue  = ((stMessage.aucData[8] >> 5)  & 0x01); //2^1
    bool oneValue  = ((stMessage.aucData[8] >> 4)  & 0x01); //2^0

    if (fourValue)
        feState += 4;
    if (twoValue)
        feState += 2;
    if (oneValue)
        feState += 1;

    //    qDebug() << "FE STATE IS" << feState;


    // Detect if lap toogle activated
    if (!lapToogleSet) {
        lapToogleValue = ((stMessage.aucData[8] >> 7)  & 0x01);
        lapToogleSet = true;
    }
    else {
        bool newLapToogleValue = ((stMessage.aucData[8] >> 7)  & 0x01);
        if (newLapToogleValue != lapToogleValue) {
            numberLap++;
            qDebug() << "NEW LAP!" << numberLap;
            emit lapChanged(userNb);
            lapToogleValue = newLapToogleValue;
        }
    }


}


