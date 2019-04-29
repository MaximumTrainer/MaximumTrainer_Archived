#ifndef CADENCE_CONTROLLER_H
#define CADENCE_CONTROLLER_H

#include "CADENCE_pages.h"
#include "common_pages.h"
#include "types.h"
#include "dsi_framer_ant.hpp"

#include <QObject>


#define CBSC_PRECISION   ((ULONG)1000)


class Cadence_Controller : public QObject
{
    Q_OBJECT

public:
    Cadence_Controller(int userNb, QObject *parent = 0);

    void decodeCadenceMessage(ANT_MESSAGE stMessage);



///////////////////////////////////////////////////////////
signals:
    void cadenceChanged(int userID, int cadence);



private :
    int userNb;
    void computeCadence();

    BCPage0_Data currentPage0;
    BCPage0_Data pastPage0;
    BCPage1_Data stPage1Data;
    BCPage2_Data stPage2Data;
    BCPage3_Data stPage3Data;

    StatePage eThePageState;
    UCHAR ucOldPage;

    ULONG ulBCAccumCadence;
    RawValues stCadenceData;

    int dataNotChanged;
    bool firstMessageNow;

};



#endif // CADENCE_CONTROLLER_H



