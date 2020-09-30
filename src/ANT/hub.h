#ifndef HUB_H
#define HUB_H


#include <QObject>
#include <QtCore>
#include <QList>

//#include "types.h"
#include "dsi_framer_ant.hpp"
#include "dsi_thread.h"
#include "dsi_serial_generic.hpp"
#include "combinedsc_controller.h"
#include "speed_controller.h"
#include "cadence_controller.h"
#include "power_controller.h"
#include "heartrate_controller.h"
#include "fec_controller.h"
#include "oxygen_controller.h"
#include "sensor.h"
#include "antmsg.h"
#include "antplus.h"
#include "kickrant.h"
#include "userstudio.h"


#define CHANNEL_TYPE_SLAVE		(1)


class Hub : public QObject
{
    Q_OBJECT

public:
    explicit Hub(int stickNumber, QObject *parent = 0);



    ///////////////////////////////////////////////////////////
signals:

    void closeCSMFinished();
    void askPermissionForSensor(int antID, int hubID);

    ///Signal received from childrens, fowarded to WorkoutDialog
    void signal_hr(int userID, int hr);
    void signal_speed(int userID, double speed);
    void signal_cadence(int userID, int cadence);
    void signal_power(int userID, int power);
    void signal_rightPedal(int userID, int rightPedal);
    void signal_oxygenValueChanged(int userID, double percentageSaturatedHemoglobin, double totalHemoglobinConcentration);  // %, g/d;
    void signal_lapChanged(int userID);

    void calibrationOver(bool,bool,double,double,double);
    void calibrationInProgress(bool pendingSpindown, bool pendingZeroOffset,
                               FEC_Controller::TEMPERATURE_CONDITION tempCond, FEC_Controller::SPEED_CONDITION speedCond,
                               double currTemperature, double targetSpeed, double targetSpinDownMs);
    void calibrationProgressPM(int timeStamp, double countdownPerc, double countDownTimeS,
                               double wholeTorque, double leftTorque, double rightTorque,
                               double wholeForce, double leftForce, double rightForce,
                               double zeroOffset, double temperatureC, double voltage);
    void pedalMetricChanged(int userID, double leftTorqueEff, double rightTorqueEff,
                            double leftPedalSmooth, double rightPedalSmooth, double combienedPedalSmooth);



    void signal_batteryLow(QString,int,int);

    void signal_powerCalibrationOverWithStatus(bool, QString, int);
    void signal_powerSupportAutoZero();



    void stickFound(bool found, QString serialString, int stickNumber);
    void deviceFound();
    void stopPairing();
    void sensorFound(int deviceType, int numberSensorFound, QList<int> lstSensor, QList<int> lstSensorType, bool forStudioMode);






public slots:

    void openScanningModeChannel(bool receiveOnly);
    void closeScanningModeChannel(bool closeShop);

    void addToControlListHub(int antID, int hubNumber);



    bool initUSBStick(int stickNumber);

    void setUserStudioData(QVector<UserStudio>);
    void setSoloDataToHub(PowerCurve curve, int wheelCircMM, QList<Sensor>, bool usePmForCadence, bool usePmForSpeed);
    void startSensors();
    void stopDecodingMsg();


    // ANT FE-C Commands
    void setLoad(int antID, double watts);
    void setSlope(int antID, double load);


    void sendOxygenCommand(int antID, Oxygen_Controller::COMMAND);
    void sendCalibrationFEC(int antID, FEC_Controller::CALIBRATION_TYPE);
    void sendCalibrationPM(int antID, bool manual);

    void startPairing(int deviceType, bool stopPairingWhenDeviceFound, int secToLook, bool fromStudioPage);

    //    void sendBroadcastData(int channel, unsigned char* data);





    //////////////////////////////////////////////////////////////////////////////
private:

    int stickNumber;


    //sensorID, userId
    QHash<int,int> hashSensorId;

    QHash<int,int> hashSensorHr;
    QHash<int,int> hashSensorCad;
    QHash<int,int> hashSensorSpeed;
    QHash<int,int> hashSensorSpeedCad;
    QHash<int,int> hashSensorOxy;
    QHash<int,int> hashSensorFEC;
    QHash<int,int> hashSensorPower;

    QHash<int,int> hashSensorFecNear;
    QHash<int,int> hashSendingChannel;  //ant id, channel


    bool firstCommandTrainer;


    // group
    bool checkToOpenChannelFEC(int antID);
    // solo
    void configureSendChannelFEC(int antID);
    void configureSendChannelPM(int antID);
    void configureSendChannelOxy(int antID);





    //Starts the Message thread.
    static DSI_THREAD_RETURN RunMessageThread(void *pvParameter_);

    //Listens for a response from the module
    void MessageThread();
    //Decodes the received message
    void ProcessMessage(ANT_MESSAGE stMessage, USHORT usSize_);
    void close();


    ///////////////////////////////////////////////////////////////////////////////
private:
    // vec[0] = controller for user number 0
    QVector<HeartRate_Controller*> vecHrController;
    QVector<Cadence_Controller*> vecCadController;
    QVector<Oxygen_Controller*> vecOxyController;
    // Theses controller have different settings depending per user
    QVector<Speed_Controller*> vecSpeedController;
    QVector<CombinedSC_Controller*> vecSpeedCadController;
    QVector<Power_Controller*> vecPowerController;
    QVector<FEC_Controller*> vecFecController;



    bool gotAntStick;




    QList<int> lstDeviceHr;
    QList<int> lstTypeDeviceHr;     //(0=HR, 1=Power, 2=Cadence, 3=Speed,  4=S&C, 5=FEC, 6=OXYGEN)

    QList<int> lstDevicePOWER;
    QList<int> lstTypeDevicePOWER;

    QList<int> lstDeviceFEC;
    QList<int> lstTypeDeviceFEC;

    QList<int> lstDeviceOXY;
    QList<int> lstTypeDeviceOXY;

    QList<int> lstDeviceCADENCE_SpeedCadence;
    QList<int> lstTypeDeviceCADENCE_SpeedCadence;

    QList<int> lstDeviceSPEED_SpeedCadence;
    QList<int> lstTypeDeviceSPEED_SpeedCadence;


    int currentLastChannel; //1 to 7 (0 reserved)
    int currentSendingChannel;

    bool decodeMsgNow;
    bool pairingMode;
    int deviceTypeToLookPairing;

    bool calibrationReponseReceived;
    int numberOfFailCalibration;



    BOOL bBroadcasting;
    BOOL bDisplay;

    DSISerialGeneric* pclSerialObject;
    DSIFramerANT* pclMessageObject;
    DSI_THREAD_ID uiDSIThread;

    DSI_CONDITION_VAR condTestDone;
    DSI_MUTEX mutexTestDone;


    UCHAR aucTransmitBuffer[ANT_STANDARD_DATA_PAYLOAD_SIZE];


    ///////////////////////////////USB STICK INFO /////////////////////////////////
    USHORT usDevicePID;
    USHORT usDeviceVID;
    UCHAR aucDeviceDescription[USB_MAX_STRLEN];
    UCHAR aucDeviceSerial[USB_MAX_STRLEN];

    UCHAR aucDeviceSerialString[USB_MAX_STRLEN];
    /////////////////////////////////////////////////////////////////////////////////


    //test CSM
    //    UCHAR ucAntAckedMesg;
    //    UCHAR ucAck;


};
Q_DECLARE_METATYPE(Hub*)

#endif // HUB_H
