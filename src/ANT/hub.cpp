/*
This software is subject to the license described in the License.txt file
included with this software distribution. You may not use this file except in compliance
with this license.

Copyright (c) Dynastream Innovations Inc. 2013
All rights reserved.
*/

#include "hub.h"

#include <QDebug>
#include <stdio.h>
#include <assert.h>

#include "types.h"
#include "dsi_framer_ant.hpp"
#include "dsi_thread.h"
#include "dsi_serial_generic.hpp"
#include "dsi_debug.hpp"

#include "util.h"
#include "account.h"
#include "antmsg.h"
#include "myconstants.h"




#define ENABLE_EXTENDED_MESSAGES

// 0=RECEIVE
#define USER_ANTCHANNEL       (0)
// ANT+ Managed Network Key
#define USER_NETWORK_KEY      {0xB9, 0xA5, 0x21, 0xFB, 0xBD, 0x72, 0xC3, 0x45, }
// The network key is assigned to this network number
#define USER_NETWORK_NUM      (0)
#define MESSAGE_TIMEOUT      (1000)





//////////////////// USB ANT KEY CONFIG /////////////////////////////////////////////////////
const UCHAR ucChannelType = (UCHAR) 0;      //Receiver
//const int USER_BAUDRATE_USB1 = 500000;
//const int USER_BAUDRATE_USB2 = 576000;
//int USER_BAUDRATE = 576000;
///////////////////////////////////////////////////////////////////////////////////////////
// Indexes into message recieved from ANT
#define MESSAGE_BUFFER_DATA1_INDEX ((UCHAR) 0)
#define MESSAGE_BUFFER_DATA2_INDEX ((UCHAR) 1)
#define MESSAGE_BUFFER_DATA3_INDEX ((UCHAR) 2)
#define MESSAGE_BUFFER_DATA4_INDEX ((UCHAR) 3)
#define MESSAGE_BUFFER_DATA5_INDEX ((UCHAR) 4)
#define MESSAGE_BUFFER_DATA6_INDEX ((UCHAR) 5)
#define MESSAGE_BUFFER_DATA7_INDEX ((UCHAR) 6)
#define MESSAGE_BUFFER_DATA8_INDEX ((UCHAR) 7)
#define MESSAGE_BUFFER_DATA9_INDEX ((UCHAR) 8)
#define MESSAGE_BUFFER_DATA10_INDEX ((UCHAR) 9)
#define MESSAGE_BUFFER_DATA11_INDEX ((UCHAR) 10)
#define MESSAGE_BUFFER_DATA12_INDEX ((UCHAR) 11)
#define MESSAGE_BUFFER_DATA13_INDEX ((UCHAR) 12)
#define MESSAGE_BUFFER_DATA14_INDEX ((UCHAR) 13)






////////////////////////////////////////////////////////////////////////////////
/// Constructor
////////////////////////////////////////////////////////////////////////////////
Hub::Hub(int stickNumber, QObject *parent) :QObject(parent) {

    this->stickNumber = stickNumber;

    pclSerialObject = (DSISerialGeneric*)NULL;
    pclMessageObject = (DSIFramerANT*)NULL;
    uiDSIThread = (DSI_THREAD_ID)NULL;
    bDisplay = TRUE;
    bBroadcasting = FALSE;

    memset(aucTransmitBuffer,0,ANT_STANDARD_DATA_PAYLOAD_SIZE);

    calibrationReponseReceived = false;
    numberOfFailCalibration = 0;



}



////////////////////////////////////////////////////////////////////////////////
// ucDeviceNumber_: USB Device Number (0 for first USB stick plugged and so on)
//                  If not specified on command line
// ucChannelType_:  ANT Channel Type. 0 = Master, 1 = Slave
////////////////////////////////////////////////////////////////////////////////
bool Hub::initUSBStick(int stickNumber) {

    qDebug() << "initUSbStick start" << stickNumber;

    if (this->stickNumber !=  stickNumber) {
        qDebug() << "This should never happen! Hub bug";
        return false;
    }

    //    if (stickNumber != this->stickNumber) {
    //        qDebug() << "okignore this init, not my command";
    //        return false;
    //    }

    bool bStatus = false;

    // Initialize condition var and mutex
    UCHAR ucCondInit = DSIThread_CondInit(&condTestDone);
    assert(ucCondInit == DSI_THREAD_ENONE);

    UCHAR ucMutexInit = DSIThread_MutexInit(&mutexTestDone);
    assert(ucMutexInit == DSI_THREAD_ENONE);

    Q_UNUSED(ucMutexInit);
    Q_UNUSED(ucCondInit);


    // Create Serial object.
    pclSerialObject = new DSISerialGeneric();
    //assert(pclSerialObject);

    bStatus = pclSerialObject->AutoInit();
    //assert(bStatus);


    qDebug() << "Ok AutoInit result" << bStatus;

    // Create Framer object.
    pclMessageObject = new DSIFramerANT(pclSerialObject);
    //assert(pclMessageObject);

    // Initialize Framer object.
    bStatus = pclMessageObject->Init();
    //assert(bStatus);

    qDebug() << "pclMessageObject.Init Status:" << bStatus;


    // Let Serial know about Framer.
    pclSerialObject->SetCallback(pclMessageObject);

    // Open Serial.
    bStatus = pclSerialObject->Open();

    qDebug() << "pclSerialObjectOpen Status:" << bStatus;



    // If the Open function failed, most likely the device
    // we are trying to access does not exist, or it is connected
    // to another program
    if(!bStatus) {
        qDebug() << "failed to open this stick";
        gotAntStick = false;
        emit stickFound(false, "", stickNumber);
        return false;
    }


    // Save USB stick info
    pclMessageObject->GetDeviceUSBVID(usDeviceVID);
    pclMessageObject->GetDeviceUSBPID(usDevicePID);
    //    pclMessageObject->GetDeviceUSBInfo(pclSerialObject->GetDeviceNumber(), aucDeviceDescription, aucDeviceSerial, USB_MAX_STRLEN);
    pclSerialObject->GetDeviceSerialString(aucDeviceSerialString, USB_MAX_STRLEN);


    QString serialString = Util::getStringFromUCHAR(aucDeviceSerialString);


    qDebug() << "OK we found this stick, serial:" << serialString;


    pclMessageObject->ResetSystem();
    DSIThread_Sleep(1200);


    gotAntStick = true;
    emit stickFound(true, serialString, stickNumber);


    //Reveived Only?
    openScanningModeChannel(false);



    return true;

}


////////////////////////////////////////////////////////////////////////////////
/// Close connection to USB stick.
////////////////////////////////////////////////////////////////////////////////
void Hub::close() {

    // Reset system
    qDebug() << "Resetting module...";
    pclMessageObject->ResetSystem();
    DSIThread_Sleep(1000);

    //Wait for test to be done
    DSIThread_MutexLock(&mutexTestDone);

    //    UCHAR ucWaitResult = DSIThread_CondTimedWait(&condTestDone, &mutexTestDone, DSI_THREAD_INFINITE);
    //    assert(ucWaitResult == DSI_THREAD_ENONE);

    DSIThread_MutexUnlock(&mutexTestDone);

    //Destroy mutex and condition var
    DSIThread_MutexDestroy(&mutexTestDone);
    DSIThread_CondDestroy(&condTestDone);


    //Close all stuff
    if(pclSerialObject)
        pclSerialObject->Close();


#if defined(DEBUG_FILE)
    DSIDebug::Close();
#endif

}



/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void Hub::openScanningModeChannel(bool receiveOnly) {


    ///------------ Continuous Scanning Mode! - voir page 4 document ANT_ANT14
    qDebug() << "configScanningModeChannel";


    BOOL bStatus = true;

    UCHAR ucChannel = 0;
    UCHAR ucChannelType = 0;
    USHORT usDeviceNo = 0;
    UCHAR ucDeviceType = 0;
    UCHAR ucTransType = 0;
    UCHAR ucRFFreq = 57;

    if(receiveOnly)
        ucChannelType = 0x40;
    else
        ucChannelType = 0x00;




    // ---- Set NetworkKey
    UCHAR ucNetKey[8] = USER_NETWORK_KEY;
    bStatus = pclMessageObject->SetNetworkKey(USER_NETWORK_NUM, ucNetKey, MESSAGE_TIMEOUT);
    if (bStatus)
        qDebug() << "Sucess SetNetworkKey Continuous";
    else
        qDebug() << "Error SetNetworkKey Continuous";


    // -- AssignChannel - Using public network, so no need to set network key
    bStatus = pclMessageObject->AssignChannel(ucChannel, ucChannelType, USER_NETWORK_NUM);
    if (bStatus) {
        qDebug() << "Sucess AssignChannel Continuous";
        //        vecChannelUsed.append(0);
    }
    else
        qDebug() << "Error AssignChannel Continuous";


    // -- Set channel ID mask
    bStatus = pclMessageObject->SetChannelID(ucChannel, usDeviceNo, ucDeviceType, ucTransType);
    if (bStatus)
        qDebug() << "Sucess SetChannelID Continuous";
    else
        qDebug() << "Error SetChannelID Continuous";


    // -- Set RF frequency
    bStatus = pclMessageObject->SetChannelRFFrequency(ucChannel, ucRFFreq);
    if (bStatus)
        qDebug() << "Sucess SetChannelRFFrequency Continuous";
    else
        qDebug() << "Error SetChannelRFFrequency Continuous";


    // -- Enable extended messages
    pclMessageObject->RxExtMesgsEnable(TRUE);
    if (bStatus)
        qDebug() << "Sucess RxExtMesgsEnable Continuous";
    else
        qDebug() << "Error RxExtMesgsEnable Continuous";


    bStatus = pclMessageObject->OpenRxScanMode();
    if (bStatus)
        qDebug() << "openScanningModeChannel Success";
    else
        qDebug() << "openScanningModeChannel Error";


    currentLastChannel = 1;
    decodeMsgNow = false;
    pairingMode = false;


    uiDSIThread = DSIThread_CreateThread(&Hub::RunMessageThread, this);
    qDebug() << "Decoders started and ready to read!";


}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void Hub::closeScanningModeChannel(bool closeShop) {


    bool bStatus = true;



    // Only close 1 channel (Channel 0), other channel are virtual channels?
    //    foreach(int chan, vecChannelUsed) {

    //        qDebug() << "should close this channel.." << chan;
    //        bStatus = pclMessageObject->CloseChannel(chan);
    //        if (bStatus)
    //            qDebug() << "CloseChannel Success" << chan;
    //        else
    //            qDebug() << "CloseChannel Error" << chan;

    //        bStatus = pclMessageObject->UnAssignChannel(chan);
    //        if (bStatus)
    //            qDebug() << "UnAssignChannel Success" << chan;
    //        else
    //            qDebug() << "UnAssignChannel Error" << chan;
    //    }
    //    qDebug() << "done clearing channels!";


    //--- Close main CSM channel
    qDebug() << "should close this channel.." << 0;
    bStatus = pclMessageObject->CloseChannel(0);
    if (bStatus)
        qDebug() << "CloseChannel Success" << 0;
    else
        qDebug() << "CloseChannel Error" << 0;

    bStatus = pclMessageObject->UnAssignChannel(0);
    if (bStatus)
        qDebug() << "UnAssignChannel Success" << 0;
    else
        qDebug() << "UnAssignChannel Error" << 0;



    if (closeShop) {
        qDebug() << "closing Shop!";
        this->close();
        qDebug() << "close shop done";

        //        if(pclMessageObject)
        //            delete pclMessageObject;
        //        if(pclSerialObject)
        //            delete pclSerialObject;
    }
    emit closeCSMFinished();

}







