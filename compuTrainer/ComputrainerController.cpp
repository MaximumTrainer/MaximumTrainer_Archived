#include "computrainercontroller.h"

#include <QDebug>
#include <QTimer>
#include <QEventLoop>
#include <QSettings>


CompuTrainerController::~CompuTrainerController()
{


    thread->quit();
    //    thread->wait();

}





CompuTrainerController::CompuTrainerController(QObject *parent) : QObject(parent)
{

    gotCT = false;
    isCalibrated = false;



    thread = new QThread;
    compuTrainer = new CompuTrainer;
    compuTrainer->moveToThread(thread);


    connect(thread, SIGNAL(finished()), compuTrainer, SLOT(deleteLater()) );
    connect(thread, SIGNAL(finished()), thread, SLOT(deleteLater()));



    //Sent to CT
    connect(this, SIGNAL(signal_quick_search()), compuTrainer, SLOT(quickSearch()) );
    connect(this, SIGNAL(signal_search()), compuTrainer, SLOT(search()) );
    connect(this, SIGNAL(signal_start()), compuTrainer, SLOT(start()) );
    connect(this, SIGNAL(signal_stop_resetIDLE()), compuTrainer, SLOT(stop_resetIDLE()) );
    connect(this, SIGNAL(signal_setLoad(double)), compuTrainer, SLOT(setLoad(double)) );
    connect(this, SIGNAL(signal_setSlope(double)), compuTrainer, SLOT(setSlopeAt(double)) );





    //from CT
    connect(compuTrainer, SIGNAL(ctFound(bool,int,int,bool)), this, SLOT(ctFound(bool,int,int,bool)) );
    connect(compuTrainer, SIGNAL(debugMesg(QString)), this, SIGNAL(signal_debugMesg(QString)) );
    connect(compuTrainer, SIGNAL(dataUpdated(float,float,float,float,float)), this, SIGNAL(signal_dataUpdated(float,float,float,float,float)) );

    connect(compuTrainer, SIGNAL(pauseClicked(bool)), this, SIGNAL(signal_pauseClicked(bool)) );
    connect(compuTrainer, SIGNAL(calibrationStarted(bool,int)), this, SIGNAL(signal_calibrationStarted(bool,int)) );






    thread->start();
}



//-------------------------------------------------------
void CompuTrainerController::send_signal_search() {
    emit signal_search();
}

//-------------------------------------------------------
void CompuTrainerController::send_signal_quicksearch() {
    emit signal_quick_search();
}

//------------------------------------------------------------------------------------
void CompuTrainerController::ctFound(bool found, int comPort, int firmware, bool calibrated) {

    qDebug() << "CompuTrainerController::ctFound!" << found << comPort << firmware << calibrated;
    gotCT = found;
    isCalibrated = calibrated;
    emit signal_ctFound(found, comPort, firmware, calibrated);
}



//-------------------------------------------------------------------
void CompuTrainerController::start() {

    qDebug() << "CompuTrainerController::start()";
    emit signal_start();
}
//-------------------------------------------------------------------
void CompuTrainerController::stop_resetIDLE() {

    qDebug() << "CompuTrainerController::stop_resetIDLE()";
    emit signal_stop_resetIDLE();
}
//-------------------------------------------------------------------
void CompuTrainerController::setLoad(int antID, double watts) {

    Q_UNUSED(antID);

    emit signal_setLoad(watts);
}

//-------------------------------------------------------------------
void CompuTrainerController::setSlope(int antID, double slope) {

    Q_UNUSED(antID);

    emit signal_setSlope(slope);
}








//////////////////////////////////////////////////////////////////////////////////////////////////
//void CompuTrainerController::loadAutoCalibrate() {

//    QSettings settings;

//    settings.beginGroup("CompuTrainer");
//    autoCalibrate = settings.value("autoCalibrate", false).toBool();
//    settings.endGroup();

//}
////------------------------------------------------------
//void CompuTrainerController::saveAutoCalibrate() {

//    QSettings settings;

//    settings.beginGroup("CompuTrainer");
//    settings.setValue("autoCalibrate", autoCalibrate);
//    settings.endGroup();
//}


