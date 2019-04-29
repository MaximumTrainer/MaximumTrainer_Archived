#ifndef DIALOGCALIBRATE_H
#define DIALOGCALIBRATE_H

#include <QDialog>
#include <QKeyEvent>
#include <QEvent>
#include "fec_controller.h"

namespace Ui {
class DialogCalibrate;
}

class DialogCalibrate : public QDialog
{
    Q_OBJECT

public:
    explicit DialogCalibrate(int andID, QWidget *parent = 0);
    ~DialogCalibrate();

    bool eventFilter(QObject *watched, QEvent *event);


public slots:
    void calibrationInProgress(bool pendingSpindown, bool pendingZeroOffset,
                               FEC_Controller::TEMPERATURE_CONDITION tempCond, FEC_Controller::SPEED_CONDITION speedCond,
                               double currTemperature, double targetSpeed, double targetSpinDownMs);
    void calibrationOver(bool zeroOffsetSuccess, bool spinDownSuccess, double temperature, double zeroOffset, double spinDownTimeMs);

signals:
    void sendCalibrationFEC(int antID, FEC_Controller::CALIBRATION_TYPE);





private slots:
    void on_pushButton_calibrate_clicked();

private :
    void resetFields(bool resultAlso);

private:
    Ui::DialogCalibrate *ui;

    int antID;
};

#endif // DIALOGCALIBRATE_H
