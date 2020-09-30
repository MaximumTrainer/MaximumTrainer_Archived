#include "googlemapwidget.h"
#include "ui_googlemapwidget.h"

#include <QFileDialog>
#include <QDebug>
#include <QWebFrame>
#include <QMessageBox>
#include <QWebElement>

#include "gpxparser.h"
#include "environnement.h"
#include "util.h"
#include "xmlutil.h"


GoogleMapWidget::~GoogleMapWidget()
{
    delete ui;
}


GoogleMapWidget::GoogleMapWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::GoogleMapWidget)
{
    ui->setupUi(this);

    ui->pushButton_save->setEnabled(false);



    ui->webView_courseInfo->page()->mainFrame()->setScrollBarPolicy(Qt::Horizontal, Qt::ScrollBarAlwaysOff);
    ui->webView_courseInfo->page()->mainFrame()->setScrollBarPolicy(Qt::Vertical, Qt::ScrollBarAlwaysOff);

    ///// TEST QWEBVIEW -----------------------------------------
    QNetworkAccessManager *nam = qApp->property("NetworkManager").value<QNetworkAccessManager*>();
    ui->webView_courseInfo->page()->setNetworkAccessManager(nam);

    // Signal is emitted before frame loads any web content:
    //        connect(ui->webView_courseInfo->page()->mainFrame(), SIGNAL(javaScriptWindowObjectCleared()),
    //                this, SLOT(addJSObject_WorkoutCreatorPage()));


    ui->webView_courseInfo->setUrl(QUrl(Environnement::getUrlCourseCreator()));
    /// ----------------------------------------------------





    //    ui->webView_zones->page()->setNetworkAccessManager(nam);
    connect(ui->webView->page()->mainFrame(), SIGNAL(javaScriptWindowObjectCleared()),
            this, SLOT(addJSObject_GooglePage()));


    ui->webView->setUrl(Environnement::getURLGoogleMap());

}


////////////////////////////////////////////////////////////////////////////////////////////////////
bool GoogleMapWidget::loadCourseTrigger()
{

    QString file = QFileDialog::getOpenFileName(this, tr("Import Course"),
                                                loadPathImportCourse(),
                                                tr("Course Files(*.gpx *.tcx *.kml)"));



    if (file.isEmpty())
        return false;

    savePathImportCourse(file);
    ui->pushButton_save->setEnabled(false);

    location = "";
    description = "";
    distanceKm = 0;
    elevationM = 0;
    populateQWebView("",location, distanceKm, elevationM, description);

    QFileInfo fileInfo(file);
    filenameLoaded =  fileInfo.baseName();


    //    QList<Trackpoint> lstTrack;
    GpxParser gpxParser;
    lstTrkptCondensed = gpxParser.parseFile(file, 100);
    if (lstTrkptCondensed.size() < 4) {
        qDebug() << "problem parsing file inside gpxParser...leave";
        return false;
    }

    //---------------------
    QJsonArray trackpoints;

    qDebug() << "after parser, lstTrkpt" << lstTrkptCondensed.size();


    foreach(Trackpoint tp, lstTrkptCondensed) {

        QJsonObject obj;
        obj["lon"] = tp.getLon();
        obj["lat"] = tp.getLat();
        obj["distance_here"] = tp.getDistanceAtThisPoint();
        //        obj["elevation"] = tp.getElevation();   //No need for elevation data for now, use google map elevation

        trackpoints.append(obj);
    }
    QVariantList variantLst = trackpoints.toVariantList();
    emit trackChanged(variantLst);
    qDebug() << "emit track changed!";

    //save Distance
    distanceKm = lstTrkptCondensed.last().getDistanceAtThisPoint()/1000;

    return true;
}


