#include "power_controller.h"

#include <QDebug>

#include "antplus.h"



#define MESSAGE_BUFFER_DATA2_INDEX ((UCHAR) 1)


#define ANT_SPORT_CALIBRATION_MESSAGE                 0x01

// kickr
#define KICKR_COMMAND_INTERVAL         60 // every 60 ms
#define KICKR_SET_RESISTANCE_MODE      0x40
#define KICKR_SET_STANDARD_MODE        0x41
#define KICKR_SET_ERG_MODE             0x42
#define KICKR_SET_SIM_MODE             0x43
#define KICKR_SET_CRR                  0x44
#define KICKR_SET_C                    0x45
#define KICKR_SET_GRADE                0x46
#define KICKR_SET_WIND_SPEED           0x47
#define KICKR_SET_WHEEL_CIRCUMFERENCE  0x48
#define KICKR_INIT_SPINDOWN            0x49
#define KICKR_READ_MODE                0x4A
#define KICKR_SET_FTP_MODE             0x4B
// 0x4C-0x4E reserved.
#define KICKR_CONNECT_ANT_SENSOR       0x4F
// 0x51-0x59 reserved.
#define KICKR_SPINDOWN_RESULT          0x5A



#define ANT_STANDARD_POWER     0x10
#define ANT_WHEELTORQUE_POWER  0x11
#define ANT_CRANKTORQUE_POWER  0x12
#define ANT_TE_AND_PS_POWER    0x13
#define ANT_CRANKSRM_POWER     0x20




