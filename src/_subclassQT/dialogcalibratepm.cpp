#include "dialogcalibratepm.h"
#include "ui_dialogcalibratepm.h"

#include <QDebug>



DialogCalibratePM::~DialogCalibratePM()
{
    delete ui;
}



DialogCalibratePM::DialogCalibratePM(int antID, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::DialogCalibratePM)
{
    ui->setupUi(this);
    this->antID = antID;

    ui->pushButton_calibrateAutoZero->setHidden(true);
    resetFields();

    installEventFilter(this); //For Hotkeys
}




///////////////////////////////////////////////////////////////////////////////////////
void DialogCalibratePM::supportAutoZero() {

    qDebug() << "DialogCalibratePM::supportAutoZero";
    ui->pushButton_calibrateAutoZero->setHidden(false);



}



///////////////////////////////////////////////////////////////////////////////////////
void DialogCalibratePM::resetFields() {

    //Result
    ui->label_calibrationResult->setText("");
    ui->label_calibrationData->setText("");


    //Progress
    ui->label_calibrationInProgress->setText("");

    ui->progressBar->setValue(0);
    ui->progressBar->setVisible(false);

    ui->label_calibrationTimeRemaining->setText("");
    ui->label_timeStamp->setText("");

    ui->groupBox_Force->setVisible(false);
    ui->label_leftForce->setText("");
    ui->label_rightForce->setText("");
    ui->label_wholeForce->setText("");

    ui->groupBox_Torque->setVisible(false);
    ui->label_leftTorque->setText("");
    ui->label_rightTorque->setText("");
    ui->label_wholeTorque->setText("");

    ui->label_calibrationVoltage->setText("");
    ui->label_calibrationTemperature->setText("");
    ui->label_calibrationZeroOffset->setText("");


}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void DialogCalibratePM::calibrationProgress(int timeStamp, double countdownPerc, double countDownTimeS,
                                            double wholeTorque, double leftTorque, double rightTorque,
                                            double wholeForce, double leftForce, double rightForce,
                                            double zeroOffset, double temperatureC, double voltage) {


    //    qDebug() << "DialogCalibratePM::calibrationProgress" << "timeStamp" << timeStamp << "countdownPerc" << countdownPerc << "countDownTimeS" << countDownTimeS <<
    //                "wholeTorque" << wholeTorque << "leftTorque" << leftTorque << "rightTorque" << rightTorque <<
    //                "wholeForce" << wholeForce << "leftForce" << leftForce << "rightForce" << rightForce <<
    //                "zeroOffset" << zeroOffset << "temperatureC" << temperatureC <<  "voltage" << voltage;


    ui->label_timeStamp->setText(tr("Message Received: ") + QString::number(timeStamp));
    ui->label_timeStamp->fadeIn(500);


    if (countdownPerc != -1) {
        ui->progressBar->setVisible(true);
        ui->progressBar->setValue(100-countdownPerc);
    }
    else if(countDownTimeS != -1) {
        ui->label_calibrationTimeRemaining->setText(QString::number(countDownTimeS) + tr(" sec. remaining until completion."));
        ui->label_calibrationTimeRemaining->fadeIn(500);
    }
    else if(wholeTorque != -1) {
        ui->groupBox_Torque->setVisible(true);
        ui->label_wholeTorque->setText(tr("Whole: ") + QString::number(wholeTorque));
        ui->label_wholeTorque->fadeIn(500);
    }
    else if(leftTorque != -1) {
        ui->groupBox_Torque->setVisible(true);
        ui->label_leftTorque->setText(tr("Left: ") + QString::number(leftTorque));
        ui->label_leftTorque->fadeIn(500);
    }
    else if(rightTorque != -1) {
        ui->groupBox_Torque->setVisible(true);
        ui->label_rightTorque->setText(tr("Right: ") + QString::number(rightTorque));
        ui->label_rightTorque->fadeIn(500);
    }
    else if(wholeForce != -1) {
        ui->groupBox_Force->setVisible(true);
        ui->label_wholeForce->setText(tr("Whole: ") + QString::number(wholeForce));
        ui->label_wholeForce->fadeIn(500);
    }
    else if(leftForce != -1) {
        ui->groupBox_Force->setVisible(true);
        ui->label_leftForce->setText(tr("Left: ") + QString::number(leftForce));
        ui->label_leftForce->fadeIn(500);
    }
    else if(rightForce != -1) {
        ui->groupBox_Force->setVisible(true);
        ui->label_rightForce->setText(tr("Right: ") + QString::number(rightForce));
        ui->label_rightForce->fadeIn(500);
    }
    else if(zeroOffset != -1) {
        ui->label_calibrationZeroOffset->setText(tr("Zero Offset: ") + QString::number(zeroOffset));
        ui->label_calibrationZeroOffset->fadeIn(500);
    }
    else if(temperatureC != -1) {
        ui->label_calibrationTemperature->setText(tr("Temperature (°C): ") + QString::number(temperatureC));
        ui->label_calibrationTemperature->fadeIn(500);
    }
    else if(voltage != -1) {
        ui->label_calibrationVoltage->setText(tr("Voltage (V): ") + QString::number(voltage));
        ui->label_calibrationVoltage->fadeIn(500);
    }





}




//////////////////////////////////////////////////////////////////////////////////////////////
void DialogCalibratePM::calibrationInfoReceived(bool success, QString dataType, int value) {


    ui->label_calibrationInProgress->fadeOut(500);

    if (success) {
        ui->label_calibrationResult->setText(tr("Calibration Successful!"));
        ui->label_calibrationResult->fadeIn(500);
    }
    else {
        ui->label_calibrationResult->setText(tr("Calibration Failed"));
        ui->label_calibrationResult->fadeIn(500);
    }

    ui->label_calibrationData->setText(dataType + "=" + QString::number(value));
    ui->label_calibrationData->fadeIn(500);

}

void DialogCalibratePM::on_pushButton_calibrateManual_clicked()
{
    resetFields();
    emit startCalibration(antID, true);
    ui->label_calibrationInProgress->setText(tr("Calibration in progress..."));
    ui->label_calibrationInProgress->fadeIn(500);
}

void DialogCalibratePM::on_pushButton_calibrateAutoZero_clicked()
{
    resetFields();
    emit startCalibration(antID, false);
}


/////////////////////////////////////////////////////////////////////////////////
bool DialogCalibratePM::eventFilter(QObject *watched, QEvent *event) {


    Q_UNUSED(watched);

    //    qDebug() << "EventFilter " << watched << "Event:" << event;

    if(event->type() == QEvent::KeyPress) {
        QKeyEvent *keyEvent = static_cast<QKeyEvent*>(event);

        if (keyEvent->key() == Qt::Key_F1) {
            this->close();
            return true;
        }
        else if (keyEvent->key() == Qt::Key_F2) {
            on_pushButton_calibrateManual_clicked();
            return true;
        }
    }
    return false;
}



