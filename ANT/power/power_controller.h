#ifndef POWER_CONTROLLER_H
#define POWER_CONTROLLER_H

#include "BPS_pages.h"
#include "common_pages.h"
#include "dsi_framer_ant.hpp"

#include <QObject>
#include <QTimer>
#include <QString>

#include "antmsg.h"




// global pages define in common_pages.h file, PBS in pbs_pages.h




///------------------------------------------------------------------------------------------------------------------
class Power_Controller : public QObject
{
    Q_OBJECT



public:
    Power_Controller(int userNb, QObject *parent = 0);
    ~Power_Controller();

    void decodePowerMessage(ANT_MESSAGE stMessage);


    void newCalibrationStarted();

    bool alreadyShownBatteryWarning;
    bool isDoingAutoZero;

    bool usingForCadence;
    bool usingForSpeed;

    const ULONG MAX_HR = 300;
    const ULONG MAX_CAD = 300;
    const ULONG MAX_POWER = 2000;
    const ULONG MAX_SPEED = 200;



    /////////////////////////////////////////////////////////////////////////////
    /// STATIC ANT Configuration Block
    ////////////////////////////////////////////////////////////////////////////////

//    bool usingForCadence;
//    bool usingForSpeed;
//    bool usingForPower;
    double wheelSize;


    UCHAR  aucTxBuf[8];			  // Primary transmit buffer

    BPSPage1_Data pstPage1Data;		// calibration
    BPSPage16_Data pstPage16Data;     // std power only
    BPSPage17_Data pstPage17Data;      // torque at wheel
    BPSPage18_Data pstPage18Data;      // torque at crank
    BPSPage32_Data pstPage32Data;      // CTF
    CommonPage80_Data stPage80Data;	// manufacturer info
    CommonPage81_Data stPage81Data;	// product info
    CommonPage82_Data stPage82Data;	// optional battery info page


    CalculatedStdPower stCalculatedStdPowerData;			       // Standard Power (Page 16) calculations
    CalculatedStdWheelTorque stCalculatedStdWheelTorqueData;  // Wheel Torque (Page 17) calculations
    CalculatedStdCrankTorque stCalculatedStdCrankTorqueData;  // Crank Torque (Page 17) calculations
    CalculatedStdCTF stCalculatedStdCTFData;				       // CTF (Page 32) calculations


    int dataPower_BPS_PAGE_16_NotChanged;
    int dataCadence_BPS_PAGE_16_NotChanged;

    int dataCadence_BPS_PAGE_18_NotChanged;



    //Standard Power Only Variables
    BOOL   bPOFirstPageCheck;
    UCHAR  ucPOEventCountPrev;
    USHORT usPOAccumulatedPowerPrev;
    ULONG  ulPOEventCountDiff;
    UCHAR  ulPOEventSpeedCheck;
    ULONG  ulPOAccumulatedPowerDiff;

    //WHEEL TORQUE SENSOR VARIABLES
    BOOL   bWTFirstPageCheck;
    UCHAR  ucWTEventCountPrev;
    ULONG  ulWTEventCountDiff;
    USHORT usWTAccumulatedWheelPeriodPrev;
    ULONG  ulWTAccumulatedWheelPeriodDiff;
    USHORT usWTAccumulatedWheelTorquePrev;
    ULONG  ulWTAccumulatedWheelTorqueDiff;
    UCHAR  ucWTWheelTicksPrev;
    ULONG  ulWTWheelTicksDiff;
    UCHAR  ucWTZeroSpeedCheck;

    //CRANK TORQUE SENSOR VARIABLES
    BOOL   bCTFirstPageCheck;
    UCHAR  ucCTEventCountPrev;
    ULONG  ulCTEventCountDiff;
    USHORT usCTAccumulatedCrankPeriodPrev;
    ULONG  ulCTAccumulatedCrankPeriodDiff;
    USHORT usCTAccumulatedCrankTorquePrev;
    ULONG  ulCTAccumulatedCrankTorqueDiff;
    UCHAR  ucCTZeroSpeedCheck;

