#ifndef SPEED_CONTROLLER_H
#define SPEED_CONTROLLER_H


#include "SPEED_pages.h"
#include "common_pages.h"
#include "types.h"
#include "dsi_framer_ant.hpp"
#include "account.h"

#include <QObject>


#define CBSC_PRECISION   ((ULONG)1000)



class Speed_Controller : public QObject
{
    Q_OBJECT

public:
    Speed_Controller(int userNb, PowerCurve powerCurve, int wheel_circ, QObject *parent = 0);

    void decodeSpeedMessage(ANT_MESSAGE stMessage);


    PowerCurve powerCurve;
    int wheelSize;


    const ULONG MAX_HR = 300;
    const ULONG MAX_CAD = 300;
    const ULONG MAX_POWER = 2000;
    const ULONG MAX_SPEED = 200;




///////////////////////////////////////////////////////////
signals:
    void speedChanged(int userID, double speed);
    void powerChanged(int userID, int power);


private:
    int userNb;


    void computeSpeed();
    void computeSpeedHelper();


    BSPage0_Data currentPage0;
    BSPage0_Data pastPage0;
    BSPage1_Data stPage1Data;
    BSPage2_Data stPage2Data;
    BSPage3_Data stPage3Data;


    StatePage eThePageState;
    UCHAR ucOldPage;
    int dataNotChanged;

    ULONG ulBSAccumRevs;
    RawValues stSpeedData;


    bool firstMessageNow;

};

#endif // SPEED_CONTROLLER_H