///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// decodePowerMessage
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void Power_Controller::decodePowerMessage(ANT_MESSAGE stMessage) {






    ///--------------- OLD CODE PROVIDED BY ANT (thisisant.com) Use C style -----------------
    UCHAR ucDataOffset = MESSAGE_BUFFER_DATA2_INDEX;   // For most data messages
    UCHAR ucPage = stMessage.aucData[ucDataOffset];


    switch(ucPage)
    {
    /// 0x13 - torque efficiency and pedal smoothness - extension to standard power
    case ANT_TE_AND_PS_POWER:
    {


        //        uint8_t eventCount0x13 = stMessage.aucData[2];
        uint8_t leftTorqueEffectiveness = stMessage.aucData[3];  //0.5% - 0xFF: Invalid or negative values
        uint8_t rightTorqueEffectiveness = stMessage.aucData[4];
        uint8_t leftPedalSmoothness = stMessage.aucData[5];
        uint8_t rightPedalSmoothness = stMessage.aucData[6];


        double dleftTorqueEffectiveness = -1;
        if (leftTorqueEffectiveness != 0xFF) {
            dleftTorqueEffectiveness = leftTorqueEffectiveness*0.5;
        }
        double drightTorqueEffectiveness = -1;
        if (rightTorqueEffectiveness != 0xFF) {
            drightTorqueEffectiveness = rightTorqueEffectiveness*0.5;
        }

        //        Pedal Smoothness (Page 36)
        //        If the sensor is able to distinguish between the left and right leg’s Pedal Smoothness values, then the appropriate values
        //        should be sent according to Table 11-1. If the Pedal Smoothness is calculated as a combined value across both pedals, then
        //        the resulting value should be sent in byte 4, and byte 5 should be set to 0xFE. If a single sensor is in use that measures
        //        either left or right pedal smoothness, then pedal smoothness should be sent in either the byte 4 or byte 5 respectively, and
        //        the unused byte should be set to invalid (0xFF).
        bool combinenedPedalSmoothness = false;
        double dleftPedalSmoothness = -1;
        double drightPedalSmoothness = -1;
        double dcombinedPedalSmoothness = -1;
        if (rightPedalSmoothness == 0xFE) {
            combinenedPedalSmoothness = true;
            if (leftPedalSmoothness != 0xFF) {
                dcombinedPedalSmoothness = leftPedalSmoothness*0.5;
            }
        }
        else {
            if (leftPedalSmoothness != 0xFF) {
                dleftPedalSmoothness = leftPedalSmoothness*0.5;
            }
            if (rightPedalSmoothness != 0xFF) {
                drightPedalSmoothness = rightPedalSmoothness*0.5;
            }
        }


        emit pedalMetricChanged(userNb, dleftTorqueEffectiveness, drightTorqueEffectiveness,
                                dleftPedalSmoothness, drightPedalSmoothness, dcombinedPedalSmoothness);


        //        qDebug() << "eventCount0x13" << eventCount0x13 << "leftTorqueEffectiveness" << leftTorqueEffectiveness << "dleftTorqueEffectiveness" << dleftTorqueEffectiveness <<
        //                    "rightTorqueEffectiveness" << rightTorqueEffectiveness <<
        //                    "leftPedalSmoothness" << leftPedalSmoothness << "rightPedalSmoothness" << rightPedalSmoothness;

        break;
    }


        //---- Calibration Message - 0x01
    case BPS_PAGE_1:
    {

        //        qDebug() << "BPS_PAGE_1****";

        pstPage1Data.ucCalibrationID = stMessage.aucData[ucDataOffset+1];

        switch(pstPage1Data.ucCalibrationID)
        {
        case PBS_CID_18: // CID 0x12 Torque Sensor Capabilities Main Data Page
        {


            pstPage1Data.stBPSCalibrationData.stBPSPage1_CID18_Data.ucCID18_SensorConfiguration_EN = PAGE1_CID18_AutoZero_ENABLE(stMessage.aucData[ucDataOffset+2]);
            pstPage1Data.stBPSCalibrationData.stBPSPage1_CID18_Data.ucCID18_SensorConfiguration_STATUS = PAGE1_CID18_AutoZero_STATUS(stMessage.aucData[ucDataOffset+2]);
            pstPage1Data.stBPSCalibrationData.stBPSPage1_CID18_Data.ssCID18_RawTorqueCount = (SSHORT)stMessage.aucData[ucDataOffset+3];
            pstPage1Data.stBPSCalibrationData.stBPSPage1_CID18_Data.ssCID18_RawTorqueCount |= (SSHORT)(stMessage.aucData[ucDataOffset+4]<<8);
            pstPage1Data.stBPSCalibrationData.stBPSPage1_CID18_Data.ssCID18_OffsetTorque = (SSHORT)stMessage.aucData[ucDataOffset+5];
            pstPage1Data.stBPSCalibrationData.stBPSPage1_CID18_Data.ssCID18_OffsetTorque |= (SSHORT)(stMessage.aucData[ucDataOffset+6]<<8);
            //             pstPage1Data.stBPSCalibrationData.stBPSPage1_CID18_Data.ucCID18_ReserveByte7 = stMessage.aucData[ucDataOffset+7];

            //            qDebug() << "SUPPORT AUTOZERO? " <<   pstPage1Data.stBPSCalibrationData.stBPSPage1_CID18_Data.ucCID18_SensorConfiguration_EN;
            //            qDebug() << "SUPPORT AUTOZERO2? " <<  pstPage1Data.stBPSCalibrationData.stBPSPage1_CID18_Data.ucCID18_SensorConfiguration_STATUS;


            if (pstPage1Data.stBPSCalibrationData.stBPSPage1_CID18_Data.ucCID18_SensorConfiguration_EN==1)
            {
                if (pstPage1Data.stBPSCalibrationData.stBPSPage1_CID18_Data.ucCID18_SensorConfiguration_STATUS==1)
                {
                    //                    qDebug() << "support AutoZero On";
                    ucLocalAutoZeroStatus = 0x01;
                    ///Emit AutO ZERO ON, display button "Change Auto Zero Config"
                    emit supportAutoZero();
                }
                else
                {
                    //                    qDebug() << "support AutoZero Off";
                    ucLocalAutoZeroStatus = 0x00;
                } // auto zero off
            }
            else
            {
                //                qDebug() << "No Support AutoZero";
                ucLocalAutoZeroStatus = 0xFF;
            } // auto zero not supported
            break;
        }

            //----------- Optional : Used for Setting custom parameters ------------------------------------------------------------------------------
        case PBS_CID_186:  ///  0xBA (Request Custom Calibration Parameters)
        {
            qDebug() << "PBS_CID_186";
            break;
        }
        case PBS_CID_187:  ///  0xBB (Custom Calibration Parameter Response)
        {
            qDebug() << "PBS_CID_187";
            break;
        }
        case PBS_CID_188:  ///  0xBC (Set Custom Calibration Parameter)
        {
            qDebug() << "PBS_CID_188";
            break;
        }
        case PBS_CID_189:  ///  0xBD (Set Custom Parameters Successful)
        {
            qDebug() << "PBS_CID_189";
            break;
        }
            //------------------------------------------------------------------------------------------------------

        case PBS_CID_172:  // Calibration Successful 0xAC
        {
            qDebug() << "Calibration Sucessful 0xAC";

            pstPage1Data.stBPSCalibrationData.stBPSPage1_CID172_Data.ucCID172_AutoZeroStatus = stMessage.aucData[ucDataOffset+2];
            pstPage1Data.stBPSCalibrationData.stBPSPage1_CID172_Data.ucCID172_ReserveByte3 = BPS_PAGE_RESERVE_BYTE;
            pstPage1Data.stBPSCalibrationData.stBPSPage1_CID172_Data.ucCID172_ReserveByte4 = BPS_PAGE_RESERVE_BYTE;
            pstPage1Data.stBPSCalibrationData.stBPSPage1_CID172_Data.ucCID172_ReserveByte5 = BPS_PAGE_RESERVE_BYTE;
            pstPage1Data.stBPSCalibrationData.stBPSPage1_CID172_Data.usCID172_CalibrationData = (USHORT)stMessage.aucData[ucDataOffset+6];
            pstPage1Data.stBPSCalibrationData.stBPSPage1_CID172_Data.usCID172_CalibrationData |= (USHORT)(stMessage.aucData[ucDataOffset+7]<<8);
            ///TOFIX: http://www.thisisant.com/forum/viewthread/4590/

            ucLocalAutoZeroStatus = pstPage1Data.stBPSCalibrationData.stBPSPage1_CID172_Data.ucCID172_AutoZeroStatus;

            qDebug() << "AutOZeroStatus is" << ucLocalAutoZeroStatus;
            if (ucLocalAutoZeroStatus == 1)
                emit supportAutoZero();

            ///EMIT Calibration sucessfull
            if (isDoingAutoZero) {
                emit calibrationOverWithStatus(true, "AutoZero Success", pstPage1Data.stBPSCalibrationData.stBPSPage1_CID172_Data.usCID172_CalibrationData);
                isDoingAutoZero = false;
            }
            else {
                emit calibrationOverWithStatus(true, dataType2, pstPage1Data.stBPSCalibrationData.stBPSPage1_CID172_Data.usCID172_CalibrationData);

            }



            qDebug() << "DONE Calibration Sucessful 0xAC";
            break;
        }
        case PBS_CID_175:  // Failed response 0xAF
        {

            qDebug() << "failed response Calibration 0xAF";

            pstPage1Data.stBPSCalibrationData.stBPSPage1_CID175_Data.ucCID175_AutoZeroStatus = stMessage.aucData[ucDataOffset+2];
            pstPage1Data.stBPSCalibrationData.stBPSPage1_CID175_Data.ucCID175_ReserveByte3 = BPS_PAGE_RESERVE_BYTE;
            pstPage1Data.stBPSCalibrationData.stBPSPage1_CID175_Data.ucCID175_ReserveByte4 = BPS_PAGE_RESERVE_BYTE;
            pstPage1Data.stBPSCalibrationData.stBPSPage1_CID175_Data.ucCID175_ReserveByte5 = BPS_PAGE_RESERVE_BYTE;
            pstPage1Data.stBPSCalibrationData.stBPSPage1_CID175_Data.usCID175_CalibrationData = (USHORT)stMessage.aucData[ucDataOffset+6];
            pstPage1Data.stBPSCalibrationData.stBPSPage1_CID175_Data.usCID175_CalibrationData |= (USHORT)(stMessage.aucData[ucDataOffset+7]<<8);

            ucLocalAutoZeroStatus = pstPage1Data.stBPSCalibrationData.stBPSPage1_CID175_Data.ucCID175_AutoZeroStatus;

            qDebug() << "AutOZeroIS2" << ucLocalAutoZeroStatus;

            ///EMIT Calibration failed
            if (isDoingAutoZero) {
                emit calibrationOverWithStatus(false, "AutoZero Failed", pstPage1Data.stBPSCalibrationData.stBPSPage1_CID175_Data.usCID175_CalibrationData);
                isDoingAutoZero = false;
            }
            else {
                emit calibrationOverWithStatus(false, dataType2, pstPage1Data.stBPSCalibrationData.stBPSPage1_CID175_Data.usCID175_CalibrationData);
            }

            break;
        }

            ///--------------------------------- CTF defined msgs 0x10 -------------- (Page 56 of 80 Power profile)
        case PBS_CID_16:
        {
            qDebug() << "CTF : PBS_CID_16";
            ucLocalCTF_ID = stMessage.aucData[ucDataOffset+2];

            switch(ucLocalCTF_ID)
            {
            case PBS_CID_16_CTFID_1:  // Offset Msg  - 0x01
            {
                qDebug() <<  "PBS_CID_16_CTFID_1  - 0x01";

                //NB BIG ENDIAN
                //                pstPage1Data.stBPSCalibrationData.stBPSPage1_CID16_CTFID1_Data.usCID16_CTFID1_OffsetData |= (USHORT)(stMessage.aucData[ucDataOffset+6]<<8);
                //                pstPage1Data.stBPSCalibrationData.stBPSPage1_CID16_CTFID1_Data.usCID16_CTFID1_OffsetData = (USHORT)stMessage.aucData[ucDataOffset+7];
                //                usCTFOffset = pstPage1Data.stBPSCalibrationData.stBPSPage1_CID16_CTFID1_Data.usCID16_CTFID1_OffsetData;
                uint16_t zeroOffset = stMessage.aucData[8] + (stMessage.aucData[7]<<8);

                qDebug() << "FOUND SRM OFFSET!" << zeroOffset << "We are at " << lstOffsetValue.size()+1 << " number of reply";

                lstOffsetValue.append(zeroOffset);
                if (lstOffsetValue.size() == 6)
                    offsetFinishedHere();

                break;
            }
            case PBS_CID_16_CTFID_172:  // Slope/Serial acknowledgement - 0xAC
            {

                qDebug() <<  "PBS_CID_16_CTFID_172 - 0xAC";


                pstPage1Data.stBPSCalibrationData.stBPSPage1_CID16_CTFID172_Data.ucCID16_CTFID172_CTFAckMessage = stMessage.aucData[ucDataOffset+3];
                pstPage1Data.stBPSCalibrationData.stBPSPage1_CID16_CTFID172_Data.ucCID16_CTFID172_ReserveByte4 = stMessage.aucData[ucDataOffset+4];
                pstPage1Data.stBPSCalibrationData.stBPSPage1_CID16_CTFID172_Data.ucCID16_CTFID172_ReserveByte5 = stMessage.aucData[ucDataOffset+5];
                pstPage1Data.stBPSCalibrationData.stBPSPage1_CID16_CTFID172_Data.ucCID16_CTFID172_ReserveByte6 = stMessage.aucData[ucDataOffset+6];
                pstPage1Data.stBPSCalibrationData.stBPSPage1_CID16_CTFID172_Data.ucCID16_CTFID172_ReserveByte7 = stMessage.aucData[ucDataOffset+7];

                break;
            }
            default:
            {
                //
            }
            }
            break;
        }	// CTF defined pages break
        }
        break; // Calibration Page1 Break
    }

        // -------------- Optional : Bicycle Power Meter Get/Set Parameters (Page 0x02) -------------------------
    case BPS_PAGE_2:
    {
        qDebug() << "BPS_PAGE_2";
        break;
    }
        /// -------------- Optional : Measurement Output Data Page (0x03) : to show progress when Calibrating :   p.66doc -------------------------
    case BPS_PAGE_3:
    {
        qDebug() << "Measurement Output Data Page (0x03)";

        qDebug() << "BPS_PAGE_3: [" << hex << "[" << hex << stMessage.aucData[1] << "]" << "[" << hex << stMessage.aucData[2] << "]" << "[" << hex << stMessage.aucData[3] << "]" << "[" << hex << stMessage.aucData[4] << "]" << "[" << hex << stMessage.aucData[5] << "]" << "[" << hex << stMessage.aucData[6] << "]" << "[" << hex << stMessage.aucData[7]  << "]" << "[" << hex << stMessage.aucData[8] << "]";


        uint8_t numberOfDataType = stMessage.aucData[2];
        uint8_t dataType = stMessage.aucData[3];
        int8_t scaleFactor = stMessage.aucData[4];
        uint16_t timeStamp = stMessage.aucData[5] + (stMessage.aucData[6]<<8);
        int16_t measurementValue = stMessage.aucData[7] + (stMessage.aucData[8]<<8);

        //Check for rollOver on timeStamp
        if (lastTimeStamp0x03 > timeStamp) {
            numberOfRollOver0x03++;
        }
        int realTimeStamp = (65536*numberOfRollOver0x03) + timeStamp;
        double realValue = qPow(2, scaleFactor)* measurementValue;

        double countdownPerc = -1;
        double countDownTimeS = -1;
        double wholeTorque = -1;
        double leftTorque = -1;
        double rightTorque = -1;
        double wholeForce = -1;
        double leftForce = -1;
        double rightForce = -1;
        double zeroOffset = -1;
        double temperatureC = -1;
        double voltage = -1;


        // 0 - Countdown (progress bar) - % Percentage of process remaining until process is complete.
        if (dataType == 0) {
            qDebug() << "Percentage Remaining " << realValue;
            countdownPerc = realValue;
        }
        //1 Countdown (time) s Seconds remaining until process is complete.
        else if (dataType == 1) {
            qDebug() << "Second Remaining" << realValue;
            countDownTimeS = realValue;
        }
        //8 Torque (whole sensor) Nm Forward driving torque is represented as positive, back-pedalling torque is represented as negative
        else if (dataType == 8) {
            qDebug() << "Torque (whole sensor)" << realValue;
            wholeTorque = realValue;
        }
        //9 Torque (left) Nm Torque applied to the left pedal. Forward driving torque is represented as positive, back-pedalling torque is represented as negative
        else if (dataType == 9) {
            qDebug() << "Torque (left) Nm Torque" << realValue;
            leftTorque = realValue;
        }
        //10 Torque (right) Nm Torque applied to the right pedal. Forward driving torque is represented as positive, back-pedalling torque is represented as negative
        else if (dataType == 10) {
            qDebug() << "Torque (right) Nm Torque" << realValue;
            rightTorque = realValue;
        }
        //16 Force (whole sensor) N Forward driving force is represented as positive, back-pedalling force is represented as negative
        else if (dataType == 16) {
            qDebug() << "Force (whole sensor)" << realValue;
            wholeForce = realValue;
        }
        //17 Force (left) N Force applied to the left pedal. Forward driving force is represented as positive, back-pedalling force is represented as negative
        else if (dataType == 17) {
            qDebug() << "Force (left) N Force" << realValue;
            leftForce = realValue;
        }
        //18 Force (right) N Force applied to the right pedal. Forward driving force is represented as positive, back-pedalling force is represented as negative
        else if (dataType == 18) {
            qDebug() << "Force (right) N Force" << realValue;
            rightForce = realValue;
        }
        //24 Zero-offset - Scalar value representing the zero offset
        else if (dataType == 24) {
            qDebug() << "Scalar value representing the zero offset" << realValue;
            zeroOffset = realValue;
        }
        //25 Temperature degC Sensor temperature
        else if (dataType == 25) {
            qDebug() << "Temperature degC Sensor temperature" << realValue;
            temperatureC = realValue;
        }
        //26 Voltage V Voltage measured within the sensor
        else if (dataType == 26) {
            qDebug() << "Voltage V Voltage measured within the sensor" << realValue;
            voltage = realValue;
        }

        qDebug() << "numberOfDataType" << numberOfDataType << "dataType" << dataType << "scaleFactor" << scaleFactor <<
                    "timeStamp" << timeStamp << "realTimeStamp" << realTimeStamp << "measurementValue" << measurementValue << "realValue" << realValue;


        emit calibrationProgress(realTimeStamp, countdownPerc,  countDownTimeS,
                                 wholeTorque,  leftTorque,  rightTorque,
                                 wholeForce,  leftForce,  rightForce,
                                 zeroOffset,  temperatureC,  voltage);


        lastTimeStamp0x03 = timeStamp;
        break;
    }



        ///---------------------------------------- Standard Power Only - (0x10) -------------------------------------------------
    case BPS_PAGE_16:
    {
        pstPage16Data.ucPOEventCount       = stMessage.aucData[ucDataOffset+1];
        pstPage16Data.ucReserveByte2       = stMessage.aucData[ucDataOffset+2];  /// Pedal percentage
        pstPage16Data.ucPOInstantCadence   = stMessage.aucData[ucDataOffset+3];
        pstPage16Data.usPOAccumulatedPower = (USHORT)stMessage.aucData[ucDataOffset+4];
        pstPage16Data.usPOAccumulatedPower |= (USHORT)(stMessage.aucData[ucDataOffset+5]<<8);
        pstPage16Data.usPOInstantPower     = (USHORT)stMessage.aucData[ucDataOffset+6];
        pstPage16Data.usPOInstantPower     |= (USHORT)(stMessage.aucData[ucDataOffset+7]<<8);

        if (bPOFirstPageCheck==1)
        {
            bPOFirstPageCheck=0;
            ucPOEventCountPrev = pstPage16Data.ucPOEventCount;
            usPOAccumulatedPowerPrev = pstPage16Data.usPOAccumulatedPower;
        }
        else
        {
            //Calculate Average Power
            ulPOEventCountDiff = (ULONG)((pstPage16Data.ucPOEventCount - ucPOEventCountPrev) & MAX_UCHAR);
            ulPOAccumulatedPowerDiff = (ULONG)((pstPage16Data.usPOAccumulatedPower - usPOAccumulatedPowerPrev) & MAX_USHORT);
            if (ulPOEventCountDiff==0)
            {
                //                 pstEventStruct_.usParam2 = BPSRX_SUBEVENT_NOUPDATE;
                ulPOEventSpeedCheck++;
                if(ulPOEventSpeedCheck >= BPSRX_MAX_REPEATS)  {

                    emit powerChanged(userNb, 0);
                    if (usingForCadence) {
                        emit cadenceChanged(userNb, 0);
                    }
                    if (usingForSpeed) {
                        emit speedChanged(userNb, 0);
                    }

                }
            }
            else
            {
                ulPOEventSpeedCheck = 0;
                //                 pstEventStruct_.usParam2 = BPSRX_SUBEVENT_UPDATE;
                stCalculatedStdPowerData.ulAvgStandardPower = ulPOAccumulatedPowerDiff/ulPOEventCountDiff;
                ucPOEventCountPrev = pstPage16Data.ucPOEventCount;
                usPOAccumulatedPowerPrev = pstPage16Data.usPOAccumulatedPower;


                /// PEDAL POWER ------------------------------------------------------------
                if (pstPage16Data.ucReserveByte2 != (UCHAR)0xFF) {

                    /// Check if bit7 is 1 (RIGHT PEDAL)
                    bool usingRightPedal = (pstPage16Data.ucReserveByte2 >> 7)  & 1;

                    if (usingRightPedal) {
                        /// RIGHT PEDAL PERCENTAGE
                        ULONG rightPedalPerc;
                        rightPedalPerc  = pstPage16Data.ucReserveByte2;   /// bits 0-6 is the percentage value
                        /// Toggle bit 7 to 0
                        rightPedalPerc = rightPedalPerc ^= 1 << 7;
                        emit rightPedalBalanceChanged(userNb, rightPedalPerc);
                    }
                }
                /// --------------------------------------------------------------------------

                if (stCalculatedStdPowerData.ulAvgStandardPower < MAX_POWER) {
                   emit powerChanged(userNb, stCalculatedStdPowerData.ulAvgStandardPower);
                }


                if (usingForCadence) {
                    if (pstPage16Data.ucPOInstantCadence != (UCHAR)0xFF) {
                        if (pstPage16Data.ucPOInstantCadence < MAX_CAD) {
                          emit cadenceChanged(userNb, pstPage16Data.ucPOInstantCadence);
                        }
                    }
                    else {
                        emit cadenceChanged(userNb, -1); //invalid values
                    }
                }



            }

        }
        break;
    }

        //---------------------------------------- Wheel Torque (WT) Main Data Page  - 0x11 -------------------------------------------------
    case BPS_PAGE_17:
    {
        pstPage17Data.ucWTEventCount = stMessage.aucData[ucDataOffset+1];
        pstPage17Data.ucWTWheelTicks = stMessage.aucData[ucDataOffset+2];
        pstPage17Data.ucWTInstantCadence = stMessage.aucData[ucDataOffset+3];
        pstPage17Data.usWTAccumulatedWheelPeriod = (USHORT)stMessage.aucData[ucDataOffset+4];
        pstPage17Data.usWTAccumulatedWheelPeriod |= (USHORT)(stMessage.aucData[ucDataOffset+5]<<8);
        pstPage17Data.usWTAccumulatedTorque = (USHORT)stMessage.aucData[ucDataOffset+6];
        pstPage17Data.usWTAccumulatedTorque |= (USHORT)(stMessage.aucData[ucDataOffset+7]<<8);



        if (bWTFirstPageCheck==1)
        {
            bWTFirstPageCheck=0;
            //              pstEventStruct_.usParam2 = BPSRX_SUBEVENT_FIRSTPAGE;

            ucWTEventCountPrev = pstPage17Data.ucWTEventCount;
            usWTAccumulatedWheelPeriodPrev =  (pstPage17Data.usWTAccumulatedWheelPeriod*ucLocalSpeedFactor);
            ucWTWheelTicksPrev = (pstPage17Data.ucWTWheelTicks*ucLocalSpeedFactor);
            usWTAccumulatedWheelTorquePrev = (pstPage17Data.usWTAccumulatedTorque*ucLocalSpeedFactor);
        }
        else
        {


            ulWTEventCountDiff = (ULONG)((pstPage17Data.ucWTEventCount - ucWTEventCountPrev) & MAX_UCHAR);
            ulWTAccumulatedWheelPeriodDiff = (ULONG)((pstPage17Data.usWTAccumulatedWheelPeriod - usWTAccumulatedWheelPeriodPrev) & MAX_USHORT);
            ulWTWheelTicksDiff = (ULONG)((pstPage17Data.ucWTWheelTicks - ucWTWheelTicksPrev) & MAX_UCHAR);
            ulWTAccumulatedWheelTorqueDiff = (ULONG)((pstPage17Data.usWTAccumulatedTorque - usWTAccumulatedWheelTorquePrev) & MAX_USHORT);

            if (ulWTEventCountDiff==0)
            {
                //                 pstEventStruct_.usParam2 = BPSRX_SUBEVENT_NOUPDATE;
                ucWTZeroSpeedCheck++;

                if(ucWTZeroSpeedCheck >= BPSRX_MAX_REPEATS)  // Event sync zero speed condition
                {
                    //                    pstEventStruct_.usParam2 = BPSRX_SUBEVENT_ZERO_SPEED;
                    //ucWTZeroSpeedCheck = 0;
                    stCalculatedStdWheelTorqueData.ulWTAvgSpeed = ((ULONG)0);
                    stCalculatedStdWheelTorqueData.ulWTDistance = ((ULONG)0);
                    stCalculatedStdWheelTorqueData.ulWTAngularVelocity = ((ULONG)0);
                    stCalculatedStdWheelTorqueData.ulWTAverageTorque = ((ULONG)0);
                    stCalculatedStdWheelTorqueData.ulWTAveragePower = ((ULONG)0);


                    emit powerChanged(userNb, 0);
                    if (usingForCadence) {
                        emit cadenceChanged(userNb, 0);
                    }
                    if (usingForSpeed) {
                        emit speedChanged(userNb, 0);
                    }


                }
            }
            else
            {
                ucWTZeroSpeedCheck = 0;
                ucLocalSpeedFactor = 1; // assume moving on update.

                if(ulWTAccumulatedWheelPeriodDiff == 0)  // Time sync zero speed condition
                {
                    stCalculatedStdWheelTorqueData.ulWTAvgSpeed = ((ULONG)0);
                    stCalculatedStdWheelTorqueData.ulWTDistance = ((ULONG)0);
                    stCalculatedStdWheelTorqueData.ulWTAngularVelocity = ((ULONG)0);
                    stCalculatedStdWheelTorqueData.ulWTAverageTorque = ((ULONG)0);
                    stCalculatedStdWheelTorqueData.ulWTAveragePower = ((ULONG)0);

                    //                    pstEventStruct_.usParam2 = BPSRX_SUBEVENT_ZERO_SPEED;
                    ucLocalSpeedFactor = 0;
                    emit powerChanged(userNb, 0);
                    if (usingForCadence) {
                        emit cadenceChanged(userNb, 0);
                    }
                    if (usingForSpeed) {
                        emit speedChanged(userNb, 0);
                    }
                }
                else
                {
                    //                    qDebug() << "Wheel Size here is:" << wheelSize;
                    stCalculatedStdWheelTorqueData.ulWTAvgSpeed = (ULONG)(((ulWTEventCountDiff<<11)*wheelSize/1000*36)/((ulWTAccumulatedWheelPeriodDiff)*10)*ucLocalSpeedFactor);
                    stCalculatedStdWheelTorqueData.ulWTDistance = (ULONG)(((wheelSize/1000)*ulWTWheelTicksDiff)*ucLocalSpeedFactor);
                    stCalculatedStdWheelTorqueData.ulWTAngularVelocity = (ULONG)((BPS_PI_CONSTANT*(ulWTEventCountDiff<<4)/ulWTAccumulatedWheelPeriodDiff)*ucLocalSpeedFactor);
                    stCalculatedStdWheelTorqueData.ulWTAverageTorque = (ULONG)((ulWTAccumulatedWheelTorqueDiff/(32*ulWTEventCountDiff))*ucLocalSpeedFactor);
                    stCalculatedStdWheelTorqueData.ulWTAveragePower = (ULONG)((BPS_PI_CONSTANT*(ulWTAccumulatedWheelTorqueDiff>>1)/ulWTAccumulatedWheelPeriodDiff)*ucLocalSpeedFactor);


                    if (stCalculatedStdWheelTorqueData.ulWTAveragePower < MAX_POWER) {
                        emit powerChanged(userNb, stCalculatedStdWheelTorqueData.ulWTAveragePower);
                    }

                    if (usingForCadence) {
                        if (pstPage17Data.ucWTInstantCadence != (UCHAR)0xFF) {
                            if (pstPage17Data.ucWTInstantCadence < MAX_CAD) {
                              emit cadenceChanged(userNb, pstPage17Data.ucWTInstantCadence);
                            }
                        }
                        else {
                            emit cadenceChanged(userNb, -1); //invalid values
                        }
                    }
                    if (usingForSpeed && stCalculatedStdWheelTorqueData.ulWTAvgSpeed < MAX_SPEED) {
                        emit speedChanged(userNb, stCalculatedStdWheelTorqueData.ulWTAvgSpeed);
                    }
                }


                if (stCalculatedStdWheelTorqueData.ulWTAverageTorque==0 && ucLocalSpeedFactor==1)
                {
                    //                    qDebug() << "COASTING.. send calibration request?";
                    /// TODO: Emit even coasting for calibration?
                    //                    pstEventStruct_.usParam2 = BPSRX_SUBEVENT_COASTING;
                }

                ucWTEventCountPrev = pstPage17Data.ucWTEventCount;
                usWTAccumulatedWheelPeriodPrev =  (pstPage17Data.usWTAccumulatedWheelPeriod);
                ucWTWheelTicksPrev = (pstPage17Data.ucWTWheelTicks);
                usWTAccumulatedWheelTorquePrev = (pstPage17Data.usWTAccumulatedTorque);
            }
        }
        break;
    }

        //---------------------------------------- Standard Crank Torque (CT) Main Data Page - 0x12 -------------------------------------------------
    case BPS_PAGE_18:
    {

        pstPage18Data.ucCTEventCount = stMessage.aucData[ucDataOffset+1];
        pstPage18Data.ucCTCrankTicks = stMessage.aucData[ucDataOffset+2];
        pstPage18Data.ucCTInstantCadence = stMessage.aucData[ucDataOffset+3];
        pstPage18Data.usCTAccumulatedCrankPeriod = (USHORT)stMessage.aucData[ucDataOffset+4];
        pstPage18Data.usCTAccumulatedCrankPeriod |= (USHORT)(stMessage.aucData[ucDataOffset+5]<<8);
        pstPage18Data.usCTAccumulatedTorque = (USHORT)stMessage.aucData[ucDataOffset+6];
        pstPage18Data.usCTAccumulatedTorque |= (USHORT)(stMessage.aucData[ucDataOffset+7]<<8);


        if (bCTFirstPageCheck==1)
        {
            bCTFirstPageCheck=0;
            ucCTEventCountPrev = pstPage18Data.ucCTEventCount;
            usCTAccumulatedCrankPeriodPrev = (pstPage18Data.usCTAccumulatedCrankPeriod*ucLocalSpeedFactor);
            usCTAccumulatedCrankTorquePrev = (pstPage18Data.usCTAccumulatedTorque*ucLocalSpeedFactor);
        }
        else
        {
            ulCTEventCountDiff = (ULONG)((pstPage18Data.ucCTEventCount - ucCTEventCountPrev) & MAX_UCHAR);
            ulCTAccumulatedCrankPeriodDiff= (ULONG)((pstPage18Data.usCTAccumulatedCrankPeriod - usCTAccumulatedCrankPeriodPrev) & MAX_USHORT);
            ulCTAccumulatedCrankTorqueDiff= (ULONG)((pstPage18Data.usCTAccumulatedTorque - usCTAccumulatedCrankTorquePrev) & MAX_USHORT);

            if (ulCTEventCountDiff==0)
            {
                //                 pstEventStruct_.usParam2 = BPSRX_SUBEVENT_NOUPDATE;
                ucCTZeroSpeedCheck++;

                if(ucCTZeroSpeedCheck>=BPSRX_MAX_REPEATS) // Event sync zero speed condition
                {
                    stCalculatedStdCrankTorqueData.ulCTAverageCadence = 0;
                    stCalculatedStdCrankTorqueData.ulCTAngularVelocity = 0;
                    stCalculatedStdCrankTorqueData.ulCTAverageTorque = 0;
                    stCalculatedStdCrankTorqueData.ulCTAveragePower = 0;

                    /// EMIT
                    emit powerChanged(userNb, 0);

                    if (usingForCadence) {
                        emit cadenceChanged(userNb, 0);
                    }
                    if (usingForSpeed) {
                        emit speedChanged(userNb, 0);
                    }

                    //                    pstEventStruct_.usParam2 = BPSRX_SUBEVENT_ZERO_SPEED;
                }
            }
            else
            {
                //                 pstEventStruct_.usParam2 = BPSRX_SUBEVENT_UPDATE;
                ucCTZeroSpeedCheck=0;         // reset
                ucLocalSpeedFactor=1;         // assume moving

                if(ulCTAccumulatedCrankPeriodDiff==0) // Time sync zero speed condition
                {
                    //                    pstEventStruct_.usParam2 = BPSRX_SUBEVENT_ZERO_SPEED;
                    ucLocalSpeedFactor=0;
                    stCalculatedStdCrankTorqueData.ulCTAverageCadence = 0;
                    stCalculatedStdCrankTorqueData.ulCTAngularVelocity = 0;
                    stCalculatedStdCrankTorqueData.ulCTAverageTorque = 0;
                    stCalculatedStdCrankTorqueData.ulCTAveragePower = 0;


                    emit powerChanged(userNb, 0);
                    if (usingForCadence) {
                        emit cadenceChanged(userNb, 0);
                    }
                    if (usingForSpeed) {
                        emit speedChanged(userNb, 0);
                    }
                }
                else
                {
                    stCalculatedStdCrankTorqueData.ulCTAverageCadence = (ULONG)((((ulCTEventCountDiff*60)<<11)/(ulCTAccumulatedCrankPeriodDiff))*ucLocalSpeedFactor);
                    stCalculatedStdCrankTorqueData.ulCTAngularVelocity = (ULONG)((BPS_PI_CONSTANT*(ulCTEventCountDiff<<4)/ulCTAccumulatedCrankPeriodDiff)*ucLocalSpeedFactor);
                    stCalculatedStdCrankTorqueData.ulCTAverageTorque = (ULONG)((ulCTAccumulatedCrankTorqueDiff/(32*ulCTEventCountDiff))*ucLocalSpeedFactor);
                    stCalculatedStdCrankTorqueData.ulCTAveragePower = (ULONG)((BPS_PI_CONSTANT*(ulCTAccumulatedCrankTorqueDiff>>1)/ulCTAccumulatedCrankPeriodDiff)*ucLocalSpeedFactor);

                    /// EMIT
                    if (stCalculatedStdCrankTorqueData.ulCTAveragePower < MAX_POWER) {
                       emit powerChanged(userNb, stCalculatedStdCrankTorqueData.ulCTAveragePower);
                    }

                    if (usingForCadence && stCalculatedStdCrankTorqueData.ulCTAverageCadence != (UCHAR)0xFF && stCalculatedStdCrankTorqueData.ulCTAverageCadence < MAX_CAD) {
                        emit cadenceChanged(userNb, stCalculatedStdCrankTorqueData.ulCTAverageCadence);
                    }


                }
            }

            ucCTEventCountPrev = pstPage18Data.ucCTEventCount;
            usCTAccumulatedCrankPeriodPrev = pstPage18Data.usCTAccumulatedCrankPeriod;
            usCTAccumulatedCrankTorquePrev = pstPage18Data.usCTAccumulatedTorque;
        }

        break;
    }

        //----------------------------------------  Crank Torque Frequency - 0x20 -------------------------------------------------
    case BPS_PAGE_32:
    {


        // Standard Crank Torque (CT) Main Data Page
        // NOTE: Big Endian Data
        pstPage32Data.ucCTFEventCount = stMessage.aucData[ucDataOffset+1];
        pstPage32Data.usCTFSlope = (USHORT)(stMessage.aucData[ucDataOffset+2]<<8);
        pstPage32Data.usCTFSlope |= (USHORT)stMessage.aucData[ucDataOffset+3];
        pstPage32Data.usCTFTimeStamp = (USHORT)(stMessage.aucData[ucDataOffset+4]<<8);
        pstPage32Data.usCTFTimeStamp |= (USHORT)stMessage.aucData[ucDataOffset+5];
        pstPage32Data.usCTFTorqueTickStamp = (USHORT)(stMessage.aucData[ucDataOffset+6]<<8);
        pstPage32Data.usCTFTorqueTickStamp |= (USHORT)stMessage.aucData[ucDataOffset+7];


        ulCTFEventCountDiff = (ULONG)((pstPage32Data.ucCTFEventCount - ucCTFEventCountPrev) & MAX_UCHAR);
        ulCTFTimeStampDiff = (ULONG)((pstPage32Data.usCTFTimeStamp - usCTFTimeStampPrev) & MAX_USHORT);
        ulCTFTorqueTickStampDiff = (ULONG)((pstPage32Data.usCTFTorqueTickStamp - usCTFTorqueTickStampPrev) & MAX_USHORT);



        if (bCTFFirstPageCheck==1)
        {
            qDebug() << "bCTFFirstPageCheck==1";

            bCTFFirstPageCheck=0;

            ucCTFEventCountPrev = pstPage32Data.ucCTFEventCount;
            usCTFTimeStampPrev = pstPage32Data.usCTFTimeStamp;
            usCTFTorqueTickStampPrev = pstPage32Data.usCTFTorqueTickStamp;
        }
        else
        {
            if (ulCTFEventCountDiff==0)
            {
                ucCTFZeroSpeedCheck++;

                if(ucCTFZeroSpeedCheck>=BPSRX_MAX_REPEATS)  // Event sync zero speed condition
                {
                    //                    qDebug() << "got here >= BPSRX_MAX_REPEATS";

                    stCalculatedStdCTFData.ulCTFAverageCadence = (ULONG)0;
                    stCalculatedStdCTFData.ulCTFTorqueFrequency = (ULONG)0;
                    stCalculatedStdCTFData.ulCTFAverageTorque = (ULONG)0;
                    stCalculatedStdCTFData.ulCTFAveragePower = (ULONG)0;


                    emit powerChanged(userNb, 0);
                    if (usingForCadence) {
                        emit cadenceChanged(userNb, 0);
                    }
                    if (usingForSpeed) {
                        emit speedChanged(userNb, 0);
                    }
                }
            }
            else
            {


                //                 pstEventStruct_.usParam2 = BPSRX_SUBEVENT_UPDATE;
                ucCTFZeroSpeedCheck = 0;

                stCalculatedStdCTFData.ulCTFAverageCadence = (ULONG)(((ulCTFEventCountDiff<<6)*1875)/ulCTFTimeStampDiff);
                stCalculatedStdCTFData.ulCTFTorqueFrequency = (ULONG)((((ulCTFTorqueTickStampDiff<<4)*125)/ulCTFTimeStampDiff) - zeroOffsetMean);
                stCalculatedStdCTFData.ulCTFAverageTorque = (ULONG)((10*stCalculatedStdCTFData.ulCTFTorqueFrequency)/pstPage32Data.usCTFSlope);
                stCalculatedStdCTFData.ulCTFAveragePower = (ULONG)(((stCalculatedStdCTFData.ulCTFAverageTorque*stCalculatedStdCTFData.ulCTFAverageCadence*BPS_PI_CONSTANT)/15)>>9);
                ucCTFEventCountPrev = pstPage32Data.ucCTFEventCount;
                usCTFTimeStampPrev = pstPage32Data.usCTFTimeStamp;
                usCTFTorqueTickStampPrev = pstPage32Data.usCTFTorqueTickStamp;


                if (stCalculatedStdCTFData.ulCTFAveragePower < MAX_POWER) {
                    emit powerChanged(userNb, stCalculatedStdCTFData.ulCTFAveragePower);
                }

                if (usingForCadence && stCalculatedStdCTFData.ulCTFAverageCadence < MAX_CAD) {
                    emit cadenceChanged(userNb, stCalculatedStdCTFData.ulCTFAverageCadence);
                }


            }
        }
        break;
    }
    case GLOBAL_PAGE_80:
    {
        stPage80Data.ucHwVersion    = stMessage.aucData[ucDataOffset+3];
        stPage80Data.usManId        = (USHORT)stMessage.aucData[ucDataOffset+4];
        stPage80Data.usManId       |= (USHORT)(stMessage.aucData[ucDataOffset+5] << 8);
        stPage80Data.usModelNumber  = (USHORT)stMessage.aucData[ucDataOffset+6];
        stPage80Data.usModelNumber |= (USHORT)(stMessage.aucData[ucDataOffset+7] << 8);
        break;
    }
    case GLOBAL_PAGE_81:
    {
        stPage81Data.ucSwVersion     = stMessage.aucData[ucDataOffset+3];
        stPage81Data.ulSerialNumber  = (ULONG)stMessage.aucData[ucDataOffset+4];
        stPage81Data.ulSerialNumber |= (ULONG)stMessage.aucData[ucDataOffset+5] << 8;
        stPage81Data.ulSerialNumber |= (ULONG)stMessage.aucData[ucDataOffset+6] << 16;
        stPage81Data.ulSerialNumber |= (ULONG)stMessage.aucData[ucDataOffset+7] << 24;
        break;
    }
    case GLOBAL_PAGE_82:
    {
        stPage82Data.ulCumOperatingTime    = (ULONG)stMessage.aucData[ucDataOffset+3];
        stPage82Data.ulCumOperatingTime   |= (ULONG)stMessage.aucData[ucDataOffset+4] << 8;
        stPage82Data.ulCumOperatingTime   |= (ULONG)stMessage.aucData[ucDataOffset+5] << 16;
        stPage82Data.ucBattVoltage256      = stMessage.aucData[ucDataOffset+6];
        stPage82Data.ucBattVoltage         = COMMON82_COARSE_BATT_VOLTAGE(stMessage.aucData[ucDataOffset+7]);
        stPage82Data.ucBattStatus          = COMMON82_BATT_STATUS(stMessage.aucData[ucDataOffset+7]);
        stPage82Data.ucCumOperatingTimeRes = COMMON82_CUM_TIME_RESOLUTION(stMessage.aucData[ucDataOffset+7]);


        USHORT usDeviceNumber = stMessage.aucData[10] | (stMessage.aucData[11] << 8);
        //---------------------------------
        switch (stPage82Data.ucBattStatus)
        {
        case GBL82_BATT_STATUS_NEW:
        {
            break;
        }
        case GBL82_BATT_STATUS_GOOD:
        {
            break;
        }
        case GBL82_BATT_STATUS_OK:
        {
            break;
        }
        case GBL82_BATT_STATUS_LOW:
        {
            if (!alreadyShownBatteryWarning) {
                emit batteryLow(tr("Power"), 1, usDeviceNumber);
                alreadyShownBatteryWarning = true;
            }
            break;
        }
        case GBL82_BATT_STATUS_CRITICAL:
        {
            if (!alreadyShownBatteryWarning) {
                emit batteryLow(tr("Power"), 0, usDeviceNumber);
                alreadyShownBatteryWarning = true;
            }
            break;
        }
        default:
        {
            qDebug() << "battery invalid status";
            break;
        }
        }
        //--------------------


        break;
    }
    default:
    {
        /// Unknown page
        break;
    }
    }
}