/////////////////////////////////////////////////////////////////////////////////////////////////////////
void GoogleMapWidget::on_pushButton_save_clicked()
{
    qDebug() << "save Course to File!";


    // ---- Set workout attributes -----
    //    workout.setName(name);
    //    workout.setPlan(plan);
    //    workout.setCreator(creator);
    //    workout.setDescription(description);
    //    Workout::Type typeWorkout = static_cast<Workout::Type>(type_workout);
    //    workout.setType(typeWorkout);



    Course course("", Course::USER_MADE, filenameLoaded, location, description, lstTrkptCondensed);




    QString pathCourse;
    if (course.getFilePath() == "" || (course.getFilePath().length() > 1 && course.getFilePath().at(0) == ':') ) {  //also check if it's an included workout (resource)
        pathCourse = Util::getSystemPathCourse() + "/" + course.getName() + ".course";
        course.setFilePath(pathCourse);
    }
    else {
        QFileInfo fileInfo(course.getFilePath());
        pathCourse = fileInfo.absolutePath() + "/" + course.getName() + ".course";
        course.setFilePath(pathCourse);
    }

    qDebug() << "course filePath is:" << course.getFilePath();

    if (Util::getSystemPathCourse() == "invalid_writable_path") {
        return;
    }

    // ---- Save workout to disk -----
    bool fileCreated = false;

    qDebug() << "SHOULD SAVE AS ***" << pathCourse;
    bool exist = Util::checkFileNameAlreadyExist(course.getFilePath());

    // ask yes no for overwrite existing file
    if (exist)
    {
        QMessageBox msgBox(this);
        msgBox.setIcon(QMessageBox::Question);
        msgBox.setText(tr("A course already exists with that name."));
        msgBox.setInformativeText(tr("Overwrite it?"));
        msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
        msgBox.setDefaultButton(QMessageBox::No);
        if (msgBox.exec() == QMessageBox::Yes) {
            qDebug() << "Yes was clicked";
            /// Delete local file xml
            Util::deleteLocalFile(course.getFilePath());

            fileCreated = XmlUtil::createCourseXml(course, "");
            qDebug() << "Emit courseOverwrited";
            emit courseOverwrited(course);
        }
    }
    else {
        fileCreated = XmlUtil::createCourseXml(course, "");
        qDebug() << "Emit courseCreated";
        emit courseCreated(course);
    }

    if (fileCreated) {

        QString textToShow;
        if (!exist)
            textToShow = tr("Course  \"%1\" created").arg(course.getFilePath());
        else
            textToShow = tr("Course  \"%1\" overwrited").arg(course.getFilePath());
        emit showStatusBarMessage(textToShow, 7000);
    }
}



/////////////////////////////////////////////////////////////////////////////////////////////////////////
void GoogleMapWidget::addJSObject_GooglePage() {

    qDebug() << "**** addJSObject_GooglePage";

    ui->webView->page()->mainFrame()->addToJavaScriptWindowObject(QString("googleMapWidget"), this);

}




////////////////////////////////////////////////////////////////////////////////////////////////////
void GoogleMapWidget::locationStrReceived(QString loc) {

    qDebug() << "locationStrReceived" << loc;

    this->location = loc;

    populateQWebView(filenameLoaded, location, distanceKm, maxElevation-minElevation, "");

}


////////////////////////////////////////////////////////////////////////////////////////////////////
void GoogleMapWidget::elevationDataReceived(QVariantList lstElevation) {

    qDebug() << "elevationDataReceived" << lstElevation.size();

    //    Trackpoint tp0 = lstTrkpt.at(0);
    //    qDebug() << "first data is:"  << lstElevation.at(0).toDouble(); //give mm precision, good enough
    //    qDebug() << "first coords are: lat:" << tp0.getLat() << " lon:" << tp0.getLon() << " elevation:" << tp0.getElevation();


    // ---- Add Good elevation data to each Trackpoint
    for(int i=0; i<lstElevation.size(); i++) {
        if (i >= lstTrkptCondensed.size())
            return;
        Trackpoint TpToEdit = lstTrkptCondensed.at(i);
        TpToEdit.setElevation(lstElevation.at(i).toDouble());
        lstTrkptCondensed.replace(i, TpToEdit);
    }



    // ---- Calculate Slope and add to each trackpoint(Delta Elevation between Trackpoint)
    Trackpoint tpFirst;
    Trackpoint tpSecond;
    minElevation = 100000000;
    maxElevation = -100000000;

    for(int i=0; i<lstTrkptCondensed.size()-1; i++) {

        tpFirst = lstTrkptCondensed.at(i);
        tpSecond = lstTrkptCondensed.at(i+1);
        double distanceBetween = tpSecond.getDistanceAtThisPoint() - tpFirst.getDistanceAtThisPoint();
        double deltaH = tpSecond.getElevation() - tpFirst.getElevation();


        double slopePercentage = deltaH/distanceBetween*100;
        tpFirst.setSlopePercentage(slopePercentage);
        lstTrkptCondensed.replace(i, tpFirst);
        qDebug() << "deltaH:" << deltaH << " distanceBetween:" << distanceBetween <<  "slopeP:" << slopePercentage << "distanceNow" << tpSecond.getDistanceAtThisPoint();

        //update Min, Max elevation
        if (tpFirst.getElevation() < minElevation) {
            minElevation = tpFirst.getElevation();
        }
        if (tpFirst.getElevation() > maxElevation) {
            maxElevation = tpFirst.getElevation();
        }
    }



    ui->pushButton_save->setEnabled(true);


}







