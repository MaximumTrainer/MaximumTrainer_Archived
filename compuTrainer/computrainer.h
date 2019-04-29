#ifndef COMPUTRAINER_H
#define COMPUTRAINER_H

#include <QObject>
#include <QTimer>

class CompuTrainer : public QObject
{
    Q_OBJECT


public:
    explicit CompuTrainer();





signals:
    void ctFound(bool found, int comPort, int firmware, bool calibrated);
    void debugMesg(QString msg);
    void dataUpdated(float power, float speedKPH, float cad, float hr, float rightPedalPercent);

    void pauseClicked(bool pause);
    void calibrationStarted(bool started, int value);




public slots:
    void quickSearch();
    void search();
    void start();
    void stop_resetIDLE();

    void setLoad(double watts);
    void setSlopeAt(double slope);






    ///private slots
    void updateData();
    void checkKeyPress();





private :
    void init();

    void resume();
    void pause();

    void calibrate();
    void endCalibrate();

    void loadPreviousPort();
    void savePort(int comPort);




private :
    bool bFoundCT;
    bool bCtStarted;
    bool isPaused;
    bool isCalibrating;


    //Comport is an integer 0-255 corresponding to Com1, Com2, Com3
    int comPort;
    int firmwareVersion;
    bool calibrated;
    int RRC;

    int lastkeys;


    QTimer *timerUpdateData;
    QTimer *timeCheckKeyPressed;





};

#endif // COMPUTRAINER_H