/////////////////////////////////////////////////////////////////////////////////
/// Constructor
/////////////////////////////////////////////////////////////////////////////////
Power_Controller::Power_Controller(int userNb, QObject *parent) : QObject(parent)  {


    this->userNb = userNb;

    /// Intialize  variables
    pstPage1Data.ucCalibrationID = 0;
    pstPage1Data.stBPSCalibrationData.stBPSPage1_CID18_Data.ucCID18_SensorConfiguration_EN = 0;     //not supported
    pstPage1Data.stBPSCalibrationData.stBPSPage1_CID18_Data.ucCID18_SensorConfiguration_STATUS = 0; //off
    pstPage1Data.stBPSCalibrationData.stBPSPage1_CID18_Data.ssCID18_RawTorqueCount = 0;
    pstPage1Data.stBPSCalibrationData.stBPSPage1_CID18_Data.ssCID18_OffsetTorque = 0;

    pstPage1Data.stBPSCalibrationData.stBPSPage1_CID172_Data.ucCID172_AutoZeroStatus = 0xFF;
    pstPage1Data.stBPSCalibrationData.stBPSPage1_CID172_Data.ucCID172_ReserveByte3 = BPS_PAGE_RESERVE_BYTE;
    pstPage1Data.stBPSCalibrationData.stBPSPage1_CID172_Data.ucCID172_ReserveByte4 = BPS_PAGE_RESERVE_BYTE;
    pstPage1Data.stBPSCalibrationData.stBPSPage1_CID172_Data.ucCID172_ReserveByte5 = BPS_PAGE_RESERVE_BYTE;
    pstPage1Data.stBPSCalibrationData.stBPSPage1_CID172_Data.usCID172_CalibrationData = 0;

    pstPage1Data.stBPSCalibrationData.stBPSPage1_CID175_Data.ucCID175_AutoZeroStatus = 0xFF;
    pstPage1Data.stBPSCalibrationData.stBPSPage1_CID175_Data.ucCID175_ReserveByte3 = BPS_PAGE_RESERVE_BYTE;
    pstPage1Data.stBPSCalibrationData.stBPSPage1_CID175_Data.ucCID175_ReserveByte4 = BPS_PAGE_RESERVE_BYTE;
    pstPage1Data.stBPSCalibrationData.stBPSPage1_CID175_Data.ucCID175_ReserveByte5 = BPS_PAGE_RESERVE_BYTE;
    pstPage1Data.stBPSCalibrationData.stBPSPage1_CID175_Data.usCID175_CalibrationData = 0;

    pstPage1Data.stBPSCalibrationData.stBPSPage1_CID16_CTFID1_Data.ucCID16_CTFID1_ReserveByte3 = BPS_PAGE_RESERVE_BYTE;
    pstPage1Data.stBPSCalibrationData.stBPSPage1_CID16_CTFID1_Data.ucCID16_CTFID1_ReserveByte4 = BPS_PAGE_RESERVE_BYTE;
    pstPage1Data.stBPSCalibrationData.stBPSPage1_CID16_CTFID1_Data.ucCID16_CTFID1_ReserveByte5 = BPS_PAGE_RESERVE_BYTE;
    pstPage1Data.stBPSCalibrationData.stBPSPage1_CID16_CTFID1_Data.usCID16_CTFID1_OffsetData = 0;

    pstPage1Data.stBPSCalibrationData.stBPSPage1_CID16_CTFID172_Data.ucCID16_CTFID172_CTFAckMessage = 0;
    pstPage1Data.stBPSCalibrationData.stBPSPage1_CID16_CTFID172_Data.ucCID16_CTFID172_ReserveByte4 = BPS_PAGE_RESERVE_BYTE;
    pstPage1Data.stBPSCalibrationData.stBPSPage1_CID16_CTFID172_Data.ucCID16_CTFID172_ReserveByte5 = BPS_PAGE_RESERVE_BYTE;
    pstPage1Data.stBPSCalibrationData.stBPSPage1_CID16_CTFID172_Data.ucCID16_CTFID172_ReserveByte6 = BPS_PAGE_RESERVE_BYTE;
    pstPage1Data.stBPSCalibrationData.stBPSPage1_CID16_CTFID172_Data.ucCID16_CTFID172_ReserveByte7 = BPS_PAGE_RESERVE_BYTE;

    // pstPage1Data.stBPSCalibrationData.stBPSPage1_CID18_Data.ucCID18_ReserveByte7 = stMessage.aucData[ucDataOffset+7]

    // Power Only (PO) Main Data Page
    pstPage16Data.ucPOEventCount = 0;
    pstPage16Data.ucPOInstantCadence = 0;
    pstPage16Data.ucReserveByte2 = BPS_PAGE_RESERVE_BYTE;
    pstPage16Data.usPOAccumulatedPower = 0;
    pstPage16Data.usPOInstantPower = 0;

    // Wheel Torque (WT) Main Data Page
    pstPage17Data.ucWTEventCount = 0;
    pstPage17Data.ucWTWheelTicks = 0;
    pstPage17Data.ucWTInstantCadence = 0;
    pstPage17Data.usWTAccumulatedWheelPeriod = 0;
    pstPage17Data.usWTAccumulatedTorque = 0;

    // Standard Crank Torque (CT) Main Data Page
    pstPage18Data.ucCTEventCount = 0;
    pstPage18Data.ucCTCrankTicks = 0;
    pstPage18Data.ucCTInstantCadence = 0;
    pstPage18Data.usCTAccumulatedCrankPeriod = 0;
    pstPage18Data.usCTAccumulatedTorque = 0;

    // Crank Torque Frequency (CTF) Main Data Page
    pstPage32Data.ucCTFEventCount = 0;
    pstPage32Data.usCTFSlope = 0;
    pstPage32Data.usCTFTimeStamp = 0;
    pstPage32Data.usCTFTorqueTickStamp = 0;

    // Global Common Pages
    stPage80Data.ucPg80ReserveByte1 = COMMON_PAGE_RESERVE_BYTE;
    stPage80Data.ucPg80ReserveByte1 = COMMON_PAGE_RESERVE_BYTE;
    stPage80Data.ucHwVersion = 0;
    stPage80Data.usManId = 0;
    stPage80Data.usModelNumber = 0;

    stPage81Data.ucPg81ReserveByte1 = COMMON_PAGE_RESERVE_BYTE;
    stPage81Data.ucPg81ReserveByte1 = COMMON_PAGE_RESERVE_BYTE;
    stPage81Data.ucSwVersion = 0;
    stPage81Data.ulSerialNumber = 0;

    stPage82Data.ucPg82ReserveByte1 = COMMON_PAGE_RESERVE_BYTE;
    stPage82Data.ucPg82ReserveByte1 = COMMON_PAGE_RESERVE_BYTE;
    stPage82Data.ulCumOperatingTime = 0;
    stPage82Data.ucBattVoltage256 = 0;
    stPage82Data.ucBattVoltage = 0;
    stPage82Data.ucBattStatus = 0;
    stPage82Data.ucCumOperatingTimeRes = 0;

    ucLocalAutoZeroStatus = 0xFF; //Not supported
    ucLocalCTF_ID = 0xFF;
    ucLocalSpeedFactor = 1;

    // Basic Power Variables
    stCalculatedStdPowerData.ulAvgStandardPower = 0;
    bPOFirstPageCheck = 1;
    ucPOEventCountPrev = 0;
    usPOAccumulatedPowerPrev = 0;
    ulPOEventCountDiff = 0;
    ulPOEventSpeedCheck = 0;
    ulPOAccumulatedPowerDiff=0;

    // Wheel Torque Variables
    stCalculatedStdWheelTorqueData.ulWTAvgSpeed = 0;
    stCalculatedStdWheelTorqueData.ulWTDistance = 0;
    stCalculatedStdWheelTorqueData.ulWTAngularVelocity = 0;
    stCalculatedStdWheelTorqueData.ulWTAveragePower = 0;
    stCalculatedStdWheelTorqueData.ulWTAverageTorque = 0;
    bWTFirstPageCheck = 1;
    ucWTEventCountPrev = 0;
    ulWTEventCountDiff = 0;
    usWTAccumulatedWheelPeriodPrev=0;
    ulWTAccumulatedWheelPeriodDiff=0;
    usWTAccumulatedWheelTorquePrev = 0;
    ulWTAccumulatedWheelTorqueDiff = 0;
    ucWTWheelTicksPrev=0;
    ulWTWheelTicksDiff=0;
    ucWTZeroSpeedCheck=0;

    // Crank Torque Variables
    stCalculatedStdCrankTorqueData.ulCTAverageCadence=0;
    stCalculatedStdCrankTorqueData.ulCTAngularVelocity = 0;
    stCalculatedStdCrankTorqueData.ulCTAveragePower = 0;
    stCalculatedStdCrankTorqueData.ulCTAverageTorque = 0;
    bCTFirstPageCheck = 1;
    ucCTEventCountPrev=0;
    ulCTEventCountDiff=0;
    usCTAccumulatedCrankPeriodPrev=0;
    ulCTAccumulatedCrankPeriodDiff=0;
    usCTAccumulatedCrankTorquePrev=0;
    ulCTAccumulatedCrankTorqueDiff=0;
    ucCTZeroSpeedCheck=0;

    // Crank Torque Frequency Variables
    stCalculatedStdCTFData.ulCTFAverageCadence = 0;
    stCalculatedStdCTFData.ulCTFAveragePower = 0;
    stCalculatedStdCTFData.ulCTFAverageTorque = 0;
    stCalculatedStdCTFData.ulCTFTorqueFrequency = 0;
    bCTFFirstPageCheck = 1;
    ucCTFEventCountPrev = 0;
    ulCTFEventCountDiff = 0;
    usCTFTimeStampPrev = 0;
    ulCTFTimeStampDiff = 0;
    usCTFTorqueTickStampPrev =0;
    ulCTFTorqueTickStampDiff =0;
    ucCTFZeroSpeedCheck=0;

    // Calibration data
    loadOffset();
    bCalibrationTimeout = 0;
    ucAckFailCount = 0;

    dataPower_BPS_PAGE_16_NotChanged = 0;
    dataCadence_BPS_PAGE_16_NotChanged = 0;
    dataCadence_BPS_PAGE_18_NotChanged = 0;

    alreadyShownBatteryWarning = false;
    isDoingAutoZero = false;

    dataTypeOffset = "Offset";
    dataType2 = "Data";


    /// New - flag
    nullCount_page0x10 = nullCount_page0x11 = nullCount_page0x12 = nullCount_page0x20 = 0;
    firstMessage0x10 = firstMessage0x11 = firstMessage0x12 = firstMessage0x20 = true;

    lastTimeStamp0x03 = 0;
    numberOfRollOver0x03 = 0;


    usingForSpeed = true;
    usingForCadence = false;


}

