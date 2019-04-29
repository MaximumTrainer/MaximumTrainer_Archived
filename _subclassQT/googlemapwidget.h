#ifndef GOOGLEMAPWIDGET_H
#define GOOGLEMAPWIDGET_H

#include <QWidget>
#include "trackpoint.h"
#include "course.h"


namespace Ui {
class GoogleMapWidget;
}

class GoogleMapWidget : public QWidget
{
    Q_OBJECT

public:
    explicit GoogleMapWidget(QWidget *parent = 0);
    ~GoogleMapWidget();

    void paintEvent(QPaintEvent *);

    bool loadCourseTrigger();


signals:

//    void trackChanged(QByteArray byteArrayJson);
    void trackChanged(QVariantList);
    void courseOverwrited(Course);
    void courseCreated(Course);
    void showStatusBarMessage(QString msg, int timeDisplay);


public slots:
    void elevationDataReceived(QVariantList);
    void locationStrReceived(QString);

private slots:
    void addJSObject_GooglePage();



    void on_pushButton_save_clicked();


private:

    void populateQWebView(QString name, QString location, double distance, double elevation, QString description);
    QString loadPathImportCourse();
    void savePathImportCourse(QString filepath);



private:
    Ui::GoogleMapWidget *ui;

    QString filenameLoaded;
    QString location;
    QString description;

    double distanceKm;
    double elevationM;
    double minElevation;
    double maxElevation;




    QList<Trackpoint> lstTrkptCondensed;
};

#endif // GOOGLEMAPWIDGET_H