    // CRANK TORQUE FREQUENCY SENSOR VARIABLES
    BOOL   bCTFFirstPageCheck;
    UCHAR  ucCTFEventCountPrev;
    ULONG  ulCTFEventCountDiff;
    USHORT usCTFTimeStampPrev;
    ULONG  ulCTFTimeStampDiff;
    USHORT usCTFTorqueTickStampPrev;
    ULONG  ulCTFTorqueTickStampDiff;
    UCHAR  ucCTFZeroSpeedCheck;

    // Calibration Response Page variables
    QList<double> lstOffsetValue;
    double zeroOffsetMean;
    //    USHORT usCTFOffset;
    BOOL bCalibrationTimeout;
    UCHAR ucAckFailCount;
    CalibrationType eLocalCalibrationType;
    UCHAR ucAlarmNumber;



    // Page 1 variables/bit masks
    UCHAR ucLocalCTF_ID;
    UCHAR ucLocalAutoZeroStatus;
    UCHAR ucLocalSpeedFactor;

#define PAGE1_CID18_AutoZero_ENABLE(x)				  (x & 0x01)
#define PAGE1_CID18_AutoZero_STATUS(x)				  ((x >> 1) & 0x01)





    ////////////////////////////////////////////////////////////////////////////////////////////////
signals:
    void powerChanged(int userID, int power);
    void speedChanged(int userID, double speed);
    void cadenceChanged(int userID, int cadence);
    void rightPedalBalanceChanged(int userID, int balance);

    void pedalMetricChanged(int userID, double leftTorqueEff, double rightTorqueEff,
                            double leftPedalSmooth, double rightPedalSmooth, double combienedPedalSmooth);


    //-- Battery
    void batteryLow(QString sensorType, int level, int antID);


    //    //--- send back to Hub
    //    void sendCalibration(CalibrationType eCalibrationType);



    void supportAutoZero();
    void calibrationOverWithStatus(bool success, QString dataType, int data);

    void calibrationProgress(int timeStamp, double countdownPerc, double countDownTimeS,
                             double wholeTorque, double leftTorque, double rightTorque,
                             double wholeForce, double leftForce, double rightForce,
                             double zeroOffset, double temperatureC, double voltage);



public slots:
    //    bool calibrationRequest(CalibrationType eCalibrationType);
    //    void sendAutoZeroRequest();


private slots:



private:
    void saveOffset();
    void loadOffset();

    void offsetFinishedHere();



private :
    int userNb;


    //ANT_SPORT_CALIBRATION_MESSAGE  0x01
    ANTMsg currentMessage_page0x01;


    //    //ANT_STANDARD_POWER // 0x10 - standard power
    bool firstMessage0x10;
    ANTMsg lastMessage_page0x10;
    ANTMsg currentMessage_page0x10;
    int nullCount_page0x10;


    //    //ANT_WHEELTORQUE_POWER: // 0x11 - wheel torque (Powertap)
    bool firstMessage0x11;
    ANTMsg lastMessage_page0x11;
    ANTMsg currentMessage_page0x11;
    int nullCount_page0x11;


    //    //ANT_CRANKTORQUE_POWER // 0x12 - crank torque (Quarq)
    bool firstMessage0x12;
    ANTMsg lastMessage_page0x12;
    ANTMsg currentMessage_page0x12;
    int nullCount_page0x12;


    //    //ANT_CRANKSRM_POWER: // 0x20 - crank torque (SRM)
    bool firstMessage0x20;
    ANTMsg lastMessage_page0x20;
    ANTMsg currentMessage_page0x20;
    int nullCount_page0x20;


    // 0x03
    uint16_t lastTimeStamp0x03;
    int numberOfRollOver0x03;



    QString dataTypeOffset;
    QString dataType2;




};



#endif // POWER_CONTROLLER_H