//////////////////////////////////////////////////////////////////////////////////////////////////////////
void GoogleMapWidget::populateQWebView(QString name, QString location, double distance, double elevation, QString description) {


    //Parse for Quotes, Javascript doesnt like them so escape them
    QString locationEscaped = location.replace("\'", "\\'");
    QString descriptionEscaped = description.replace("\'", "\\'");


    /// Name
//    QWebElement input1 = ui->webView_courseInfo->page()->mainFrame()->documentElement().findFirst("input[id=\"name-course\"]");
//    QString jsValue = QString("this.value='%1';").arg(name);
//    input1.evaluateJavaScript(jsValue);


//    /// Location
//    QWebElement input2 = ui->webView_courseInfo->page()->mainFrame()->documentElement().findFirst("input[id=\"location-course\"]");
//    jsValue = QString("this.value='%1';").arg(locationEscaped);
//    input2.evaluateJavaScript(jsValue);


//    /// Distance
//    //Format to (km,mmm)
//    QString formatedDistance = QString::number(distance, 'f', 3);
//    formatedDistance+=  " km";
//    QWebElement input3 = ui->webView_courseInfo->page()->mainFrame()->documentElement().findFirst("input[id=\"distance-course\"]");
//    //    inputCreatorElement.setAttribute("value", creator);
//    jsValue = QString("this.value='%1';").arg(formatedDistance);
//    input3.evaluateJavaScript(jsValue);

//    /// Elevation Diff
//    //Format to (km,mmm)
//    QString formatedElevation = QString::number(elevation, 'f', 1);
//    formatedElevation+=  " m";
//    QWebElement input4 = ui->webView_courseInfo->page()->mainFrame()->documentElement().findFirst("input[id=\"elevation-course\"]");
//    //    textAreaDescriptionElement.setPlainText(description);
//    jsValue = QString("this.value='%1';").arg(formatedElevation);
//    input4.evaluateJavaScript(jsValue);

//    /// Description
//    QWebElement textAreaDescriptionElement = ui->webView_courseInfo->page()->mainFrame()->documentElement().findFirst("textarea[id=\"description-course\"]");
//    //    textAreaDescriptionElement.setPlainText(description);
//    jsValue = QString("this.value='%1';").arg(descriptionEscaped);
//    textAreaDescriptionElement.evaluateJavaScript(jsValue);


    //TODO: use eval javascript instead of above code
    //// ----------- Set Data in QWebView : have to put null at the end of evaluate javascript or it's really slow
    /// source : http://stackoverflow.com/questions/19505063/qt-javascript-execution-slow-unless-i-log-to-the-console
//    QString jsToExecute = QString("$('#name-workout').val( '%1' ); ").arg(name);
//    jsToExecute += QString("$('#plan-workout').val( '%1' ); ").arg(planEscaped);
//    jsToExecute += QString("$('#creator-workout').val( '%1' ); ").arg(creatorEscaped);
//    jsToExecute += QString("$('#description-workout').val( '%1' ); ").arg(descriptionEscaped);

//    jsToExecute += QString("$('#select-type-workout').val( %1 );").arg(type);
//    jsToExecute += "$('#select-type-workout').selectpicker('refresh');";
//    ui->webView_createWorkout->page()->mainFrame()->documentElement().evaluateJavaScript(jsToExecute + "; null");
}





//-------------------------------------------------------
QString GoogleMapWidget::loadPathImportCourse() {

    QSettings settings;

    settings.beginGroup("ImporterCourse");
    QString path = settings.value("loadPath", QDir::homePath() ).toString();
    settings.endGroup();

    return path;

}
//------------------------------------------------------------------
void GoogleMapWidget::savePathImportCourse(QString filepath) {

    QSettings settings;

    settings.beginGroup("ImporterCourse");
    settings.setValue("loadPath", filepath);
    settings.endGroup();
}







/////////////////////////////////////////////////////////////////////////////////////////////////////////
void GoogleMapWidget::paintEvent(QPaintEvent *) {
    QStyleOption opt;
    opt.init(this);
    QPainter p(this);
    style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);
}

