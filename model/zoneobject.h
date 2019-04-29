#ifndef ZONEOBJECT_H
#define ZONEOBJECT_H

#include <QObject>
#include "account.h"


class ZoneObject : public QObject
{
    Q_OBJECT

public:
    explicit ZoneObject(QObject *parent = 0);



public slots:
    void updateFTP(int value);
    void updateLTHR(int value);
    void updateUserWeight(double weight, bool isKg);
    void updateUserHeight(int height_cm);
    void updateCDA(double cda);

    void updateWheelSize(int size_mm);
    void updateBikeWeight(double weight_kg);
    void updateBiketype(int bikeType);

    void updateDisplayName(int playerId, QString name);


signals:
    void signal_updateFTP();
    void signal_updateLTHR();


private :
    Account *account;

};

#endif // ZONEOBJECT_H
