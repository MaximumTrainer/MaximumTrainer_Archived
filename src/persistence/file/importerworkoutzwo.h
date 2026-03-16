#ifndef IMPORTERWORKOUTZWO_H
#define IMPORTERWORKOUTZWO_H

#include <QObject>
#include <QXmlStreamReader>
#include "workout.h"
#include "interval.h"

class ImporterWorkoutZwo : public QObject
{
    Q_OBJECT
public:
    explicit ImporterWorkoutZwo(QObject *parent = 0);
    ~ImporterWorkoutZwo();

    static Workout importFromFile(QString filename);

private:
    static QList<Interval> parseWorkoutElement(QXmlStreamReader &xml);
};

#endif // IMPORTERWORKOUTZWO_H
