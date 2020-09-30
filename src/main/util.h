#ifndef UTIL_H
#define UTIL_H

#include <QtCore>
#include <QApplication>

#include <qwt_plot.h>
#include "qwt_plot_grid.h"
#include "qwt_plot_histogram.h"
#include "workout.h"
#include "interval.h"
#include "sensor.h"
#include "radio.h"
#include "achievement.h"


class Util
{
public:
    enum Color
    {
        LINE_POWER,
        SQUARE_POWER,

        LINE_CADENCE,
        SQUARE_CADENCE,

        LINE_HEARTRATE,
        SQUARE_HEARTRATE,

        LINE_SPEED,

        TOO_LOW,
        TOO_HIGH,
        ON_TARGET,
        NOT_DONE,
        DONE,

        BALANCE_POWER_TXT,
        LINE_ON_TARGET_GRAPH,
    };



    Util();



    static double convertQTimeToSecD(const QTime &time);
    static QTime convertMinutesToQTime(double minutes);

    static QString cleanQString(QString toClean);
    static QString cleanForOsSaving(QString toClean);


    static QString showCurrentTimeAsString(const QTime &time);
    static QString showQTimeAsString(const QTime &time);
    static QString showQTimeAsStringWithMs(const QTime &time);
    static QString getStringFromUCHAR(unsigned char* ch);
    static QColor getColor(Color color);


    //// Folders
    static bool checkFolderPathIsValidForWrite(QString path);
    static QString getMaximumTrainerDocumentPath();
    static QString getSystemPathWorkout();
    static QString getSystemPathCourse();
    static QString getSystemPathHistory();



    static QString getSystemPathHelperReturnDefaultLoc(QString docType); //workout, history, course



    /// --------


    static QStringList getListFiles(QString fileType); //.workout or //.course
    static void openWorkoutFolder(QString workoutPath);
    static void openCourseFolder(QString workoutPath);
    static void openHistoryFolder();


    static bool checkFileNameAlreadyExist(QString pathFile);
    static void deleteLocalFile(QString fileName);


    // Zip, Unzip
    static void zipFileToDisk(QString filename, QString zipFilename, bool useGzip);
    static void unzipFile(QString zipFilename , QString filename);
    // used for Gzip convert
    static QByteArray zipFileHelperConvertToGzip(const QByteArray& data);
    static quint32 crc32buf(const QByteArray& data);
    static quint32 updateCRC32(unsigned char ch, quint32 crc);





    /// Parse JSON
    static void parseJsonObjectAccount(QString data);
    static double parseJsonObjectVersion(QString data);
    static void parseJsonStravaObject(QString data);
    static int parseIdJsonStravaUploadObject(QString data);
    static int parseStravaUploadStatus(QString data);

    static void parseJsonTPObject(QString data);

    static QList<Sensor> parseJsonSensorList(QString data);
    static QList<Radio> parseJsonRadioList(QString data);
    static QList<Achievement> parseJsonAchievementList(QString data);
    static QSet<int> parseJsonAchievementListForUser(QString data);






private :


};

#endif // UTIL_H