//----------------------------------------------
Power_Controller::~Power_Controller() {

}



//-----------------------------------------
void Power_Controller::saveOffset() {

    QSettings settings;

    settings.beginGroup("PowerMeter");
    settings.setValue("offset", zeroOffsetMean);
    settings.endGroup();
}

//-----------------------------------------
void Power_Controller::loadOffset() {

    QSettings settings;

    settings.beginGroup("PowerMeter");
    zeroOffsetMean = settings.value("offset", BPSRX_INITIAL_OFFSET ).toInt();
    settings.endGroup();
}

////////////////////////////////////////////////////////////////////////////////////////
void Power_Controller::newCalibrationStarted() {

//    qDebug() << "Inside POWER_CONTROLLER thread" << QThread::currentThreadId();

    lastTimeStamp0x03 = 0;
    numberOfRollOver0x03 = 0;
    lstOffsetValue.clear();

}


//-------------------------------------------------------
void Power_Controller::offsetFinishedHere() {

    qDebug() << "Enought offset value received... calculate offset mean now!, size list:" << lstOffsetValue.size();


    double meanOffset = 0;
    for (int i=0; i<lstOffsetValue.size(); i++) {
        meanOffset+= lstOffsetValue.at(i);
    }

    zeroOffsetMean = meanOffset/lstOffsetValue.size();
    saveOffset();
    emit calibrationOverWithStatus(true, dataTypeOffset, zeroOffsetMean);

    qDebug() << "TEST1";

    qDebug() << "CTF calibrate success:" << zeroOffsetMean;


}


