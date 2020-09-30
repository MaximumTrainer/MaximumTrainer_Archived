#include "oxygen_controller.h"

#include "antplus.h"

#include <QDebug>
#include <QDateTime>
#include <QDate>
#include <QTime>





#define MUSCLE_OXYGEN_PAGE1         0x01



Oxygen_Controller::~Oxygen_Controller(){
}



Oxygen_Controller::Oxygen_Controller(int userNb, QObject *parent) : QObject(parent){

    this->userNb = userNb;
    //Init variables
    alreadyShownBatteryWarning = false;
    lastEventCount0x01 = 0;
}








////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void Oxygen_Controller::decodeOxygenMessage(ANT_MESSAGE stMessage) {



    //qDebug() << "Oxygen_Controller: [" << hex << "[" << hex << stMessage.aucData[1] << "]" << "[" << hex << stMessage.aucData[2] << "]" << "[" << hex << stMessage.aucData[3] << "]" << "[" << hex << stMessage.aucData[4] << "]" << "[" << hex << stMessage.aucData[5] << "]" << "[" << hex << stMessage.aucData[6] << "]" << "[" << hex << stMessage.aucData[7]  << "]" << "[" << hex << stMessage.aucData[8] << "]";



    UCHAR data_page = stMessage.aucData[1];

    switch (data_page) {

    //////////////////////////////////////////////////////////////////////////////////////////////////////////////
    case MUSCLE_OXYGEN_PAGE1:
    {
        //        qDebug() << "MUSCLE_OXYGEN_PAGE1";


        uint8_t eventCount0x01 = stMessage.aucData[2];
        uint16_t diffEventCount = 0;


        if (lastEventCount0x01 > eventCount0x01) {
            diffEventCount = 256-lastEventCount0x01 + eventCount0x01;
        }
        else {
            diffEventCount = eventCount0x01 - lastEventCount0x01;
        }



        if (diffEventCount > 0) {
            //Decode message here

            // --- UTC Time Required
//            bool utcTimeRequired  = ((stMessage.aucData[3] >> 0)  & 0x01);
//            if (utcTimeRequired)
//                sendCommand(COMMAND::SET_TIME);

            // ----- Capabilities
            // 1 (0x01) Measurement interval = 0.25s
            // 2 (0x02) Measurement interval = 0.50s
            // 3 (0x03) Measurement interval = 1s
            // 4 (0x04) Measurement interval = 2s
            int capabilities = 0;
            bool capBitOne   = ((stMessage.aucData[4] >> 1)  & 0x01);
            bool capBitTwo   = ((stMessage.aucData[4] >> 2)  & 0x01);
            bool capBitThree = ((stMessage.aucData[4] >> 3)  & 0x01);

            if (capBitThree)
                capabilities += 4;
            if (capBitTwo)
                capabilities += 2;
            if (capBitOne)
                capabilities += 1;



            //--- Total Hemoglobin Concentration
            uint16_t total4bits = 0;
            bool hemoBitZero  = ((stMessage.aucData[6] >> 0)  & 0x01);   //2^8
            bool hemoBitOne   = ((stMessage.aucData[6] >> 1)  & 0x01);   //2^9
            bool hemoBitTwo   = ((stMessage.aucData[6] >> 2)  & 0x01);   //2^10
            bool hemoBitThree = ((stMessage.aucData[6] >> 3)  & 0x01);   //2^11

            if (hemoBitZero)
                total4bits += 256;
            if (hemoBitOne)
                total4bits += 512;
            if (hemoBitTwo)
                total4bits += 1024;
            if (hemoBitThree)
                total4bits += 2048;

            //Ambient light too high= 0x3FE
            //Invalid= 0xFFF
            uint32_t totalHemoglobinConc = stMessage.aucData[5] + total4bits;
            double totalHemoglobinConcentration =  totalHemoglobinConc *0.01;
            //            qDebug() << "stMessage.aucData[5]" << stMessage.aucData[5] << "total4bits" << total4bits << "TOTAL" << totalHemoglobinConc;


            //--- Current Saturated Hemoglobin Percentage
            uint16_t firstPart = 0;
            uint16_t secondPart =  (stMessage.aucData[8]<< 2);

            bool satBitZero     = ((stMessage.aucData[7] >> 6)  & 0x01);   //2^0
            bool satBitOne      = ((stMessage.aucData[7] >> 7)  & 0x01);   //2^1

            if (satBitZero)
                firstPart += 1;
            if (satBitOne)
                firstPart += 2;


            //Ambient light too high= 0x3FE
            //Invalid= 0x3FF
            uint32_t currentSaturatedHemo = firstPart + secondPart;
            double currentSaturatedHemoPerc = qRound(currentSaturatedHemo *0.1);
            //            qDebug() << "firstPart" << firstPart << "secondPart" << secondPart << "TOTAL" << currentSaturatedHemo;


            emit oxygenValueChanged(userNb, currentSaturatedHemoPerc, totalHemoglobinConcentration);
            //            qDebug() << "totalHemoglobinConcentration" << totalHemoglobinConcentration << "currentSaturatedHemoglobinePercentage" << currentSaturatedHemoPerc;

        }


        lastEventCount0x01 = eventCount0x01;
        break;
    }
    case GLOBAL_PAGE_82:
    {

        USHORT usDeviceNumber = stMessage.aucData[10] | (stMessage.aucData[11] << 8);
        uint8_t batteryStatus = COMMON82_BATT_STATUS(stMessage.aucData[8]);

        //---------------------------------
        switch (batteryStatus)
        {
        case GBL82_BATT_STATUS_NEW:
        {
            break;
        }
        case GBL82_BATT_STATUS_GOOD:
        {
            break;
        }
        case GBL82_BATT_STATUS_OK:
        {
            break;
        }
        case GBL82_BATT_STATUS_LOW:
        {
            if (!alreadyShownBatteryWarning) {
                emit batteryLow(tr("Muscle Oxygen"), 1, usDeviceNumber);
                alreadyShownBatteryWarning = true;
            }
            break;
        }
        case GBL82_BATT_STATUS_CRITICAL:
        {
            if (!alreadyShownBatteryWarning) {
                emit batteryLow(tr("Muscle Oxygen"), 0, usDeviceNumber);
                alreadyShownBatteryWarning = true;
            }
            break;
        }
        default:
        {
            qDebug() << "battery invalid status";
            break;
        }
        }
        //--------------------

    }
    default:
    {
        //        qDebug() << "decodeOxygenMessage Default break";
        break;
    }


    }







    //    emit hemoglobinChanged(90.0);
}
