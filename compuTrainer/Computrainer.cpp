#include "computrainer.h"


#include "util.h"
#include "racermate.h"

#include <QDebug>





CompuTrainer::CompuTrainer()
{
    bFoundCT = false;
    bCtStarted = false;
    isPaused = false;
    isCalibrating = false;


    firmwareVersion = -1;
    calibrated = false;

    lastkeys = 0;


    loadPreviousPort();
    init();
}



//---------------------------------------------------------------------------------------------------------
void CompuTrainer::init() {


    qDebug() << "CT INIT!";

    QString pathCT = Util::getSystemPathCompuTrainer();  //C:/User/Documents/CompuTrainerLogs

    std::string windows_text = pathCT.toLocal8Bit().constData();

    qDebug() << "CT: Enable CT logging...";
    if (pathCT != "invalid_writable_path") {

        int val = Setlogfilepath(windows_text.c_str());
        if (val != 0)
            qDebug() << "Error setLogFilePath" << val;

        bool logEnabled = false;
        val = Enablelogs(logEnabled,logEnabled,logEnabled,logEnabled,logEnabled,logEnabled);
        if (val != 0)
            qDebug() << "Error Enable Logs" << val;
    }





}


//////////////////////////////////////////////////////////////////////////////////////////////////
void CompuTrainer::quickSearch() {

    qDebug() << "QuickSearch CT?";
    if (comPort != 0) {
        search();
    }

}


//////////////////////////////////////////////////////////////////////////////////////////////////
void CompuTrainer::search() {


    qDebug() << "Search";
    emit debugMesg("Starting Search...");


    /// ----- Quick search
    if (comPort != 0) {
        emit debugMesg("Looking for CT on last known port:" + QString::number(comPort) + "...");
        EnumDeviceType deviceType1 = GetRacerMateDeviceID(comPort);
        if (deviceType1 == DEVICE_COMPUTRAINER)  {
            emit debugMesg("Found CT:" + QString::number(comPort) + "...");
            bFoundCT = true;
        }
    }

    /// ----- Deep search
    if (!bFoundCT) {
        int portToCheck = 0;
        while (portToCheck <= 250) {
            portToCheck++;
            emit debugMesg("Checking comPort" + QString::number(portToCheck) + "...");
            qDebug() << "checking port.." << portToCheck;
            EnumDeviceType deviceType1 = GetRacerMateDeviceID(portToCheck);
            if (deviceType1 == DEVICE_COMPUTRAINER)  {
                emit debugMesg("Found CT:" + QString::number(portToCheck));
                bFoundCT = true;
                comPort = portToCheck;
                //Save to settings for next time
                savePort(comPort);
                emit debugMesg("Save ComPort:" + QString::number(comPort) + " for later...");
                break;
            }
//                        if (portToCheck == 9)  {
//                            emit debugMesg("Found CT:" + QString::number(portToCheck));
//                            bFoundCT = true;
//                            comPort = portToCheck;
//                            //Save to settings for next time
//                            savePort(comPort);
//                            emit debugMesg("Save ComPort:" + QString::number(comPort) + " for later...");
//                            break;
//                        }
        }
    }





    /// ----- Get current info
    if (bFoundCT) {
        firmwareVersion = GetFirmWareVersion(comPort);
        emit debugMesg("CT FirmWare version is:" + QString::number(firmwareVersion) + "...");


        /// -------- Get Calibration";
        int returnVal = GetIsCalibrated(comPort,firmwareVersion);
        if (returnVal == 0)  {
            emit debugMesg("CT not yet calibrated since power on...");
            calibrated = false;
        }
        else {
            emit debugMesg("CT was calibrated since power on...");
            calibrated = true;
        }
        RRC = GetCalibration(comPort);
    }



    emit ctFound(bFoundCT, comPort, firmwareVersion, calibrated);
}




//////////////////////////////////////////////////////////////////////////////////////////////////
void CompuTrainer::start() {

    qDebug() << "start CT";



    if (bFoundCT) {

        int status;
        if (!bCtStarted) {
            /// -------- Start Trainer
            status = startTrainer(comPort);
            if (status != ALL_OK) {
                emit debugMesg("Failure to startTrainer...");
            }
            else {
                bCtStarted = true;
                emit debugMesg("Success startTrainer...");
            }
            QThread::msleep(500);
        }



        /// -------- Reset Trainer
        status = resetTrainer(comPort, firmwareVersion, RRC);
        if (status != ALL_OK)  {
            emit debugMesg("Failure to resetTrainer...");
        }
        else {
            emit debugMesg("Success resetTrainer...");
        }


        /// -------- Reset Averages
        status =  ResetAverages(comPort, firmwareVersion);
        if (status != ALL_OK)  {
            emit debugMesg("Failure to ResetAverages...");
        }
        else {
            emit debugMesg("Success ResetAverages...");
        }


        // setPause off
        resume();




        timerUpdateData = new QTimer(this);
        timeCheckKeyPressed = new QTimer(this);

        connect(timerUpdateData, SIGNAL(timeout()), this, SLOT(updateData()) );
        connect(timeCheckKeyPressed, SIGNAL(timeout()), this, SLOT(checkKeyPress()) );

        timerUpdateData->start(180);
        timeCheckKeyPressed->start(350);
    }


}


//////////////////////////////////////////////////////////////////////////////////////////////////
void CompuTrainer::updateData() {


    TrainerData trainerData = GetTrainerData(comPort, firmwareVersion);
    float watts = trainerData.Power;
    float kph = trainerData.kph;
    float rpm = trainerData.cadence;
    float hr = trainerData.HR;

    //    SSDATA ssdata = get_ss_data(comPort, firmwareVersion);
    //    float rightPercent = ssdata.rsplit;  // right leg watts percentage

    emit dataUpdated(watts,kph,rpm,hr,-1);


    //if power is -1 and speed -1, something is wrong, redo search and restart device?
}