/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void Hub::setSlope(int antID, double slope) {


    qDebug() << "HUB SET SLOPE" << slope << "to AntId:" << antID;

    //check first set Load call, delete lst SensorNear to refresh it and get the most close sensors first
    if (firstCommandTrainer) {
        hashSensorFecNear.clear();
        firstCommandTrainer = false;
        qDebug() << "HUB# "<< stickNumber << " lst Close sensor cleared!";
    }


    if (hashSensorFecNear.contains(antID)) {
        qDebug() << "ok this sensor is close, send slope to it!" << antID;

        //return true if channel already openned, if not openned ask for permission to open
        bool readyForSendingData = checkToOpenChannelFEC(antID);

        if (readyForSendingData) {
            //Get the channel of this device
            int channelNumber = hashSendingChannel.value(antID);
            uint8_t aucTxBuf[ANT_STANDARD_DATA_PAYLOAD_SIZE];
            FEC_Controller::EncodeTrainerTrackResistancePercentage(aucTxBuf, slope);
            pclMessageObject->SendAcknowledgedData(channelNumber, aucTxBuf);
        }
    }
    else {
        qDebug() << "HUB#" << stickNumber << "this sensor is too far, do not send slope to it..." << antID;
    }

}


/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void Hub::setLoad(int antID, double load) {


    //check first set Load call, delete lst SensorNear to refresh it and get the most close sensors first
    if (firstCommandTrainer) {
        hashSensorFecNear.clear();
        firstCommandTrainer = false;
        qDebug() << "HUB# "<< stickNumber << " lst Close sensor cleared!";
    }



    if (hashSensorFecNear.contains(antID)) {
        qDebug() << "HUB#" << stickNumber << "ok this sensor is close, send Load to it!" << antID;

        //return true if channel already openned, if not openned ask for permission to open
        bool readyForSendingData = checkToOpenChannelFEC(antID);

        if (readyForSendingData) {
            //Get the channel of this device
            int channelNumber = hashSendingChannel.value(antID);
            uint8_t aucTxBuf[ANT_STANDARD_DATA_PAYLOAD_SIZE];
            FEC_Controller::EncodeTrainerTargetPower(aucTxBuf, load);
            pclMessageObject->SendAcknowledgedData(channelNumber, aucTxBuf);
        }
    }
    else {
        qDebug() << "HUB#" << stickNumber << "this sensor is too far, do not send load to it..." << antID;
    }





}



