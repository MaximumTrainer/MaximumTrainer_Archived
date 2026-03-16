// Test-only stub for util.h
// Shadows the real util.h (which has QWT deps) within the test project.
// Only declares the symbols that are actually referenced by workout.cpp / interval.cpp.
#ifndef UTIL_H
#define UTIL_H

#include <QtCore>

class Util
{
public:
    static double convertQTimeToSecD(const QTime &time)
    {
        return time.hour() * 3600.0 + time.minute() * 60.0 + time.second()
               + time.msec() / 1000.0;
    }

    static QString getSystemPathWorkout()
    {
        return QDir::temp().absoluteFilePath(QStringLiteral("mt_test_workouts"));
    }
};

#endif // UTIL_H
