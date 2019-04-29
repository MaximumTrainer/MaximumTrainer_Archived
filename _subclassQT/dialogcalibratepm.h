#ifndef DIALOGCALIBRATEPM_H
#define DIALOGCALIBRATEPM_H

#include <QDialog>
#include <QKeyEvent>
#include <QEvent>

namespace Ui {
class DialogCalibratePM;
}

class DialogCalibratePM : public QDialog
{
    Q_OBJECT

public:
    explicit DialogCalibratePM(int antID, QWidget *parent = 0);
    ~DialogCalibratePM();

    bool eventFilter(QObject *watched, QEvent *event);


signals:
    void startCalibration(int antID, bool manual); //manual = false means AutoZero




public slots:
    void calibrationInfoReceived(bool,QString,int);
    void supportAutoZero();
    void calibrationProgress(int timeStamp, double countdownPerc, double countDownTimeS,
                             double wholeTorque, double leftTorque, double rightTorque,
                             double wholeForce, double leftForce, double rightForce,
                             double zeroOffset, double temperatureC, double voltage);




private slots:
    void on_pushButton_calibrateManual_clicked();
    void on_pushButton_calibrateAutoZero_clicked();

private :
    void resetFields();

private:
    Ui::DialogCalibratePM *ui;

    int antID;
};

#endif // DIALOGCALIBRATEPM_H