// return true = channel is ready for sending data
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool Hub::checkToOpenChannelFEC(int antID) {

    qDebug() << "HUB#" << stickNumber << "checkToOpenChannelFEC";

    // Channel already open
    if (hashSendingChannel.contains(antID)) {
        return true;
    }
    else if (currentLastChannel < 8 && !hashSendingChannel.contains(antID)) {
        emit askPermissionForSensor(antID, stickNumber);
    }
    return false;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void Hub::addToControlListHub(int antID, int hubNumber) {

    qDebug() << "HUB#" << stickNumber << "OK Granted! add to control list hub-- antID:" << antID << "hubNumber" << hubNumber;

    if (hubNumber != stickNumber) {
        qDebug() << "HUB#" << stickNumber  << "Not for my Thread, ignore this!";
        return;
    }

    //Granted! open channel and set channel id?
    qDebug() << "HUB#" << stickNumber <<  "open channel for this device!" << antID;
    bool bStatus =  pclMessageObject->AssignChannel(currentLastChannel, 0, USER_NETWORK_NUM);
    if (bStatus) {
        bStatus = pclMessageObject->SetChannelID(currentLastChannel, antID, constants::fecDeviceType, constants::transTypeFec);
    }
    else {
        qDebug() << "HUB#" << stickNumber <<  "error AssignChannel for this device!";
    }
    if (bStatus) {
        hashSendingChannel.insert(antID, currentLastChannel);
        currentLastChannel++;
    }
    else {
        qDebug() << "HUB#" << stickNumber <<  "error, should try again!";
    }

}


/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void Hub::configureSendChannelFEC(int antID) {

    qDebug() << "configureSendChannelPM Now";
    if (currentSendingChannel != constants::fecChannelNumber) {
        pclMessageObject->AssignChannel(constants::fecChannelNumber, 0, USER_NETWORK_NUM);
        currentSendingChannel = constants::fecChannelNumber;
    }
    pclMessageObject->SetChannelID(constants::fecChannelNumber, antID, constants::fecDeviceType, constants::transTypeFec);
}


/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void Hub::configureSendChannelPM(int antID) {

    qDebug() << "configureSendChannelPM Now";
    if (currentSendingChannel != constants::pmChannelNumber) {
        pclMessageObject->AssignChannel(constants::pmChannelNumber, 0, USER_NETWORK_NUM);
        currentSendingChannel = constants::pmChannelNumber;
    }
    pclMessageObject->SetChannelID(constants::pmChannelNumber, antID, constants::powerDeviceType, constants::transTypePower);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void Hub::configureSendChannelOxy(int antID) {

    qDebug() << "configureSendChannelOxy Now";
    if (currentSendingChannel != constants::oxyChannelNumber) {
        pclMessageObject->AssignChannel(constants::oxyChannelNumber, 0, USER_NETWORK_NUM);
        currentSendingChannel = constants::oxyChannelNumber;
    }
    pclMessageObject->SetChannelID(constants::oxyChannelNumber, antID, constants::oxyDeviceType, constants::transTypeOxy);

}




/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void Hub::sendOxygenCommand(int antID, Oxygen_Controller::COMMAND command) {


    qDebug() << "Hub::sendCommandOxygen" << antID;

    if (!gotAntStick) {
        qDebug() << "HUB#" << stickNumber << "No stick here, leave!";
        return;
    }

    if (!hashSensorOxy.contains(antID)) {
        qDebug() << "We dont have this sensor.. leave";
        return;
    }


    configureSendChannelOxy(antID);

    uint8_t aucTxBuf[ANT_STANDARD_DATA_PAYLOAD_SIZE];
    uint8_t typeCommand = 0x00;

    if (command == Oxygen_Controller::COMMAND::SET_TIME) {
        typeCommand = 0x00;
    }
    else if (command == Oxygen_Controller::COMMAND::START_SESSION) {
        typeCommand = 0x01;
    }
    else if (command == Oxygen_Controller::COMMAND::STOP_SESSION) {
        typeCommand = 0x02;
    }
    else { //COMMAND::LAP
        typeCommand = 0x03;
    }

    QDateTime currentDateTime = QDateTime::currentDateTime();
    int offsetSecond =  currentDateTime.offsetFromUtc();
    // Signed 2’s complement value indicating the local time offset in 15min intervals ranging from -60 (UTC -15hours) to +60 (UTC +15hours). Use 0x7F for invalid.
    int8_t offset15min  = offsetSecond /3600*4;

    QDate dateRef(1989,12,31);
    QTime timeRef(0,0,0);
    QDateTime dateTimeRef(dateRef, timeRef);
    // Current Time in seconds since UTC 00:00 Dec 31 1989.
    uint64_t timeDiff = dateTimeRef.secsTo(currentDateTime);

    aucTxBuf[0] =  0x10;
    aucTxBuf[1] =  typeCommand;
    aucTxBuf[2] =  0xFF;
    aucTxBuf[3] =  offset15min;
    aucTxBuf[4] =  (uint8_t)timeDiff;
    aucTxBuf[5] =  (uint8_t)(timeDiff >> 8);
    aucTxBuf[6] =  (uint8_t)(timeDiff >> 16);
    aucTxBuf[7] =  (uint8_t)(timeDiff >> 24);

    pclMessageObject->SendAcknowledgedData(constants::oxyChannelNumber, aucTxBuf);
    //    }

}



//-----------------------------------------------------------------------------------------------------------------
void Hub::sendCalibrationPM(int andID, bool manual) {

    qDebug() << "HUB send calibrationPM";


    if (!gotAntStick) {
        qDebug() << "HUB#" << stickNumber << "No stick here, leave!";
        return;
    }


    configureSendChannelPM(andID);

    vecPowerController.at(0)->newCalibrationStarted();
    calibrationReponseReceived = false;


    UCHAR aucTxBuf[ANT_STANDARD_DATA_PAYLOAD_SIZE];

    if (manual) {
        vecPowerController.at(0)->isDoingAutoZero = false;
        // Manual Calibration request - General Calibration Request Main Data Page (0xAA)
        // format page
        aucTxBuf[0] = BPS_PAGE_1; //1
        aucTxBuf[1] = PBS_CID_170;  // 0xAA
        aucTxBuf[2] = BPS_PAGE_RESERVE_BYTE;
        aucTxBuf[3] = BPS_PAGE_RESERVE_BYTE;
        aucTxBuf[4] = BPS_PAGE_RESERVE_BYTE;
        aucTxBuf[5] = BPS_PAGE_RESERVE_BYTE;
        aucTxBuf[6] = BPS_PAGE_RESERVE_BYTE;
        aucTxBuf[7] = BPS_PAGE_RESERVE_BYTE;
    }
    //AUTO-ZERO
    else {
        vecPowerController.at(0)->isDoingAutoZero = true;
        // AutoZero Calibration request - Auto Zero Configuration Main Data Page (0xAB)
        // format page
        aucTxBuf[0] = BPS_PAGE_1;
        aucTxBuf[1] = PBS_CID_171;   //0xAB
        aucTxBuf[2] = vecPowerController.at(0)->ucLocalAutoZeroStatus; //0x00 – Auto Zero OFF, 0x01 – Auto Zero ON, 0xFF – Auto Zero Not Supported
        aucTxBuf[3] = BPS_PAGE_RESERVE_BYTE;
        aucTxBuf[4] = BPS_PAGE_RESERVE_BYTE;
        aucTxBuf[5] = BPS_PAGE_RESERVE_BYTE;
        aucTxBuf[6] = BPS_PAGE_RESERVE_BYTE;
        aucTxBuf[7] = BPS_PAGE_RESERVE_BYTE;
    }

    qDebug() << "send calib ack data now!";
    pclMessageObject->SendAcknowledgedData(constants::pmChannelNumber, aucTxBuf);
}

//-----------------------------------------------------------------------------------------------
void Hub::sendCalibrationFEC(int antID, FEC_Controller::CALIBRATION_TYPE calibration) {

    qDebug() << "Hub::sendCalibrationFEC";

    if (!gotAntStick) {
        qDebug() << "HUB#" << stickNumber << "No stick here, leave!";
        return;
    }

    configureSendChannelFEC(antID);

    qDebug() << "sendCalibrationFEC::calibrationRequested" << calibration;

    uint8_t aucTxBuf[ANT_STANDARD_DATA_PAYLOAD_SIZE];
    uint8_t typeCalibration = 0;

    if (calibration == FEC_Controller::ZERO_OFFSET) {
        typeCalibration = 64;  //0x06
    }
    else {  // SPIN_DOWN
        typeCalibration = 128; //0x07
    }

    aucTxBuf[0] =  0x01;
    aucTxBuf[1] =  typeCalibration;
    aucTxBuf[2] =  0x00;
    aucTxBuf[3] =  0xFF;
    aucTxBuf[4] =  0xFF;
    aucTxBuf[5] =  0xFF;
    aucTxBuf[6] =  0xFF;
    aucTxBuf[7] =  0xFF;


    // Send data
    pclMessageObject->SendAcknowledgedData(constants::fecChannelNumber, aucTxBuf);

}






/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void Hub::setUserStudioData(QVector<UserStudio> vecUserStudio) {

    qDebug() << "Hub..setUserStudioData" << stickNumber;



    for (int i=0; i<constants::nbMaxUserStudio; i++) {

        UserStudio userStudio = vecUserStudio.at(i);

        bool pmUsedForCadence = true;
        bool pmUsedForSpeed = true;

        if (userStudio.getHrID() > 0) {
            hashSensorId.insert(userStudio.getHrID(), i+1);
            hashSensorHr.insert(userStudio.getHrID(), i+1);
        }
        if (userStudio.getCadenceID() > 0 && userStudio.getCadenceID() != userStudio.getSpeedID() && userStudio.getCadenceID() != userStudio.getPowerID()) {
            hashSensorId.insert(userStudio.getCadenceID(), i+1);
            hashSensorCad.insert(userStudio.getCadenceID(), i+1);
            pmUsedForCadence = false;
        }
        if (userStudio.getSpeedID() > 0 && userStudio.getCadenceID() != userStudio.getSpeedID() && userStudio.getSpeedID() != userStudio.getPowerID()) {
            hashSensorId.insert(userStudio.getSpeedID(), i+1);
            hashSensorSpeed.insert(userStudio.getSpeedID(), i+1);
            pmUsedForSpeed = false;
        }
        if (userStudio.getCadenceID() > 0 && userStudio.getCadenceID() == userStudio.getSpeedID() && userStudio.getCadenceID() != userStudio.getPowerID()) {
            hashSensorId.insert(userStudio.getCadenceID(), i+1);
            hashSensorSpeedCad.insert(userStudio.getCadenceID(), i+1);
            pmUsedForCadence = false;
            pmUsedForSpeed = false;
        }
        if (userStudio.getFecID() > 0) {
            hashSensorId.insert(userStudio.getFecID(), i+1);
            hashSensorFEC.insert(userStudio.getFecID(), i+1);
        }
        if (userStudio.getPowerID() > 0) {
            hashSensorId.insert(userStudio.getPowerID(), i+1);
            hashSensorPower.insert(userStudio.getPowerID(), i+1);  
        }


        qDebug() << "User Studio power Curve is__hub:" << userStudio.getPowerCurve().getFullName() << userStudio.getPowerCurve().getId();

        //----------------- Create a controller for each user ---------------------------------------------
        HeartRate_Controller *mHeartRate_Controller = new HeartRate_Controller(i+1, this);
        connect(mHeartRate_Controller, SIGNAL(HeartRateChanged(int, int)), this, SIGNAL(signal_hr(int, int)) );
        connect(mHeartRate_Controller, SIGNAL(batteryLow(QString,int,int)), this, SIGNAL(signal_batteryLow(QString,int,int)) );
        vecHrController.append(mHeartRate_Controller);

        Cadence_Controller *mCadence_Controller = new Cadence_Controller(i+1, this);
        connect(mCadence_Controller, SIGNAL(cadenceChanged(int, int)), this, SIGNAL(signal_cadence(int, int)) );
        vecCadController.append(mCadence_Controller);

        Oxygen_Controller *mOxygen_Controller = new Oxygen_Controller(i+1, this);
        connect(mOxygen_Controller, SIGNAL(oxygenValueChanged(int, double,double)), this, SIGNAL(signal_oxygenValueChanged(int, double,double)) );
        connect(mOxygen_Controller, SIGNAL(batteryLow(QString,int,int)), this, SIGNAL(signal_batteryLow(QString,int,int)) );
        vecOxyController.append(mOxygen_Controller);


        Speed_Controller *mSpeed_Controller = new Speed_Controller(i+1, userStudio.getPowerCurve(), userStudio.getWheelCircMM(), this);
        connect(mSpeed_Controller, SIGNAL(speedChanged(int, double)), this, SIGNAL(signal_speed(int, double)) );
        connect(mSpeed_Controller, SIGNAL(powerChanged(int, int)), this, SIGNAL(signal_power(int, int)) );
        vecSpeedController.append(mSpeed_Controller);

        CombinedSC_Controller *mCombinedSC_Controller = new CombinedSC_Controller(i+1, userStudio.getPowerCurve(), userStudio.getWheelCircMM(), this);
        connect(mCombinedSC_Controller, SIGNAL(cadenceChanged(int, int)), this, SIGNAL(signal_cadence(int, int)) );
        connect(mCombinedSC_Controller, SIGNAL(speedChanged(int, double)), this, SIGNAL(signal_speed(int, double)) );
        connect(mCombinedSC_Controller, SIGNAL(powerChanged(int, int)), this, SIGNAL(signal_power(int, int)) );
        vecSpeedCadController.append(mCombinedSC_Controller);

        Power_Controller *mPower_Controller = new Power_Controller(i+1, this);
        mPower_Controller->usingForCadence = pmUsedForCadence;
        mPower_Controller->usingForSpeed = pmUsedForSpeed;
        mPower_Controller->wheelSize = userStudio.getWheelCircMM();
        connect(mPower_Controller, SIGNAL(powerChanged(int, int)), this, SIGNAL(signal_power(int, int)) );
        connect(mPower_Controller, SIGNAL(speedChanged(int, double)), this, SIGNAL(signal_speed(int, double)) );
        connect(mPower_Controller, SIGNAL(cadenceChanged(int, int)), this, SIGNAL(signal_cadence(int, int)) );
        connect(mPower_Controller, SIGNAL(rightPedalBalanceChanged(int, int)), this, SIGNAL(signal_rightPedal(int, int)) );
        connect(mPower_Controller, SIGNAL(calibrationOverWithStatus(bool,QString,int)), this, SIGNAL(signal_powerCalibrationOverWithStatus(bool,QString,int)) );
        connect(mPower_Controller, SIGNAL(supportAutoZero()), this, SIGNAL(signal_powerSupportAutoZero()) );
        connect(mPower_Controller, SIGNAL(batteryLow(QString,int,int)), this, SIGNAL(signal_batteryLow(QString,int,int)) );
        connect(mPower_Controller, SIGNAL(calibrationProgress(int,double,double,double,double,double,double,double,double,double,double,double)),
                this, SIGNAL(calibrationProgressPM(int,double,double,double,double,double,double,double,double,double,double,double)) );
        connect(mPower_Controller, SIGNAL(pedalMetricChanged(int, double,double,double,double,double)),
                this, SIGNAL(pedalMetricChanged(int, double,double,double,double,double)));
        vecPowerController.append(mPower_Controller);


        FEC_Controller *mFec_Controller = new FEC_Controller(i+1, this);
        mFec_Controller->wheelSizeMeters = userStudio.getWheelCircMM()/1000.0;
        connect(mFec_Controller, SIGNAL(hrChanged(int, int)), this, SIGNAL(signal_hr(int, int)) );
        connect(mFec_Controller, SIGNAL(speedChanged(int, double)), this, SIGNAL(signal_speed(int, double)) );
        connect(mFec_Controller, SIGNAL(powerChanged(int, int)), this, SIGNAL(signal_power(int, int)) );
        connect(mFec_Controller, SIGNAL(cadenceChanged(int, int)), this, SIGNAL(signal_cadence(int, int)) );
        connect(mFec_Controller, SIGNAL(lapChanged(int)), this, SIGNAL(signal_lapChanged(int)) );
        //        connect(mFec_Controller, SIGNAL(batteryLow(QString,int)), this, SIGNAL(signal_batteryLow(QString,int)) );
        connect(mFec_Controller, SIGNAL(calibrationInProgress(bool,bool,FEC_Controller::TEMPERATURE_CONDITION,FEC_Controller::SPEED_CONDITION,double,double,double)),
                this, SIGNAL(calibrationInProgress(bool,bool,FEC_Controller::TEMPERATURE_CONDITION,FEC_Controller::SPEED_CONDITION,double,double,double)) );
        connect(mFec_Controller, SIGNAL(calibrationOver(bool,bool,double,double,double)), this, SIGNAL(calibrationOver(bool,bool,double,double,double)) );
        vecFecController.append(mFec_Controller);

    }


    qDebug() << "hashSensorId ID list:" << hashSensorId.size();
    qDebug() << "hashSensorHr ID list:" << hashSensorHr.size();
    qDebug() << "hashSensorCad ID list:" << hashSensorCad.size();
    qDebug() << "hashSensorSpeed ID list:" << hashSensorSpeed.size();
    qDebug() << "hashSensorSpeedCad ID list:" << hashSensorSpeedCad.size();
    qDebug() << "hashSensorOxy ID list:" << hashSensorOxy.size();
    qDebug() << "hashSensorFEC ID list:" << hashSensorFEC.size();
    qDebug() << "hashSensorPower ID list:" << hashSensorPower.size();


    startSensors();
}


/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void Hub::setSoloDataToHub(PowerCurve curve, int wheelCircMM, QList<Sensor> lstSensor, bool usePmForCadence, bool usePmForSpeed) {

    qDebug() << "setDataToHub ID:" << stickNumber << curve.getId() << "wheelCircMM" << wheelCircMM << "usePmForCadence" << usePmForCadence << "usePmForSpeed" << usePmForSpeed;
    qDebug() << "PowerCurve is: " << curve.getFullName() << " curve ID" << curve.getId();


    //----------------------------------------------
    int userID = 1;

    foreach(Sensor s, lstSensor) {

        hashSensorId.insert(s.getAntId(), userID);

        qDebug() << "DeviceTYPE?" << s.getDeviceType();

        if (s.getDeviceType() == constants::hrDeviceType) {
            hashSensorHr.insert(s.getAntId(), userID);
        }
        else if (s.getDeviceType() == constants::cadDeviceType) {
            hashSensorCad.insert(s.getAntId(), userID);
        }
        else if (s.getDeviceType() == constants::speedCadDeviceType) {
            hashSensorSpeedCad.insert(s.getAntId(), userID);
        }
        else if (s.getDeviceType() == constants::speedDeviceType) {
            hashSensorSpeed.insert(s.getAntId(), userID);
        }
        else if (s.getDeviceType() == constants::powerDeviceType) {
            hashSensorPower.insert(s.getAntId(), userID);
        }
        else if (s.getDeviceType() == constants::fecDeviceType) {
            hashSensorFEC.insert(s.getAntId(), userID);
        }
        else if (s.getDeviceType() == constants::oxyDeviceType) {
            hashSensorOxy.insert(s.getAntId(), userID);
        }
    }

    qDebug() << "hashSensorId ID list:" << hashSensorId.size();
    qDebug() << "hashSensorHr ID list:" << hashSensorHr.size();
    qDebug() << "hashSensorCad ID list:" << hashSensorCad.size();
    qDebug() << "hashSensorSpeed ID list:" << hashSensorSpeed.size();
    qDebug() << "hashSensorSpeedCad ID list:" << hashSensorSpeedCad.size();
    qDebug() << "hashSensorOxy ID list:" << hashSensorOxy.size();
    qDebug() << "hashSensorFEC ID list:" << hashSensorFEC.size();
    qDebug() << "hashSensorPower ID list:" << hashSensorPower.size();




    qDebug() << "Ok create controller here!";

    //    if (powerCurve.getId() > 0 )
    //        usingTrainerCurve = true;


    // ------------------------------ Start general decoders -------------------------------
    HeartRate_Controller *mHeartRate_Controller = new HeartRate_Controller(1, this);
    connect(mHeartRate_Controller, SIGNAL(HeartRateChanged(int, int)), this, SIGNAL(signal_hr(int, int)) );
    connect(mHeartRate_Controller, SIGNAL(batteryLow(QString,int,int)), this, SIGNAL(signal_batteryLow(QString,int,int)) );
    vecHrController.append(mHeartRate_Controller);


    Cadence_Controller *mCadence_Controller = new Cadence_Controller(1, this);
    connect(mCadence_Controller, SIGNAL(cadenceChanged(int, int)), this, SIGNAL(signal_cadence(int, int)) );
    vecCadController.append(mCadence_Controller);


    Oxygen_Controller *mOxygen_Controller = new Oxygen_Controller(1, this);
    connect(mOxygen_Controller, SIGNAL(oxygenValueChanged(int, double,double)), this, SIGNAL(signal_oxygenValueChanged(int, double,double)) );
    connect(mOxygen_Controller, SIGNAL(batteryLow(QString,int,int)), this, SIGNAL(signal_batteryLow(QString,int,int)) );
    vecOxyController.append(mOxygen_Controller);


    Speed_Controller *mSpeed_Controller = new Speed_Controller(1, curve, wheelCircMM, this);
    connect(mSpeed_Controller, SIGNAL(speedChanged(int, double)), this, SIGNAL(signal_speed(int, double)) );
    connect(mSpeed_Controller, SIGNAL(powerChanged(int, int)), this, SIGNAL(signal_power(int, int)) );
    vecSpeedController.append(mSpeed_Controller);


    CombinedSC_Controller *mCombinedSC_Controller = new CombinedSC_Controller(1, curve, wheelCircMM, this);
    if (hashSensorPower.size() > 0) {
        mCombinedSC_Controller->ignoreCadence = usePmForCadence;
        mCombinedSC_Controller->ignoreSpeed = usePmForSpeed;
    }
    connect(mCombinedSC_Controller, SIGNAL(cadenceChanged(int, int)), this, SIGNAL(signal_cadence(int, int)) );
    connect(mCombinedSC_Controller, SIGNAL(speedChanged(int, double)), this, SIGNAL(signal_speed(int, double)) );
    connect(mCombinedSC_Controller, SIGNAL(powerChanged(int, int)), this, SIGNAL(signal_power(int, int)) );
    vecSpeedCadController.append(mCombinedSC_Controller);


    Power_Controller *mPower_Controller = new Power_Controller(1, this);
    mPower_Controller->usingForCadence = usePmForCadence;
    mPower_Controller->usingForSpeed = usePmForSpeed;
    mPower_Controller->wheelSize = wheelCircMM;
    connect(mPower_Controller, SIGNAL(powerChanged(int, int)), this, SIGNAL(signal_power(int, int)) );
    connect(mPower_Controller, SIGNAL(speedChanged(int, double)), this, SIGNAL(signal_speed(int, double)) );
    connect(mPower_Controller, SIGNAL(cadenceChanged(int, int)), this, SIGNAL(signal_cadence(int, int)) );
    connect(mPower_Controller, SIGNAL(rightPedalBalanceChanged(int, int)), this, SIGNAL(signal_rightPedal(int, int)) );
    connect(mPower_Controller, SIGNAL(calibrationOverWithStatus(bool,QString,int)), this, SIGNAL(signal_powerCalibrationOverWithStatus(bool,QString,int)) );
    connect(mPower_Controller, SIGNAL(supportAutoZero()), this, SIGNAL(signal_powerSupportAutoZero()) );
    connect(mPower_Controller, SIGNAL(batteryLow(QString,int,int)), this, SIGNAL(signal_batteryLow(QString,int,int)) );
    connect(mPower_Controller, SIGNAL(calibrationProgress(int,double,double,double,double,double,double,double,double,double,double,double)),
            this, SIGNAL(calibrationProgressPM(int,double,double,double,double,double,double,double,double,double,double,double)) );
    connect(mPower_Controller, SIGNAL(pedalMetricChanged(int, double,double,double,double,double)),
            this, SIGNAL(pedalMetricChanged(int, double,double,double,double,double)));
    vecPowerController.append(mPower_Controller);


    FEC_Controller *mFec_Controller = new FEC_Controller(1, this);
    mFec_Controller->wheelSizeMeters = wheelCircMM/1000.0;
    connect(mFec_Controller, SIGNAL(hrChanged(int, int)), this, SIGNAL(signal_hr(int, int)) );
    connect(mFec_Controller, SIGNAL(speedChanged(int, double)), this, SIGNAL(signal_speed(int, double)) );
    connect(mFec_Controller, SIGNAL(powerChanged(int, int)), this, SIGNAL(signal_power(int, int)) );
    connect(mFec_Controller, SIGNAL(cadenceChanged(int, int)), this, SIGNAL(signal_cadence(int, int)) );
    connect(mFec_Controller, SIGNAL(lapChanged(int)), this, SIGNAL(signal_lapChanged(int)) );
    //        connect(mFec_Controller, SIGNAL(batteryLow(QString,int)), this, SIGNAL(signal_batteryLow(QString,int)) );
    connect(mFec_Controller, SIGNAL(calibrationInProgress(bool,bool,FEC_Controller::TEMPERATURE_CONDITION,FEC_Controller::SPEED_CONDITION,double,double,double)),
            this, SIGNAL(calibrationInProgress(bool,bool,FEC_Controller::TEMPERATURE_CONDITION,FEC_Controller::SPEED_CONDITION,double,double,double)) );
    connect(mFec_Controller, SIGNAL(calibrationOver(bool,bool,double,double,double)), this, SIGNAL(calibrationOver(bool,bool,double,double,double)) );
    vecFecController.append(mFec_Controller);



    startSensors();
}



/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void Hub::startSensors() {


    qDebug () << "Hub Starting sensor---!";

    firstCommandTrainer = true;
    pairingMode = false;

    //    fecTrainerID = -1;
    //    fecTrainerChanConfigured = false;

    //    kickrDeviceID = -1;
    //    powerDeviceID = -1;
    //    pmChanConfigured = false;

    //    oxyDeviceID = -1;
    //    oxyChanConfigured = false;


    decodeMsgNow = true;


    qDebug() << "LOOP DONE!..startSensors";

}

////////////////////////////////////////////////////////////////////////////////////
void Hub::stopDecodingMsg() {

    qDebug() << "Hub, stop decoding Msg!";


    //    fecTrainerID = -1;
    //    fecTrainerChanConfigured = false;

    //    kickrDeviceID = -1;
    //    powerDeviceID = -1;
    //    pmChanConfigured = false;

    //    oxyDeviceID = -1;
    //    oxyChanConfigured = false;

    decodeMsgNow = false;
    currentSendingChannel = -1;
    currentLastChannel = 1;


    hashSensorId.clear();
    hashSensorHr.clear();
    hashSensorCad.clear();
    hashSensorSpeed.clear();
    hashSensorSpeedCad.clear();
    hashSensorOxy.clear();
    hashSensorFEC.clear();
    hashSensorPower.clear();

    hashSensorFecNear.clear();
    hashSendingChannel.clear();

    //    hashSensorKickr.clear();


    //---- Clear previous Controllers
    qDeleteAll(vecHrController);
    qDeleteAll(vecCadController);
    qDeleteAll(vecOxyController);
    qDeleteAll(vecSpeedController);
    qDeleteAll(vecSpeedCadController);
    qDeleteAll(vecPowerController);
    qDeleteAll(vecFecController);

    vecHrController.clear();
    vecCadController.clear();
    vecOxyController.clear();
    vecSpeedController.clear();
    vecSpeedCadController.clear();
    vecPowerController.clear();
    vecFecController.clear();

    qDebug() << "Hub, stop decoding - object cleared!";
}





/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void Hub::startPairing(int deviceType, bool stopPairingWhenDeviceFound, int secToLook, bool fromStudioPage) {

//    if (!gotAntStick) {
//        QList<int> lstBidon;
//        emit sensorFound(deviceType, 0, lstBidon, lstBidon, fromStudioPage);
//    }


    qDebug() << "HUB#" << stickNumber << " Start Pairing! deviceType " << deviceType  <<  "stopPairingWhenDeviceFound" << stopPairingWhenDeviceFound <<  "secToLook" << secToLook << "fromStudioPage" << fromStudioPage;

    pairingMode = true;
    decodeMsgNow = false;
    deviceTypeToLookPairing = deviceType;

    lstDeviceHr.clear();
    lstTypeDeviceHr.clear();    //(0=HR, 1=Power, 2=Cadence, 3=Speed,  4=S&C, 5=FEC, 6=OXYGEN)

    lstDevicePOWER.clear();
    lstTypeDevicePOWER.clear();

    lstDeviceFEC.clear();
    lstTypeDeviceFEC.clear();

    lstDeviceOXY.clear();
    lstTypeDeviceOXY.clear();

    lstDeviceCADENCE_SpeedCadence.clear();
    lstTypeDeviceCADENCE_SpeedCadence.clear();

    lstDeviceSPEED_SpeedCadence.clear();
    lstTypeDeviceSPEED_SpeedCadence.clear();


    QTimer timer;
    timer.setSingleShot(true);
    QEventLoop loop;
    if (stopPairingWhenDeviceFound) {
        timer.start(4000); //default max value, 4sec
        connect(this, SIGNAL(deviceFound()), &loop, SLOT(quit()));
        connect(this, SIGNAL(stopPairing()), &loop, SLOT(quit()));
    }
    else {
        timer.start(secToLook*1000);
        disconnect(this, SIGNAL(deviceFound()), &loop, SLOT(quit()));
    }
    QObject::connect(&timer, SIGNAL(timeout()), &loop, SLOT(quit()));
    loop.exec();


    //--------------
    qDebug() << "Pairing Over StudioMode Search?" << fromStudioPage;

    if (deviceType == constants::hrDeviceType) {
        emit sensorFound(constants::hrDeviceType, lstDeviceHr.size(), lstDeviceHr, lstTypeDeviceHr, fromStudioPage);
    }
    else if (deviceType == constants::powerDeviceType) {
        emit sensorFound(constants::powerDeviceType, lstDevicePOWER.size(), lstDevicePOWER, lstTypeDevicePOWER, fromStudioPage);
    }
    else if (deviceType == constants::speedDeviceType) {
        emit sensorFound(constants::speedDeviceType, lstDeviceSPEED_SpeedCadence.size(), lstDeviceSPEED_SpeedCadence, lstTypeDeviceSPEED_SpeedCadence, fromStudioPage);
    }
    else if (deviceType == constants::cadDeviceType) {
        emit sensorFound(constants::cadDeviceType, lstDeviceCADENCE_SpeedCadence.size(), lstDeviceCADENCE_SpeedCadence, lstTypeDeviceCADENCE_SpeedCadence, fromStudioPage);
    }
    else if (deviceType == constants::fecDeviceType) {
        emit sensorFound(constants::fecDeviceType, lstDeviceFEC.size(), lstDeviceFEC, lstTypeDeviceFEC, fromStudioPage);
    }
    else if (deviceType == constants::oxyDeviceType) {
        emit sensorFound(constants::oxyDeviceType, lstDeviceOXY.size(), lstDeviceOXY, lstTypeDeviceOXY, fromStudioPage);
    }

    pairingMode = false;

}


////////////////////////////////////////////////////////////////////////////////
// RunMessageThread
//
// Callback function that is used to create the thread. This is a static
// function.
//
////////////////////////////////////////////////////////////////////////////////
DSI_THREAD_RETURN Hub::RunMessageThread(void *pvParameter_)
{
    ((Hub*) pvParameter_)->MessageThread();
    return NULL;
}

////////////////////////////////////////////////////////////////////////////////
// MessageThread
//
// Run message thread
////////////////////////////////////////////////////////////////////////////////
void Hub::MessageThread()
{
    ANT_MESSAGE stMessage;
    USHORT usSize;


    while(true) {

        if(pclMessageObject->WaitForMessage(1000)) {

            usSize = pclMessageObject->GetMessage(&stMessage);

            if(usSize == DSI_FRAMER_ERROR) {
                // Get the message to clear the error
                usSize = pclMessageObject->GetMessage(&stMessage, MESG_MAX_SIZE_VALUE);
                continue;
            }
            if(usSize != DSI_FRAMER_ERROR && usSize != DSI_FRAMER_TIMEDOUT && usSize != 0) {
                ProcessMessage(stMessage, usSize);
            }
        }
    }


    qDebug() << "End messageThread";


    DSIThread_MutexLock(&mutexTestDone);
    UCHAR ucCondResult = DSIThread_CondSignal(&condTestDone);
    assert(ucCondResult == DSI_THREAD_ENONE);
    Q_UNUSED(ucCondResult)
    DSIThread_MutexUnlock(&mutexTestDone);
}







////////////////////////////////////////////////////////////////////////////////
/// ProcessMessage
/// Process ALL messages that come from ANT, including event messages.
/// stMessage: Message struct containing message recieved from ANT
////////////////////////////////////////////////////////////////////////////////
void Hub::ProcessMessage(ANT_MESSAGE stMessage, USHORT usSize_)
{

    //    BOOL bStatus;
    BOOL bPrintBuffer = FALSE;
    UCHAR ucDataOffset = MESSAGE_BUFFER_DATA2_INDEX;   // For most data messages

    //    QDebug deb = qDebug();


    switch(stMessage.ucMessageID) {
    case MESG_RESPONSE_EVENT_ID: {
        //RESPONSE TYPE
        switch(stMessage.aucData[1]) {

        case MESG_CHANNEL_SEARCH_TIMEOUT_ID: {
            if(stMessage.aucData[2] != RESPONSE_NO_ERROR) {
                qDebug() << "Error MESG_CHANNEL_SEARCH_TIMEOUT_ID:" << stMessage.aucData[2];
                break;
            }
            qDebug() << "Setting search timeout";
            break;
        }

        case MESG_SET_LP_SEARCH_TIMEOUT_ID: {
            if(stMessage.aucData[2] != RESPONSE_NO_ERROR) {
                qDebug() << "Error MESG_SET_LP_SEARCH_TIMEOUT_ID:" << stMessage.aucData[2];
                break;
            }
            qDebug() << "Setting Low priority search timeout";
            break;
        }

        case MESG_CLOSE_CHANNEL_ID: {
            if(stMessage.aucData[2] != RESPONSE_NO_ERROR) {
                qDebug() << "Error closing channel:" << stMessage.aucData[2];
                break;
            }
            qDebug() << "Closing Channel" << pclMessageObject->GetChannelNumber(&stMessage);
            break;

        }

        case MESG_NETWORK_KEY_ID: {
            if(stMessage.aucData[2] != RESPONSE_NO_ERROR) {
                qDebug() << "Error configuring network key:" << stMessage.aucData[2];
                break;
            }
            qDebug() << "Network key set";
            break;
        }

        case MESG_ASSIGN_CHANNEL_ID: {
            if(stMessage.aucData[2] != RESPONSE_NO_ERROR) {
                qDebug() << "Error assigning channel" << stMessage.aucData[2];
                break;
            }
            qDebug() << "Channel assigned";
            break;
        }

        case MESG_CHANNEL_ID_ID: {
            if(stMessage.aucData[2] != RESPONSE_NO_ERROR) {
                qDebug() << "Error configuring Channel ID:" << stMessage.aucData[2];
                break;
            }
            qDebug() << "Channel ID set";
            break;
        }

        case MESG_CHANNEL_MESG_PERIOD_ID: {
            if(stMessage.aucData[2] != RESPONSE_NO_ERROR) {
                qDebug() << "Error configuring Channel Period:" << stMessage.aucData[2];
                break;
            }
            qDebug() << "Message Period set";
            break;
        }

        case MESG_CHANNEL_RADIO_FREQ_ID: {
            if(stMessage.aucData[2] != RESPONSE_NO_ERROR) {
                qDebug() << "Error configuring Radio Frequency:" << stMessage.aucData[2];
                break;
            }
            qDebug() << "Radio Frequency and period set";
            break;
        }

        case MESG_OPEN_CHANNEL_ID: {
            if(stMessage.aucData[2] != RESPONSE_NO_ERROR) {
                qDebug() << "Error opening channel: Code" << stMessage.aucData[2];
                bBroadcasting = FALSE;
                break;
            }
#if defined (ENABLE_EXTENDED_MESSAGES)
            qDebug() << "Enabling extended messages...";
            pclMessageObject->RxExtMesgsEnable(TRUE);
#endif
            qDebug() << "Channel opened";
            break;
        }

        case MESG_RX_EXT_MESGS_ENABLE_ID: {
            if(stMessage.aucData[2] == INVALID_MESSAGE) {
                qDebug() << "Extended messages not supported in this ANT product";
                break;
            }
            else if(stMessage.aucData[2] != RESPONSE_NO_ERROR) {
                qDebug() << "Error enabling extended messages:" << stMessage.aucData[2];
                break;
            }
            qDebug() << "Extended messages enabled";
            break;
        }

        case MESG_UNASSIGN_CHANNEL_ID: {
            if(stMessage.aucData[2] != RESPONSE_NO_ERROR) {
                qDebug() << "Error unassigning channel: Code" << stMessage.aucData[2];
                break;
            }
            qDebug() << "Channel unasigned!";
            break;
        }


        case MESG_REQUEST_ID: {
            if(stMessage.aucData[2] == INVALID_MESSAGE) {
                qDebug() << "Requested message not supported in this ANT product";
            }
            qDebug() << "REQUESTED ID! -----------";
            break;
        }

        case MESG_EVENT_ID: {

            switch(stMessage.aucData[2]) {

            case EVENT_CHANNEL_CLOSED: {
                qDebug() << "Channel Closed___" << pclMessageObject->GetChannelNumber(&stMessage);
                //                pclMessageObject->UnAssignChannel(pclMessageObject->GetChannelNumber(&stMessage), MESSAGE_TIMEOUT);
                break;
            }


            case EVENT_TX: {
                // This event indicates that a message has just been
                // sent over the air. We take advantage of this event to set
                // up the data for the next message period.
                static UCHAR ucIncrement = 0;      // Increment the first byte of the buffer

                aucTransmitBuffer[0] = ucIncrement++;

                // Broadcast data will be sent over the air on
                // the next message period.
                if(bBroadcasting)
                {
                    qDebug() << "User ANT_CHANNEL?-------\n";
                    pclMessageObject->SendBroadcastData(USER_ANTCHANNEL, aucTransmitBuffer);

                    // Echo what the data will be over the air on the next message period.
                    if(bDisplay)
                    {
                        printf("Tx:(%d): [%02x],[%02x],[%02x],[%02x],[%02x],[%02x],[%02x],[%02x]\n",
                               USER_ANTCHANNEL,
                               aucTransmitBuffer[MESSAGE_BUFFER_DATA1_INDEX],
                               aucTransmitBuffer[MESSAGE_BUFFER_DATA2_INDEX],
                               aucTransmitBuffer[MESSAGE_BUFFER_DATA3_INDEX],
                               aucTransmitBuffer[MESSAGE_BUFFER_DATA4_INDEX],
                               aucTransmitBuffer[MESSAGE_BUFFER_DATA5_INDEX],
                               aucTransmitBuffer[MESSAGE_BUFFER_DATA6_INDEX],
                               aucTransmitBuffer[MESSAGE_BUFFER_DATA7_INDEX],
                               aucTransmitBuffer[MESSAGE_BUFFER_DATA8_INDEX]);
                    }
                    else
                    {
                        static int iIndex = 0;
                        static char ac[] = {'|','/','-','\\'};
                        printf("Tx: %c\r",ac[iIndex++]); fflush(stdout);
                        iIndex &= 3;
                    }
                }
                break;
            }

            case EVENT_RX_SEARCH_TIMEOUT: {
                qDebug() << "Search Timeout";
                break;
            }
            case TRANSFER_IN_ERROR : {
                qDebug() << "Transfer in Error";
                break;
            }
            case EVENT_RX_FAIL: {
                qDebug() << "Rx Fail";
                break;
            }
            case EVENT_TRANSFER_RX_FAILED: {
                qDebug() << "Burst receive has failed";
                break;
            }
            case EVENT_TRANSFER_TX_COMPLETED: {
                qDebug() << "HUB#" << stickNumber <<  "Transfer Was complete!  - EVENT_TRANSFER_TX_COMPLETED *********";
                if (stMessage.aucData[1] == BPS_PAGE_1) {
                    calibrationReponseReceived = true;
                    numberOfFailCalibration = 0;
                }
                break;
            }
            case EVENT_TRANSFER_TX_FAILED: {
                qDebug() << "HUB#" << stickNumber <<  "Transfer Failed! - EVENT_TRANSFER_TX_FAILED ****************";
                //check if it's a calibration message, resend
                //                if (!calibrationReponseReceived && stMessage.aucData[1] == BPS_PAGE_1) {
                //                    numberOfFailCalibration++;
                //                    qDebug() << "Retry Calibration!";
                //                    if (numberOfFailCalibration < 5)
                //                        sendCalibration(CALIBRATION_TYPE_MANUAL);
                //                }

                break;
            }
            case EVENT_RX_FAIL_GO_TO_SEARCH: {
                qDebug() << "Go to Search";
                break;
            }
            case EVENT_CHANNEL_COLLISION: {
                //                qDebug() << "Channel collison";
                break;
            }
            case EVENT_TRANSFER_TX_START: {
                qDebug() << "Burst Started\n";
                break;
            }
            default: {
                qDebug() << "Unhandled channel event:" <<  hex << stMessage.aucData[2];
                break;
            }
            }
            break;
        }

        default: {
            qDebug() << "Unhandled response" << stMessage.aucData[2] << " to message " << stMessage.aucData[1];
            break;
        }
        }
        break;
    }

    case MESG_STARTUP_MESG_ID: {
        qDebug() << "RESET Complete, reason: ";
        UCHAR ucReason = stMessage.aucData[MESSAGE_BUFFER_DATA1_INDEX];

        if(ucReason == RESET_POR)
            qDebug() << "RESET_POR";
        if(ucReason & RESET_SUSPEND)
            qDebug() << "RESET_SUSPEND";
        if(ucReason & RESET_SYNC)
            qDebug() << "RESET_SYNC";
        if(ucReason & RESET_CMD)
            qDebug() << "RESET_CMD";
        if(ucReason & RESET_WDT)
            qDebug() << "RESET_WDT";
        if(ucReason & RESET_RST)
            qDebug() << "RESET_RST";
        break;
    }

    case MESG_CAPABILITIES_ID: {
        printf("CAPABILITIES:\n");
        printf("   Max ANT Channels: %d\n",stMessage.aucData[MESSAGE_BUFFER_DATA1_INDEX]);
        printf("   Max ANT Networks: %d\n",stMessage.aucData[MESSAGE_BUFFER_DATA2_INDEX]);

        UCHAR ucStandardOptions = stMessage.aucData[MESSAGE_BUFFER_DATA3_INDEX];
        UCHAR ucAdvanced = stMessage.aucData[MESSAGE_BUFFER_DATA4_INDEX];
        UCHAR ucAdvanced2 = stMessage.aucData[MESSAGE_BUFFER_DATA5_INDEX];

        printf("Standard Options:\n");
        if( ucStandardOptions & CAPABILITIES_NO_RX_CHANNELS )
            printf("CAPABILITIES_NO_RX_CHANNELS\n");
        if( ucStandardOptions & CAPABILITIES_NO_TX_CHANNELS )
            printf("CAPABILITIES_NO_TX_CHANNELS\n");
        if( ucStandardOptions & CAPABILITIES_NO_RX_MESSAGES )
            printf("CAPABILITIES_NO_RX_MESSAGES\n");
        if( ucStandardOptions & CAPABILITIES_NO_TX_MESSAGES )
            printf("CAPABILITIES_NO_TX_MESSAGES\n");
        if( ucStandardOptions & CAPABILITIES_NO_ACKD_MESSAGES )
            printf("CAPABILITIES_NO_ACKD_MESSAGES\n");
        if( ucStandardOptions & CAPABILITIES_NO_BURST_TRANSFER )
            printf("CAPABILITIES_NO_BURST_TRANSFER\n");

        printf("Advanced Options:\n");
        if( ucAdvanced & CAPABILITIES_OVERUN_UNDERRUN )
            printf("CAPABILITIES_OVERUN_UNDERRUN\n");
        if( ucAdvanced & CAPABILITIES_NETWORK_ENABLED )
            printf("CAPABILITIES_NETWORK_ENABLED\n");
        if( ucAdvanced & CAPABILITIES_AP1_VERSION_2 )
            printf("CAPABILITIES_AP1_VERSION_2\n");
        if( ucAdvanced & CAPABILITIES_SERIAL_NUMBER_ENABLED )
            printf("CAPABILITIES_SERIAL_NUMBER_ENABLED\n");
        if( ucAdvanced & CAPABILITIES_PER_CHANNEL_TX_POWER_ENABLED )
            printf("CAPABILITIES_PER_CHANNEL_TX_POWER_ENABLED\n");
        if( ucAdvanced & CAPABILITIES_LOW_PRIORITY_SEARCH_ENABLED )
            printf("CAPABILITIES_LOW_PRIORITY_SEARCH_ENABLED\n");
        if( ucAdvanced & CAPABILITIES_SCRIPT_ENABLED )
            printf("CAPABILITIES_SCRIPT_ENABLED\n");
        if( ucAdvanced & CAPABILITIES_SEARCH_LIST_ENABLED )
            printf("CAPABILITIES_SEARCH_LIST_ENABLED\n");

        if(usSize_ > 4) {
            printf("Advanced 2 Options 1:\n");
            if( ucAdvanced2 & CAPABILITIES_LED_ENABLED )
                printf("CAPABILITIES_LED_ENABLED\n");
            if( ucAdvanced2 & CAPABILITIES_EXT_MESSAGE_ENABLED )
                printf("CAPABILITIES_EXT_MESSAGE_ENABLED\n");
            if( ucAdvanced2 & CAPABILITIES_SCAN_MODE_ENABLED )
                printf("CAPABILITIES_SCAN_MODE_ENABLED\n");
            if( ucAdvanced2 & CAPABILITIES_RESERVED )
                printf("CAPABILITIES_RESERVED\n");
            if( ucAdvanced2 & CAPABILITIES_PROX_SEARCH_ENABLED )
                printf("CAPABILITIES_PROX_SEARCH_ENABLED\n");
            if( ucAdvanced2 & CAPABILITIES_EXT_ASSIGN_ENABLED )
                printf("CAPABILITIES_EXT_ASSIGN_ENABLED\n");
            if( ucAdvanced2 & CAPABILITIES_FS_ANTFS_ENABLED)
                printf("CAPABILITIES_FREE_1\n");
            if( ucAdvanced2 & CAPABILITIES_FIT1_ENABLED )
                printf("CAPABILITIES_FIT1_ENABLED\n");
        }
        break;
    }
    case MESG_CHANNEL_STATUS_ID: {
        printf("Got Status\n");
        char astrStatus[][32] = {  "STATUS_UNASSIGNED_CHANNEL",
                                   "STATUS_ASSIGNED_CHANNEL",
                                   "STATUS_SEARCHING_CHANNEL",
                                   "STATUS_TRACKING_CHANNEL"   };

        UCHAR ucStatusByte = stMessage.aucData[MESSAGE_BUFFER_DATA2_INDEX] & STATUS_CHANNEL_STATE_MASK; // MUST MASK OFF THE RESERVED BITS
        printf("STATUS: %s\n",astrStatus[ucStatusByte]);
        break;
    }

    case MESG_CHANNEL_ID_ID: {
        // Channel ID of the device that we just recieved a message from.

        //        qDebug() << "MESG_CHANNEL_ID_ID";
        //        USHORT usDeviceNumber = stMessage.aucData[MESSAGE_BUFFER_DATA2_INDEX] | (stMessage.aucData[MESSAGE_BUFFER_DATA3_INDEX] << 8);
        //        UCHAR ucDeviceType =  stMessage.aucData[MESSAGE_BUFFER_DATA4_INDEX];
        //        UCHAR ucTransmissionType = stMessage.aucData[MESSAGE_BUFFER_DATA5_INDEX];

        //        qDebug() << "deviceNumber1:" << usDeviceNumber;
        //        qDebug() << "deviceType1:" << ucDeviceType;
        //        qDebug() << "transmitType1:" << ucTransmissionType;


        //        if (pairingMode) {
        //            qDebug() << "deviceNumber1:" << usDeviceNumber;
        //            qDebug() << "deviceType1:" << ucDeviceType;
        //            qDebug() << "transmitType1:" << ucTransmissionType;

        //            if (ucDeviceType << 120) { //hr

        //                if (usDeviceNumber != 0 && !lstDeviceHR.contains(usDeviceNumber)) {
        //                    lstDeviceHR.append(usDeviceNumber);
        //                    qDebug() << "Added Device HR to lst:" << usDeviceNumber;
        //                }

        //            }
        //        }
        break;
    }

    case MESG_VERSION_ID: {
        printf("VERSION: %s\n", (char*) &stMessage.aucData[MESSAGE_BUFFER_DATA1_INDEX]);
        break;
    }
    case MESG_ACKNOWLEDGED_DATA_ID:   // Handle all BROADCAST, ACKNOWLEDGED and BURST data the same
    case MESG_BURST_DATA_ID:
    case MESG_BROADCAST_DATA_ID: {
        // The flagged and unflagged data messages have the same
        // message ID. Therefore, we need to check the size to
        // verify of a flag is present at the end of a message.
        // To enable flagged messages, must call ANT_RxExtMesgsEnable first.
        if(usSize_ > MESG_DATA_SIZE) {
            UCHAR ucFlag = stMessage.aucData[MESSAGE_BUFFER_DATA10_INDEX];

            if(bDisplay && ucFlag & ANT_EXT_MESG_BITFIELD_DEVICE_ID) {
                // Channel ID of the device that we just recieved a message from.

                /*
                USHORT usDeviceNumber = stMessage.aucData[MESSAGE_BUFFER_DATA11_INDEX] | (stMessage.aucData[MESSAGE_BUFFER_DATA12_INDEX] << 8);
                UCHAR ucDeviceType =  stMessage.aucData[MESSAGE_BUFFER_DATA13_INDEX];
                UCHAR ucTransmissionType = stMessage.aucData[MESSAGE_BUFFER_DATA14_INDEX];
                */

                //                printf("Chan ID1(%d/%d/%d) - ", usDeviceNumber, ucDeviceType, ucTransmissionType);
                //                deb << "Chan ID(" << usDeviceNumber << "," << ucDeviceType << "," << ucTransmissionType << ")";
            }
        }

        // Display recieved message
        bPrintBuffer = TRUE;
        ucDataOffset = MESSAGE_BUFFER_DATA2_INDEX;   // For most data messages


        if(bDisplay) {
            if(stMessage.ucMessageID == MESG_ACKNOWLEDGED_DATA_ID )
                qDebug() << "Acked Rx: " << stMessage.aucData[MESSAGE_BUFFER_DATA1_INDEX];
            else if(stMessage.ucMessageID == MESG_BURST_DATA_ID)
                qDebug() << "Burst Rx: ";
            //                printf("Burst(0x%02x) Rx:(%d): ", ((stMessage.aucData[MESSAGE_BUFFER_DATA1_INDEX] & 0xE0) >> 5), stMessage.aucData[MESSAGE_BUFFER_DATA1_INDEX] & 0x1F );
            //            else
            //                deb << "Rx:(" << hex << stMessage.aucData[MESSAGE_BUFFER_DATA1_INDEX] << ")";

        }
        break;
    }
    case MESG_EXT_BROADCAST_DATA_ID: // Handle all BROADCAST, ACKNOWLEDGED and BURST data the same
    case MESG_EXT_ACKNOWLEDGED_DATA_ID:
    case MESG_EXT_BURST_DATA_ID: {


        // The "extended" part of this message is the 4-byte
        // id of the device that we recieved this message from. This message
        // is only available on the AT3. The AP2 uses flagged versions of the
        // data messages as shown above.

        //  ID of the device that we just recieved a message from.
        USHORT usDeviceNumber = stMessage.aucData[MESSAGE_BUFFER_DATA2_INDEX] | (stMessage.aucData[MESSAGE_BUFFER_DATA3_INDEX] << 8);
        UCHAR ucDeviceType =  stMessage.aucData[MESSAGE_BUFFER_DATA4_INDEX];
        UCHAR ucTransmissionType = stMessage.aucData[MESSAGE_BUFFER_DATA5_INDEX];

        bPrintBuffer = TRUE;
        ucDataOffset = MESSAGE_BUFFER_DATA6_INDEX;   // For most data messages


        if(bDisplay) {
            // Display the  id
            printf("Chan ID(%d/%d/%d) ", usDeviceNumber, ucDeviceType, ucTransmissionType );


            if(stMessage.ucMessageID == MESG_EXT_ACKNOWLEDGED_DATA_ID)
                printf("- Acked Rx:(%d): ", stMessage.aucData[MESSAGE_BUFFER_DATA1_INDEX]);
            else if(stMessage.ucMessageID == MESG_EXT_BURST_DATA_ID)
                printf("- Burst(0x%02x) Rx:(%d): ", ((stMessage.aucData[MESSAGE_BUFFER_DATA1_INDEX] & 0xE0) >> 5), stMessage.aucData[MESSAGE_BUFFER_DATA1_INDEX] & 0x1F );
            //            else
            //                deb << "Rx:" << hex << stMessage.aucData[MESSAGE_BUFFER_DATA1_INDEX] << " :";
        }
        break;
    }

    default:
    {
        break;
    }
    }

    // If we recieved a data message, we decode its content here and display it
    if(bPrintBuffer) {



        //        deb << "Data_Message: " << hex << stMessage.aucData[ucDataOffset + 0] << "- "
        //            << hex << stMessage.aucData[ucDataOffset + 1] << "- "
        //            << hex << stMessage.aucData[ucDataOffset + 2] << "- "
        //            << hex << stMessage.aucData[ucDataOffset + 3] << "- "
        //            << hex << stMessage.aucData[ucDataOffset + 4] << "- "
        //            << hex << stMessage.aucData[ucDataOffset + 5] << "- "
        //            << hex << stMessage.aucData[ucDataOffset + 6] << "- "
        //            << hex << stMessage.aucData[ucDataOffset + 7];



        USHORT usDeviceNumber = stMessage.aucData[MESSAGE_BUFFER_DATA11_INDEX] | (stMessage.aucData[MESSAGE_BUFFER_DATA12_INDEX] << 8);
        UCHAR ucDeviceType =  stMessage.aucData[MESSAGE_BUFFER_DATA13_INDEX];
        UCHAR ucTransmissionType = stMessage.aucData[MESSAGE_BUFFER_DATA14_INDEX];




        //        qDebug() << "PairingMode" << pairingMode << "Contains?" << hashSensorId.contains(usDeviceNumber);
        //        int userID = hashSensorId.value(usDeviceNumber);
        //qDebug() << "ANTID:" << stickNumber <<  "decodeMsg now?" << decodeMsgNow << "DeviceNumber:" << usDeviceNumber << " type:" << ucDeviceType << " transType:" << ucTransmissionType;

        // dont waste time processing sensor we dont need
        if (!pairingMode && !hashSensorId.contains(usDeviceNumber)) {
            //            qDebug() << "no waste time processing sensor we dont have here";
            return;
        }




        //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        /// HR
        if (ucDeviceType == constants::hrDeviceType) {

            if (pairingMode && deviceTypeToLookPairing == constants::hrDeviceType) {
                if (usDeviceNumber != 0 && !lstDeviceHr.contains(usDeviceNumber)) {
                    qDebug() << "Hub#" << stickNumber << "should add Sensor: " << usDeviceNumber << "deviceType:" << ucDeviceType << "transType:" << ucTransmissionType  <<" to list of HR";
                    lstDeviceHr.append(usDeviceNumber);
                    lstTypeDeviceHr.append(constants::hrDeviceType);
                    emit deviceFound();
                }
            }
            else if (decodeMsgNow) {
                //get UserID associated with sensorID
                int userID = hashSensorHr.value(usDeviceNumber);
                if (userID > 0) {
                    vecHrController.at(userID-1)->decodeHeartRateMessage(stMessage);
                }
            }
        }
        //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        /// CADENCE
        else if (ucDeviceType == constants::cadDeviceType) {

            if (pairingMode && deviceTypeToLookPairing == constants::cadDeviceType) {
                if (usDeviceNumber != 0 && !lstDeviceCADENCE_SpeedCadence.contains(usDeviceNumber)) {
                    qDebug() << "Hub#" << stickNumber << "should add Sensor: " << usDeviceNumber << "deviceType:" << ucDeviceType << "transType:" << ucTransmissionType  <<" to list of CADENCE";
                    lstDeviceCADENCE_SpeedCadence.append(usDeviceNumber);
                    lstTypeDeviceCADENCE_SpeedCadence.append(constants::cadDeviceType);
                    emit deviceFound();
                }
            }
            else if (decodeMsgNow) {
                //get UserID associated with sensorID
                int userID = hashSensorCad.value(usDeviceNumber);
                if (userID > 0) {
                    vecCadController.at(userID-1)->decodeCadenceMessage(stMessage);
                }
            }
        }
        //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        /// OXYGEN
        else if (ucDeviceType == constants::oxyDeviceType) {

            if (pairingMode && deviceTypeToLookPairing == constants::oxyDeviceType) {
                if (usDeviceNumber != 0 && !lstDeviceOXY.contains(usDeviceNumber)) {
                    qDebug() << "Hub#" << stickNumber <<  "should add Sensor: " << usDeviceNumber << "deviceType:" << ucDeviceType << "transType:" << ucTransmissionType  <<" to list of SPEED_CADENCE";
                    lstDeviceOXY.append(usDeviceNumber);
                    lstTypeDeviceOXY.append(constants::oxyDeviceType);
                    emit deviceFound();
                }
            }
            else if (decodeMsgNow) {
                //get UserID associated with sensorID
                int userID = hashSensorOxy.value(usDeviceNumber);
                if (userID > 0) {
                    vecOxyController.at(userID-1)->decodeOxygenMessage(stMessage);
                }
            }
        }


        //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        /// SPEED
        else if (ucDeviceType == constants::speedDeviceType) {

            if (pairingMode && deviceTypeToLookPairing == constants::speedDeviceType) {
                if (usDeviceNumber != 0 && !lstDeviceSPEED_SpeedCadence.contains(usDeviceNumber)) {
                    qDebug() << "Hub#" << stickNumber << "should add Sensor: " << usDeviceNumber << "deviceType:" << ucDeviceType << "transType:" << ucTransmissionType  <<" to list of SPEED";
                    lstDeviceSPEED_SpeedCadence.append(usDeviceNumber);
                    lstTypeDeviceSPEED_SpeedCadence.append(constants::speedDeviceType);
                    emit deviceFound();
                }
            }
            else if (decodeMsgNow) {
                int userID = hashSensorSpeed.value(usDeviceNumber);
                if (userID > 0) {
                    vecSpeedController.at(userID-1)->decodeSpeedMessage(stMessage);
                }
            }
        }

        //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        /// SPEED_CADENCE
        else if (ucDeviceType == constants::speedCadDeviceType) {

            if (pairingMode && (deviceTypeToLookPairing == constants::speedDeviceType || deviceTypeToLookPairing == constants::cadDeviceType)) {
                if (usDeviceNumber != 0 && !lstDeviceSPEED_SpeedCadence.contains(usDeviceNumber)) {
                    qDebug() << "Hub#" << stickNumber <<  "should add Sensor: " << usDeviceNumber << "deviceType:" << ucDeviceType << "transType:" << ucTransmissionType  <<" to list of SPEED_CADENCE";
                    lstDeviceSPEED_SpeedCadence.append(usDeviceNumber);
                    lstDeviceCADENCE_SpeedCadence.append(usDeviceNumber);
                    lstTypeDeviceSPEED_SpeedCadence.append(constants::speedCadDeviceType);
                    lstTypeDeviceCADENCE_SpeedCadence.append(constants::speedCadDeviceType);
                    emit deviceFound();
                }
            }
            else if (decodeMsgNow) {
                int userID = hashSensorSpeedCad.value(usDeviceNumber);
                if (userID > 0) {
                    vecSpeedCadController.at(userID-1)->decodeSpeedCadenceMessage(stMessage);
                }
            }
        }
        //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        /// FE-C
        else if (ucDeviceType == constants::fecDeviceType) {

            if (pairingMode && deviceTypeToLookPairing == constants::fecDeviceType) {
                if (usDeviceNumber != 0 && !lstDeviceFEC.contains(usDeviceNumber)) {
                    qDebug() << "Hub#" << stickNumber <<  "should add Sensor: " << usDeviceNumber << "deviceType:" << ucDeviceType << "transType:" << ucTransmissionType  <<" to list of SENSOR_FEC";
                    lstDeviceFEC.append(usDeviceNumber);
                    lstTypeDeviceFEC.append(constants::fecDeviceType);
                    emit deviceFound();
                }
            }
            else if (decodeMsgNow) {
                int userID = hashSensorFEC.value(usDeviceNumber);
                if (userID > 0) {
                    vecFecController.at(userID-1)->decodeFecMessage(stMessage);
                }

                // Add Sensor to List of sensor close (we received data = it's close:)
                if (!hashSensorFecNear.contains(usDeviceNumber)) {
                    hashSensorFecNear.insert(usDeviceNumber, userID);
                }
            }

        }
        //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        /// POWER
        else if (ucDeviceType == constants::powerDeviceType) {


            if (pairingMode && deviceTypeToLookPairing == constants::powerDeviceType) {
                if (usDeviceNumber != 0 && !lstDevicePOWER.contains(usDeviceNumber)) {
                    qDebug() << "Hub#" << stickNumber <<  "should add Sensor: " << usDeviceNumber << "deviceType:" << ucDeviceType << "transType:" << ucTransmissionType  <<" to list of Power";
                    lstDevicePOWER.append(usDeviceNumber);
                    lstTypeDevicePOWER.append(constants::powerDeviceType);
                    emit deviceFound();
                }
            }
            else if (decodeMsgNow) {
                int userID = hashSensorPower.value(usDeviceNumber);
                if (userID > 0) {
                    vecPowerController.at(userID-1)->decodePowerMessage(stMessage);

                    //  Check if it's a KICKR
                    //                    bool isKickr = ((stMessage.aucData[MESSAGE_BUFFER_DATA14_INDEX]&0xF0 ) == 0xA0);
                    //                    if (isKickr && !hashSensorKickr.contains(usDeviceNumber)) {
                    //                        qDebug() << "Hub#" << stickNumber <<  "WE found a kickr!" << usDeviceNumber;
                    //                        int userID = hashSensorPower.value(usDeviceNumber);
                    //                        hashSensorKickr.insert(usDeviceNumber, userID);
                    //                    }
                }
            }

        }
        else {
            qDebug() << "device not supported by app..." << ucDeviceType << "number:" << usDeviceNumber;
        }
    }

    return;
}

