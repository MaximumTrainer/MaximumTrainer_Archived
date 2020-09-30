#include "planobject.h"

#include <QDebug>


PlanObject::PlanObject(QObject *parent) :
    QObject(parent)
{
}





/////////////////////////////////////////////////////////////////////////////
void PlanObject::goToWorkout(QString name) {

    qDebug() << "Go ToWorkout!" << name;

    emit signal_goToWorkout(name);

}
