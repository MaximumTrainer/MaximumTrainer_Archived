#ifndef COMPUTRAINERCONTROLLER_H
#define COMPUTRAINERCONTROLLER_H


#include <QObject>
#include <QThread>

#include "computrainer.h"




class CompuTrainerController : public QObject
{
    Q_OBJECT

public:
    explicit CompuTrainerController(QObject *parent = 0);
    ~CompuTrainerController();

    bool hasCT() {
        return this->gotCT;
    }
    bool isCalibratedf() {
        return this->isCalibrated;
    }




public slots:
    void ctFound(bool found, int comPort, int firmware, bool calibrated);
    void start();
    void stop_resetIDLE();

    void setLoad(int antID, double watts);
    void setSlope(int antID, double slope);

    void send_signal_search();
    void send_signal_quicksearch();





signals:
    //Sent to CT
    void signal_quick_search();
    void signal_search();
    void signal_start();
    void signal_stop_resetIDLE();
    void signal_setLoad(double watts);
    void signal_setSlope(double slope);




    //from CT
    void signal_debugMesg(QString msg);
    void signal_ctFound(bool found, int comPort, int firmware, bool calibrated);
    void signal_dataUpdated(float power, float speedKPH, float cad, float hr, float rightPedalPercent);

    void signal_pauseClicked(bool pause);
    void signal_calibrationStarted(bool started, int value);









private :
    QThread *thread;
    CompuTrainer *compuTrainer;


    bool gotCT;
    bool isCalibrated;


};
Q_DECLARE_METATYPE(CompuTrainerController*)

#endif // COMPUTRAINERCONTROLLER_H