//--------------------------------------------
void CompuTrainer::checkKeyPress() {

    //    qDebug() << "checkKeyPress..";


    int keys = GetHandleBarButtons(comPort, firmwareVersion);

    if (keys == lastkeys)
        return;

    switch(keys)  {
    case 0x00:
        break;

        // not connected
    case 0x40:
        break;

        // reset
    case 0x01:
        emit debugMesg("Reset Button Pressed...");
        break;

        // f2 - Switches modes between Slope (gradient) and ERG (resistance changes to match target power)
    case 0x04:
        emit debugMesg("F2 Button Pressed...");
        break;

        // +
    case 0x10:
        emit debugMesg("+ Button Pressed...");
        break;

        // f1 - Start or pause the workout
    case 0x02:
        emit debugMesg("F1 Button Pressed...");
        if (isPaused) {
            emit debugMesg("F1-Resume...");
            resume();
        }
        else {
            emit debugMesg("F1-Pause...");
            pause();
        }

        break;

        // f3 - Enter or exit calibration mode.
    case 0x08:
        emit debugMesg("F3 Button Pressed...");
        if (isCalibrating) {
            emit debugMesg("F3-EndCalibrate...");
            endCalibrate();
        }
        else {
            emit debugMesg("F3-StartCalibrate...");
            calibrate();
        }

        break;

        // -
    case 0x20:
        break;
    default:
        break;
    }


    lastkeys = keys;

}




//////////////////////////////////////////////////////////////////////////////////////////////////
void CompuTrainer::resume() {

    isPaused = false;

    int status = setPause(comPort, false);
    if (status != ALL_OK)  {
        emit debugMesg("Failure to setPause to Off...");
    }
    else {
        emit debugMesg("Success setPause to Off...");
    }
    QThread::msleep(500);

    emit pauseClicked(false);
}
//----------------------------------------------------
void CompuTrainer::pause() {

    isPaused = true;

    int status = setPause(comPort, true);
    if (status != ALL_OK)  {
        emit debugMesg("Failure to setPause to On...");
    }
    else {
        emit debugMesg("Success setPause to On...");
    }
    QThread::msleep(500);

    emit pauseClicked(true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////
void CompuTrainer::calibrate() {

    if (!isPaused)
        pause();

    isCalibrating = true;
    int status = SetRecalibrationMode(comPort, firmwareVersion);
    if (status != ALL_OK)  {
        emit debugMesg("Failure to SetRecalibrationMode...");
    }
    else {
        emit debugMesg("Success SetRecalibrationMode...");
    }


    emit calibrationStarted(true, -1);
}

//--------------------------------------------------------------
void CompuTrainer::endCalibrate() {

    isCalibrating = false;
    RRC = EndRecalibrationMode(comPort, firmwareVersion);


    emit calibrationStarted(false, RRC);
}



//////////////////////////////////////////////////////////////////////////////////////////////////
void CompuTrainer::stop_resetIDLE() {

    qDebug() << "stop_resetIDLE!" << bFoundCT << bCtStarted ;


    if (!bFoundCT || !bCtStarted)
        return;

    timerUpdateData->stop();
    timeCheckKeyPressed->stop();

    emit debugMesg("Success Close Workout, stop data collection from CT...");


    //    int status = stopTrainer(comPort);
    //    if (status != ALL_OK)  {
    //        bCtStarted = false;
    //        qDebug() << "Error stopTrainer...";
    //    }
    //    else {
    //        qDebug() << "Success StopTrainer...";
    //    }


    //    status = ResettoIdle(comPort);
    //    if (status != ALL_OK)  {
    //        qDebug() << "Error ResettoIdle...";
    //    }
    //    else {
    //        qDebug() << "Success ResettoIdle...";
    //    }

}




//////////////////////////////////////////////////////////////////////////////////////////////////
void CompuTrainer::setLoad(double watts) {


    //    qDebug() << "SETLOAD!"<< watts;

    int status = SetErgModeLoad(comPort, firmwareVersion, RRC, watts);
    if (status!=ALL_OK)  {
        emit debugMesg("Failure to set load to " + QString::number(watts) + "...");
    }
    else {
        emit debugMesg("Success set load to " + QString::number(watts) + "...");
    }
}



//////////////////////////////////////////////////////////////////////////////////////////////////
///int SetSlope(int Comport, int FirmwareVersion, int RRC,float bike_kgs, float rider_kgs int DragFactor, float slope)
///
void CompuTrainer::setSlopeAt(double slope) {


    qDebug() << "SetSlopeCT:" << slope;


    int status = SetSlope(comPort, firmwareVersion, RRC, 10, 70, 100, slope);
    if (status!=ALL_OK)  {
        emit debugMesg("Failure to set Slope to " + QString::number(slope) + "...");
    }
    else {
        emit debugMesg("Success set Slope to " + QString::number(slope) + "...");
    }


}















//////////////////////////////////////////////////////////////////////////////////////////////////
void CompuTrainer::loadPreviousPort() {

    QSettings settings;

    settings.beginGroup("CompuTrainer");
    comPort = settings.value("comPort", 0 ).toInt();
    settings.endGroup();


    qDebug() << "load previous Port.." << comPort;


}
//------------------------------------------------------
void CompuTrainer::savePort(int comPort) {

    QSettings settings;

    settings.beginGroup("CompuTrainer");
    settings.setValue("comPort", comPort);
    settings.endGroup();

    qDebug() << "save previous Port.." << comPort;
}

