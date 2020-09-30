#ifndef COMBINEDSC_CONTROLLER_H
#define COMBINEDSC_CONTROLLER_H

#include "CBSC_pages.h"
#include "common_pages.h"
#include "types.h"
#include "dsi_framer_ant.hpp"
#include "account.h"

#include <QObject>


#define CBSC_PRECISION   ((ULONG)1000)



class CombinedSC_Controller : public QObject
{
    Q_OBJECT

public:
    CombinedSC_Controller(int userNb, PowerCurve powerCurve, int wheel_circ, QObject *parent = 0);

    void decodeSpeedCadenceMessage(ANT_MESSAGE stMessage);

    PowerCurve powerCurve;
    int wheelSize;

    bool ignoreSpeed;
    bool ignoreCadence;

    const ULONG MAX_HR = 300;
    const ULONG MAX_CAD = 300;
    const ULONG MAX_POWER = 2000;
    const ULONG MAX_SPEED = 200;



    ///////////////////////////////////////////////////////////
signals:
    void cadenceChanged(int userID, int cadence);
    void speedChanged(int userID, double speed);
    void powerChanged(int userID, int power);


private:
    int userNb;

    void computeCadence();
    void computeSpeed();
    void computeSpeedHelper();




    CBSCPage0_Data currentPage0;
    CBSCPage0_Data pastPage0;
    RawValues stSpeedData;
    RawValues stCadenceData;
    ULONG ulBSAccumRevs;
    ULONG ulBCAccumCadence;


    int speedDataNotChanged;
    int cadenceDataNotChanged;
    bool firstMessageSpeedNow;
    bool firstMessageCadenceNow;

};

#endif // COMBINEDSC_CONTROLLER_H
