#include "dialogcalibrate.h"
#include "ui_dialogcalibrate.h"

#include <QDebug>



DialogCalibrate::~DialogCalibrate()
{
    delete ui;
}



DialogCalibrate::DialogCalibrate(int antID, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::DialogCalibrate)
{
    ui->setupUi(this);
    this->antID = antID;

    resetFields(true);

    installEventFilter(this); //For Hotkeys

}


void DialogCalibrate::resetFields(bool resultAlso) {

    ui->label_pendingTxt->setText("");
    ui->label_tempCondition->setText("");
    ui->label_speedCondition->setText("");
    ui->label_currTemp->setText("");
    ui->label_targetSpeed->setText("");
    ui->label_targetSpinDownTime->setText("");

    if (resultAlso) {
        ui->label_calibrationResult->setText("");
        ui->label_calibrationTemp->setText("");
        ui->label_calibrationZeroOffset->setText("");
        ui->label_calibrationSpinDownTime->setText("");
    }
}





////////////////////////////////////////////////////////////////////////
void DialogCalibrate::on_pushButton_calibrate_clicked()
{

    resetFields(true);
    //Spin-Down
    if (ui->comboBox_calibrationType->currentIndex() == 0) {
        emit sendCalibrationFEC(antID, FEC_Controller::SPIN_DOWN);
    }
    //Offset
    else {
        emit sendCalibrationFEC(antID, FEC_Controller::ZERO_OFFSET);
    }

    ui->label_pendingTxt->setText(tr("Calibration in progress..."));
    ui->label_pendingTxt->fadeIn(500);

}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void DialogCalibrate::calibrationInProgress(bool pendingSpindown, bool pendingZeroOffset,
                                            FEC_Controller::TEMPERATURE_CONDITION tempCond, FEC_Controller::SPEED_CONDITION speedCond,
                                            double currTemperature, double targetSpeed, double targetSpinDownMs) {

    qDebug() << "DialogCalibrate::calibrationInProgress";

    QString pending = tr("Pending ");
    QString spinDownTxt   = tr("Spin-Down Calibration");
    QString zeroOffsetTxt = tr("Zero Offset Calibration");


    if (pendingSpindown && pendingZeroOffset) {
        ui->label_pendingTxt->setText(pending + spinDownTxt + tr(" and ") + zeroOffsetTxt);
        ui->label_pendingTxt->fadeIn(500);
    }
    else if (pendingSpindown) {
        ui->label_pendingTxt->setText(pending + spinDownTxt);
        ui->label_pendingTxt->fadeIn(500);
    }
    else if (pendingZeroOffset) {
        ui->label_pendingTxt->setText(pending + zeroOffsetTxt);
        ui->label_pendingTxt->fadeIn(500);
    }

    /// --- Temp Condition
    if (tempCond == FEC_Controller::TEMP_TOO_LOW) {
        ui->label_tempCondition->setText(tr("Temperature is currently too low"));
        ui->label_tempCondition->fadeIn(500);
    }
    else if (tempCond == FEC_Controller::TEMP_TOO_HIGH) {
        ui->label_tempCondition->setText(tr("Temperature is currently too high"));
        ui->label_tempCondition->fadeIn(500);
    }
    else if (tempCond == FEC_Controller::TEMP_OK) {
        ui->label_tempCondition->setText(tr("Temperature is OK!"));
        ui->label_tempCondition->fadeIn(500);
    }
    //Not applicable
    else {
        ui->label_tempCondition->setText("");
    }

    /// --- Speed Condition
    if (speedCond == FEC_Controller::SPEED_TOO_LOW) {
        ui->label_speedCondition->setText(tr("Speed is currently too low"));
        ui->label_speedCondition->fadeIn(500);
    }
    else if (speedCond == FEC_Controller::SPEED_OK) {
        ui->label_speedCondition->setText(tr("Speed is OK!"));
        ui->label_speedCondition->fadeIn(500);
    }
    //Not applicable
    else {
        ui->label_speedCondition->setText("");
    }

    //label_currTemp
    if (currTemperature != -100) {
        ui->label_currTemp->setText(tr("Current Temperature: ") + QString::number(currTemperature) + " °C");
        ui->label_currTemp->fadeIn(500);
    }
    if (targetSpeed != -1) {
        ui->label_targetSpeed->setText(tr("Target Speed is ") + QString::number(targetSpeed) + " KM/H");
        ui->label_targetSpeed->fadeIn(500);
    }
    if (targetSpinDownMs != -1) {
        ui->label_targetSpinDownTime->setText(tr("Target Spin-Down Time is ") + QString::number(targetSpinDownMs) + " ms");
        ui->label_targetSpinDownTime->fadeIn(500);
    }




}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void DialogCalibrate::calibrationOver(bool zeroOffsetSuccess, bool spinDownSuccess, double temperature, double zeroOffset, double spinDownTimeMs) {

    if (zeroOffsetSuccess || spinDownSuccess) {
        ui->label_calibrationResult->setText(tr("Calibration Successful!"));
        ui->label_calibrationResult->fadeIn(500);
    }
    else {
        ui->label_calibrationResult->setText(tr("Calibration Failed"));
        ui->label_calibrationResult->fadeIn(500);
    }

    if (temperature != -1) {
        ui->label_calibrationTemp->setText(tr("Temperature: ") + QString::number(temperature) + " °C");
        ui->label_calibrationTemp->fadeIn(500);
    }
    else {
        ui->label_calibrationTemp->setText("");
    }
    if (zeroOffset != -1) {
        ui->label_calibrationZeroOffset->setText(tr("Zero Offset: ") + QString::number(zeroOffset));
        ui->label_calibrationZeroOffset->fadeIn(500);
    }
    else {
        ui->label_calibrationZeroOffset->setText("");
    }
    if (spinDownTimeMs != -1) {
        ui->label_calibrationSpinDownTime->setText(tr("Spin-Down Time : ") + QString::number(spinDownTimeMs) + " ms");
        ui->label_calibrationSpinDownTime->fadeIn(500);
    }
    else {
        ui->label_calibrationSpinDownTime->setText("");
    }

    resetFields(false);
}







/////////////////////////////////////////////////////////////////////////////////
bool DialogCalibrate::eventFilter(QObject *watched, QEvent *event) {


    Q_UNUSED(watched);

    //    qDebug() << "EventFilter " << watched << "Event:" << event;

    if(event->type() == QEvent::KeyPress) {
        QKeyEvent *keyEvent = static_cast<QKeyEvent*>(event);

        if (keyEvent->key() == Qt::Key_F1) {
            this->close();
            return true;
        }
        else if (keyEvent->key() == Qt::Key_F2) {
            on_pushButton_calibrate_clicked();
            return true;
        }
    }
    return false;
}



