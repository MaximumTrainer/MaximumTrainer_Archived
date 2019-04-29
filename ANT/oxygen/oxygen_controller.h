#ifndef OXYGEN_CONTROLLER_H
#define OXYGEN_CONTROLLER_H


#include "types.h"
#include "dsi_framer_ant.hpp"
#include <stdint.h>
#include <QObject>



class Oxygen_Controller : public QObject
{
    Q_OBJECT
public:

    enum COMMAND
    {
        SET_TIME,       //0x00 – Set Time
        START_SESSION,  //0x01 – Start Session
        STOP_SESSION,   //0x02 – Stop Session
        LAP,            //0x03 – Lap
    };


    explicit Oxygen_Controller(int userNb, QObject *parent = 0);
    ~Oxygen_Controller();

    void decodeOxygenMessage(ANT_MESSAGE stMessage);





signals:
    void oxygenValueChanged(int userID, double percentageSaturatedHemoglobin, double totalHemoglobinConcentration);  // %, g/d;
    void batteryLow(QString sensorType, int level, int antID);





private :
    int userNb;

    bool alreadyShownBatteryWarning;
    uint8_t lastEventCount0x01;

};

#endif // OXYGEN_CONTROLLER_H
