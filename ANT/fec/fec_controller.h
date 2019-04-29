#ifndef FEC_CONTROLLER_H
#define FEC_CONTROLLER_H



#include "types.h"
#include "dsi_framer_ant.hpp"
#include <stdint.h>
#include <QObject>




class FEC_Controller : public QObject
{
    Q_OBJECT


public:

    enum CALIBRATION_TYPE
    {
        ZERO_OFFSET,
        SPIN_DOWN,
    };

    enum TEMPERATURE_CONDITION
    {
        TEMP_NOT_APPLICABLE,
        TEMP_TOO_LOW,
        TEMP_OK,
        TEMP_TOO_HIGH,
    };
    enum SPEED_CONDITION
    {
        SPEED_NOT_APPLICABLE,
        SPEED_TOO_LOW,
        SPEED_OK,
        RESERVED,
    };


    explicit FEC_Controller(int userNb, QObject *parent = 0);
    ~FEC_Controller();

    void decodeFecMessage(ANT_MESSAGE stMessage);


    double wheelSizeMeters;

    const ULONG MAX_HR = 300;
    const ULONG MAX_CAD = 300;
    const ULONG MAX_POWER = 2000;
    const ULONG MAX_SPEED = 200;


    //Data Page 48 (0x30) – Basic Resistance
    static void EncodeTrainerBasicResistance(uint8_t* pucBuffer, uint8_t percentage);
    //Data Page 49 (0x31) – Target Power
    static void EncodeTrainerTargetPower(uint8_t* pucBuffer, uint16_t watts);

    //Data Page 50 (0x32) – Wind Resistance
    static void EncodeTrainerWindResistance(uint8_t* pucBuffer, uint8_t windResistCoef, uint8_t windSpeed, uint8_t draftingFactor);
    //Data Page 51 (0x33) – Track Resistance
    static void EncodeTrainerTrackResistancePercentage(uint8_t* pucBuffer, double percSlope);




public slots:



signals:
    void speedChanged(int userID, double speedKMH);
    void hrChanged(int userID, int hr);
    void powerChanged(int userID, int power);
    void cadenceChanged(int userID, int cadence);
    void lapChanged(int userID);
    void batteryLow(QString sensorType, int level, int antID);

    void calibrationInProgress(bool pendingSpindown, bool pendingZeroOffset,
                               FEC_Controller::TEMPERATURE_CONDITION tempCond, FEC_Controller::SPEED_CONDITION speedCond,
                               double currTemperature, double targetSpeed, double targetSpinDownMs);
    void calibrationOver(bool zeroOffsetSuccess, bool spinDownSuccess, double temperature, double zeroOffset, double spinDownTimeMs);


public slots:


private :
    void checkFEStateBitField(ANT_MESSAGE stMessage);

private :
    int userNb;

    bool alreadyShownBatteryWarning;
    int numberLap; //used by ant verification tool
    bool lapToogleSet;
    bool lapToogleValue;


    bool firstValueAccumulatedPower;
    uint8_t lastEventCount0x19;
    uint16_t lastAccumulatedPower;
    uint64_t accumulatedEventCount;
    uint64_t totalAccumulatedPower;


    bool firstPage0x1A;
    uint8_t lastEventCount0x1A;
    uint8_t lastWheelTick;
    uint16_t lastWheelPeriod;
    uint16_t lastAccumulatedTorque;
    int noVelocityOccured;
};

#endif // FEC_CONTROLLER_H
