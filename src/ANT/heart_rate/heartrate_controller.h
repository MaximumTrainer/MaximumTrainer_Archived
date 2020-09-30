#ifndef HEARTRATE_CONTROLLER_H
#define HEARTRATE_CONTROLLER_H


#include "HRM_pages.h"
#include "common_pages.h"
#include "types.h"
#include "dsi_framer_ant.hpp"
#include <QObject>





class HeartRate_Controller : public QObject
{
    Q_OBJECT

public:
    HeartRate_Controller(int userNb, QObject *parent = 0);
    ~HeartRate_Controller();

    void decodeHeartRateMessage(ANT_MESSAGE stMessage);


    const ULONG MAX_HR = 300;

///////////////////////////////////////////////////////////
signals:
    void HeartRateChanged(int userID, int hr);

    //-- Battery
    void batteryLow(QString sensorType, int level, int antID);


private :
    int userNb;

    HRMPage0_Data stPage0Data;
    HRMPage1_Data stPage1Data;
    HRMPage2_Data stPage2Data;
    HRMPage3_Data stPage3Data;
    HRMPage4_Data stPage4Data;

    StatePage eThePageState;
    UCHAR ucOldPage;

    int dataNotChanged;
    int previousHeartBeatCount;
    int previousHeartBeatTime;
    bool alreadyShownBatteryWarning;

};


#endif // HEARTRATE_CONTROLLER_H




