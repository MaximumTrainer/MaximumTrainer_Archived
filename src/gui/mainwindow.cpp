#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QDebug>
#include <QSettings>
#include <QMessageBox>
#include <QCloseEvent>
#include <QDesktopServices>
#include <QApplication>
#include <QFileDialog>

#include "util.h"
#include "environnement.h"
#include "userdao.h"
#include "savingwindow.h"
#include "soundplayer.h"
#include "dialoginfowebview.h"
#include "dialogmainwindowconfig.h"
#include "savingwindow.h"
#include "workoutdialog.h"
#include "workout.h"
#include "radiodao.h"
#include "mycreatorplot.h"
#include "reportutil.h"
#include "importerworkout.h"
#include "managerachievement.h"

#include <QWebEngineProfile>
#include <QWebEngineScript>
#include <QWebEnginePage>
#include <QWebEngineScriptCollection>
#include <QWebChannel>
#include "myqwebenginepage.h"

#include "extrequest.h"





MainWindow::~MainWindow() {
    delete ui;

    qDebug() << "Desctructor MainWindow";

    for (int i=0; i<vecThread.size(); i++) {
        qDebug() << "Stopping this thread.." << i;
        QThread *thread = vecThread.at(i);
        thread->quit();
        thread->wait();
    }

    qDebug() << "Desctructor Over MainWindow";
}





MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), ui(new Ui::MainWindow) {

    ui->setupUi(this);
    this->setEnabled(false);
    ui->widget_bottomMenu->setGeneralMessage(tr("Retrieving Data...")); //will be removed when response is received
    ui->actionAbout_Qt->setIcon(QIcon(":/qt-project.org/qmessagebox/images/qtlogo-64.png"));
    ui->actionOpen_Ride->setEnabled(true);

    this->settings = qApp->property("User_Settings").value<Settings*>();
    this->account = qApp->property("Account").value<Account*>();

    zoneObject = new ZoneObject(this);         /// Used with QWebView zone page
    planObject = new PlanObject(this);         ///Used with QWebView Plan page


    createWebChannelPlan();
    createWebChannelZone();
    createWebChannelSettings();
    createWebChannelStudio();

    ui->webView_zones->setUrl(QUrl(Environnement::getUrlZones()));
    ui->webView_achiev->setUrl(QUrl(Environnement::getUrlAchievement()));
    ui->webView_settings->setUrl(QUrl(Environnement::getUrlSettings()));
    ui->webView_plan->setUrl(QUrl(Environnement::getUrlPlans()));
    ui->webView_studio->setUrl(QUrl(Environnement::getUrlStudio()));
    ui->webView_ergDb->setUrl(QUrl("https://www.ergdb.org/maximum-trainer/"));



    // ------------------------------------- Start Hubs ---------------------------------
    hubStickInitFinished = false;
    numberInitStickDone = 0;
    numberStickFound = 0;
    descSerialSticks = "";
    closedCSMFinishedNb = 0;
    startHub();


    stravaUploadID = -1;
    saveAccountTry = 0;

    ftb = new FancyTabBar(FancyTabBar::TabBarPosition::Left, ui->widget_fancyMenu);
    ftb->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    ftb->insertTab(0, QIcon(":/image/icon/workoutMan"), tr("Workout"));
    ftb->insertTab(1, QIcon(":/image/icon/images/ergdb-logo-black-rider.png"), tr("Find Workouts"));
    ftb->insertTab(2, QIcon(":/image/icon/calendar"),  tr("Plan"));
    ftb->insertTab(3, QIcon(":/image/icon/studio"), tr("Studio"));
    ftb->insertTab(4, QIcon(":/image/icon/user"), tr("Profile"));
    ftb->insertTab(5, QIcon(":/image/icon/antmenu"), "ANT+");

    ftb->setTabEnabled(0, true);
    ftb->setTabEnabled(1, true);
    ftb->setTabEnabled(2, true);
    ftb->setTabEnabled(3, true);
    ftb->setTabEnabled(4, true);
    ftb->setTabEnabled(5, true);



    ftb->setCurrentIndex(0);
    connect(ftb, SIGNAL(currentChanged(int)), this, SLOT(leftMenuChanged(int)) );


    ui->tabWidget_workout->tabBar()->setObjectName("tabBarWorkout");
    setStyleSheet(qApp->styleSheet());


    //Load userStudio xml file to VecUserStudio
    XmlUtil *xmlUtil = new XmlUtil(settings->language, this);
    vecUserStudio = xmlUtil->parseUserStudioFile("");


    ManagerAchievement *achievementManager = new ManagerAchievement(this);
    qApp->setProperty("ManagerAchievement", QVariant::fromValue<ManagerAchievement*>(achievementManager));



    //Parse Workouts
    ui->tab_workout1->parseIncludedWorkouts();
    ui->tab_workout1->parseMapWorkout(account->FTP);
    ui->tab_workout1->parseUserWorkouts();


    currentIndexLeftMenu = 0;
    ftpChanged = false;


    // connect Workout Dialog done with refresh Training Data page
    connect (ui->tab_workout1, SIGNAL(executeWorkout(Workout)), this, SLOT(executeWorkout(Workout)) );


    /// Connect Workout Creator --> WorkoutList Page
    connect(ui->tab_create, SIGNAL(workoutCreated(Workout)), ui->tab_workout1, SLOT(addWorkout(Workout)));
    connect(ui->tab_create, SIGNAL(workoutCreated(Workout)), this, SLOT(showWorkoutList()));
    connect(ui->tab_create, SIGNAL(workoutOverwrited(Workout)), ui->tab_workout1, SLOT(overwriteWorkout(Workout)));
    connect(ui->tab_create, SIGNAL(workoutOverwrited(Workout)), this, SLOT(showWorkoutList()));

    connect(ui->tab_workout1, SIGNAL(editWorkout(Workout)), this, SLOT(showWorkoutCreator()));
    connect(ui->tab_workout1, SIGNAL(editWorkout(Workout)), ui->tab_create, SLOT(editWorkout(Workout)));



    connect(ui->tab_create, SIGNAL(showStatusBarMessage(QString, int)), ui->widget_bottomMenu, SLOT(setGeneralMessage(QString,int)));
    connect(ui->tab_workout1, SIGNAL(signal_exportWorkoutToPdf(Workout)), this, SLOT(exportWorkoutToPdf(Workout)) );


    /// Update create workout graph on FTP and LTHR change
    connect(zoneObject, SIGNAL(signal_updateFTP()), this, SLOT(setFlagFtpChanged()) );
    connect(zoneObject, SIGNAL(signal_updateLTHR()), this, SLOT(setFlagFtpChanged()) );
    connect(this, SIGNAL(ftpAndTabProfileChanged()), ui->tab_create, SLOT(computeWorkout()) );
    ///Also update workout metrics based on FTP
    connect(this, SIGNAL(ftpAndTabProfileChanged()), ui->tab_workout1, SLOT(updateTableViewMetrics()) );
    connect(this, SIGNAL(ftpAndTabProfileChanged()), ui->tab_workout1, SLOT(refreshMapWorkout()) );



    /// Load Settings
    loadSettings();
    isInsideWorkout = false;


    //DialogConfig
    dconfig = new DialogMainWindowConfig(this);

    //    dconfig->setModal(true);
    connect(dconfig, SIGNAL(folderWorkoutChanged()), ui->tab_workout1, SLOT(refreshUserWorkout()) );


    leftMenuChanged(0);
    enableStudioMode(account->enable_studio_mode);



    ///Retrieve Internet Radio from DB
    replyRadioDone = false;
    replyRadio = RadioDAO::getAllRadios();
    connect(replyRadio, SIGNAL(finished()), this, SLOT(slotFinishedGetRadio()));

    qDebug() << "Test1";

    connect(planObject, SIGNAL(signal_goToWorkout(QString)), this, SLOT(goToWorkoutPlanFilter(QString)) );


    connect(ui->webView_settings, SIGNAL(loadFinished(bool)), this, SLOT(fillSettingPage()));
    connect(ui->webView_studio, SIGNAL(loadFinished(bool)), this, SLOT(fillStudioPage()));

    connect(ui->webView_ergDb->page()->profile(), SIGNAL(downloadRequested(QWebEngineDownloadItem*)),
                    this, SLOT(downloadRequested(QWebEngineDownloadItem*)));

}


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void MainWindow::downloadRequested(QWebEngineDownloadItem* download) {

    qDebug() << "Format: " <<  download->savePageFormat();
    qDebug() << "Path: " << download->path();

    QFileInfo fileInfo(download->path());
    QString filename(fileInfo.fileName());

//    fileInfo.

    download->setPath(Util::getSystemPathWorkout() + QDir::separator() + "ergdb" + QDir::separator() + filename);
    qDebug() << "Path: " << download->path();
    download->accept();


    this->lastWorkoutNameDownloaded = fileInfo.completeBaseName();
    // should show the workout in the list so user is ready to do it?
    connect(download, SIGNAL(finished()),
            this, SLOT(goToWorkoutNameFilter()));
}


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void MainWindow::startHub() {

    qDebug() << "ok creating a thread for a hub here.." << numberInitStickDone;
    ui->widget_bottomMenu->updateHubStatus(numberInitStickDone);

    QThread *thread = new QThread;
    Hub *hub = new Hub(numberInitStickDone);
    hub->moveToThread(thread);

    vecHub.append(hub);
    vecThread.append(thread);

    connect(thread, SIGNAL(finished()), hub, SLOT(deleteLater()) );
    connect(thread, SIGNAL(finished()), thread, SLOT(deleteLater()));

    connect(hub, SIGNAL(stickFound(bool,QString,int)), this, SLOT(setStickFound(bool,QString,int)) );
    connect(hub, SIGNAL(sensorFound(int,int,QList<int>,QList<int>,bool)), this, SLOT(setSensorFound(int,int,QList<int>,QList<int>,bool)) );

    connect(this, SIGNAL(signal_hubInitUsbStick(int)), hub, SLOT(initUSBStick(int)) );
    connect(this, SIGNAL(signal_hubCloseChannelCSM(bool)), hub, SLOT(closeScanningModeChannel(bool)) );
    connect(this, SIGNAL(signal_searchForSensor(int, bool, int, bool)), hub, SLOT(startPairing(int, bool, int, bool)) );
    connect(this, SIGNAL(signal_stopPairing()), hub, SIGNAL(stopPairing()) );

    thread->start();

    emit signal_hubInitUsbStick(numberInitStickDone);
    //QThread::msleep(100); //Wait a bit, discovering USB device is done in sequence?

    //hack to we don't try to start this thread again the next loop, because all hubs receive this signal we would need to use an ID
    disconnect(this, SIGNAL(signal_hubInitUsbStick(int)), hub, SLOT(initUSBStick(int)) );

}



//---------------------------------------------------------------------------------
void MainWindow::checkToEnableWindow() {

    if (replyRadioDone && hubStickInitFinished) {
        this->setEnabled(true);
    }
}



//---------------------------------------------------------------------------------
void MainWindow::slotFinishedGetRadio() {


    //success
    if (replyRadio->error() == QNetworkReply::NoError) {
        qDebug() << "Get radio finished!";
        QByteArray arrayData =  replyRadio->readAll();
        lstRadio = Util::parseJsonRadioList(arrayData);
        //enable mainWindow
        ui->widget_bottomMenu->removeGeneralMessage();

        replyRadioDone = true;
        checkToEnableWindow();


        replyRadio->deleteLater();
    }
    else {
        qDebug() << "Problem getting radio list! retry again..." << replyRadio->errorString();
        ui->widget_bottomMenu->setGeneralMessage("Error retrieving data from Server..." + replyRadio->errorString(), 7000);
        replyRadio = RadioDAO::getAllRadios();
        connect(replyRadio, SIGNAL(finished()), this, SLOT(slotFinishedGetRadio()));
    }

}



//---------------------------------------------------------------------------------
void MainWindow::exportWorkoutToPdf(const Workout& workout) {


    qDebug() << "Export to PDF!";

    myCreatorPlot myPlot(this);
    myPlot.updateWorkout(workout);


    myPlot.setAxisTitle(2, tr("Time"));


    QString fileNameToShow = myPlot.getSavePathExport() + QDir::separator() +  workout.getName();
    QString fileName = QFileDialog::getSaveFileName(this, tr("Export Workout"), fileNameToShow, tr("PDF Documents(*.pdf)"));
    if (fileName.isEmpty())
        return;

    //Save path for future uses
    QFileInfo fileInfo(fileName);
    myPlot.savePathExport(fileInfo.absolutePath());


    ReportUtil::printWorkoutToPdf(workout, &myPlot, fileName);
    ui->widget_bottomMenu->setGeneralMessage(tr("Saved to: ") + fileName, 7000);
}

//---------------------------------------------------------------------------------
void MainWindow::goToWorkoutPlanFilter(const QString& plan) {

    qDebug() << "mainWindow Plan" << plan;

    ui->tabWidget_workout->setCurrentIndex(0);
    ui->tab_workout1->setFilterPlanName(plan);
    ftb->setCurrentIndex(0);
    //    ui->tab_workout1

}

// trigger after a download is finish on ergDb tab
//---------------------------------------------------------------------------------
void MainWindow::goToWorkoutNameFilter() {

    ui->tab_workout1->refreshUserWorkout();

    qDebug() << "mainWindow name" << this->lastWorkoutNameDownloaded;

    ui->tabWidget_workout->setCurrentIndex(0);
    ui->tab_workout1->setFilterWorkoutName(lastWorkoutNameDownloaded);
    ftb->setCurrentIndex(0);


}





/////////////////////////////////////////////////////////////////////////////////
void MainWindow::workoutExecuting() {

    this->setDisabled(true);
    isInsideWorkout = true;
    qDebug() << "WORKOUT EXECUTING************";


    for (int i=0; i<ftb->getNumberOfTabs(); i++)
        ftb->setTabEnabled(i, false);

}
/////////////////////////////////////////////////////////////////////
void MainWindow::workoutOver() {

    this->setDisabled(false);
    isInsideWorkout = false;
    qDebug() << "WORKOUT DONE************";

    for (int i=0; i<ftb->getNumberOfTabs(); i++)
        ftb->setTabEnabled(i, true);

    if (account->enable_studio_mode) {
        enableStudioMode(true);
    }

}



/////////////////////////////////////////////////////////////////////////////////
void MainWindow::setFlagFtpChanged() {

    ftpChanged = true;

}

/////////////////////////////////////////////////////////////////////////////////////////////
void MainWindow::leftMenuChanged(int tabSelected) {

    //    qDebug() << "leftMenuChanged" << tabSelected;

    //    if (tabSelected == 0) {
    //        ui->label_headerMain->setText(tr("Workout"));
    //        ui->label_iconMenu->setStyleSheet("QLabel#label_iconMenu{image: url(:/image/icon/road);border-radius: 1px;}");
    //    }
    //    else if (tabSelected == 1) {
    //        ui->label_headerMain->setText(tr("Plan"));
    //        ui->label_iconMenu->setStyleSheet("QLabel#label_iconMenu{image: url(:/image/icon/calendar);border-radius: 1px;}");
    //    }
    //    else if (tabSelected == 2) {
    //        ui->label_headerMain->setText(tr("Profile"));
    //        ui->label_iconMenu->setStyleSheet("QLabel#label_iconMenu{image: url(:/image/icon/user);border-radius: 1px;}");
    //    }
    //    else if (tabSelected == 3) {
    //        ui->label_headerMain->setText(tr("Gear"));
    //        ui->label_iconMenu->setStyleSheet("QLabel#label_iconMenu{image: url(:/image/icon/gear);border-radius: 1px;}");
    //    }



    ///Keyword: RemoveCOURSE Temp
    /// to change index is different with course added..

    if (currentIndexLeftMenu == 3 && ftpChanged) { ///Also check that FTP has changed..
        qDebug() << "-*-*-*-OK FTP CHANGED, RECALCULATE!";
        emit ftpAndTabProfileChanged();
        ftpChanged = false;
    }

    ui->stackedWidget_menu->setCurrentIndex(tabSelected);
    currentIndexLeftMenu = tabSelected;

}



//------------------------------------------------------------
void MainWindow::createWebChannelZone() {

    qDebug() << "createWebChannelZone";

    QFile webChannelJsFile(":/qtwebchannel/qwebchannel.js");
    if(  !webChannelJsFile.open(QIODevice::ReadOnly) ) {
        qDebug() << QString("Couldn't open qwebchannel.js file: %1").arg(webChannelJsFile.errorString());
    }
    else {
        qDebug() << "OK webEngineProfile";
        QByteArray webChannelJs = webChannelJsFile.readAll();
        webChannelJs.append(
                    "\n"
                    "var zoneObject"
                    "\n"
                    "new QWebChannel(qt.webChannelTransport, function(channel) {"
                    "     zoneObject = channel.objects.zoneObject;"
                    "});"
                    "\n"
                    );

        QWebChannel *channel = new QWebChannel(ui->webView_zones);
        QWebEngineScript script;
        script.setSourceCode(webChannelJs);
        script.setName("qwebchannel.js");
        script.setWorldId(QWebEngineScript::MainWorld);
        script.setInjectionPoint(QWebEngineScript::DocumentCreation);
        script.setRunsOnSubFrames(false);

        ui->webView_zones->page()->scripts().insert(script);
        ui->webView_zones->page()->setWebChannel(channel);
        channel->registerObject("zoneObject", zoneObject);

        // execute updateCdA() js function to init value
        ui->webView_studio->page()->runJavaScript("updateCdA();");
    }
}


//------------------------------------------------------------
void MainWindow::createWebChannelPlan() {

    qDebug() << "createWebChannelPlan";

    QFile webChannelJsFile(":/qtwebchannel/qwebchannel.js");
    if(  !webChannelJsFile.open(QIODevice::ReadOnly) ) {
        qDebug() << QString("Couldn't open qwebchannel.js file: %1").arg(webChannelJsFile.errorString());
    }
    else {
        qDebug() << "OK webEngineProfile";
        QByteArray webChannelJs = webChannelJsFile.readAll();
        webChannelJs.append(
                    "\n"
                    "var planObject"
                    "\n"
                    "new QWebChannel(qt.webChannelTransport, function(channel) {"
                    "     planObject = channel.objects.planObject;"
                    "});"
                    "\n"
                    );

        QWebChannel *channel = new QWebChannel(ui->webView_plan);
        QWebEngineScript script;
        script.setSourceCode(webChannelJs);
        script.setName("qwebchannel.js");
        script.setWorldId(QWebEngineScript::MainWorld);
        script.setInjectionPoint(QWebEngineScript::DocumentCreation);
        script.setRunsOnSubFrames(false);

        // External links
        QStringList lstExternal = {"forum", "cms" };
        MyQWebEnginePage *myPage = new MyQWebEnginePage(ui->webView_plan);
        myPage->setExternalList(lstExternal);
        ui->webView_plan->setPage(myPage);

        ui->webView_plan->page()->scripts().insert(script);
        ui->webView_plan->page()->setWebChannel(channel);
        channel->registerObject("planObject", planObject);
    }
}


//------------------------------------------------------------
void MainWindow::createWebChannelSettings() {

    qDebug() << "createWebChannelSettings";

    QFile webChannelJsFile(":/qtwebchannel/qwebchannel.js");
    if(  !webChannelJsFile.open(QIODevice::ReadOnly) ) {
        qDebug() << QString("Couldn't open qwebchannel.js file: %1").arg(webChannelJsFile.errorString());
    }
    else {
        qDebug() << "OK webEngineProfile";
        QByteArray webChannelJs = webChannelJsFile.readAll();
        webChannelJs.append(
                    "\n"
                    "var MainWindow"
                    "\n"
                    "new QWebChannel(qt.webChannelTransport, function(channel) {"
                    "     MainWindow = channel.objects.MainWindow;"
                    "});"
                    "\n"
                    );

        QWebChannel *channel = new QWebChannel(ui->webView_settings);
        QWebEngineScript script;
        script.setSourceCode(webChannelJs);
        script.setName("qwebchannel.js");
        script.setWorldId(QWebEngineScript::MainWorld);
        script.setInjectionPoint(QWebEngineScript::DocumentCreation);
        script.setRunsOnSubFrames(false);

        ui->webView_settings->page()->scripts().insert(script);
        ui->webView_settings->page()->setWebChannel(channel);
        channel->registerObject("MainWindow", this);
    }
}


//------------------------------------------------------------
void MainWindow::createWebChannelStudio() {

    qDebug() << "createWebChannelStudio";

    QFile webChannelJsFile(":/qtwebchannel/qwebchannel.js");
    if(  !webChannelJsFile.open(QIODevice::ReadOnly) ) {
        qDebug() << QString("Couldn't open qwebchannel.js file: %1").arg(webChannelJsFile.errorString());
    }
    else {
        qDebug() << "OK webEngineProfile";
        QByteArray webChannelJs = webChannelJsFile.readAll();
        webChannelJs.append(
                    "\n"
                    "var MainWindow"
                    "\n"
                    "new QWebChannel(qt.webChannelTransport, function(channel) {"
                    "     MainWindow = channel.objects.MainWindow;"
                    "});"
                    "\n"
                    );

        QWebChannel *channel = new QWebChannel(ui->webView_studio);
        QWebEngineScript script;
        script.setSourceCode(webChannelJs);
        script.setName("qwebchannel.js");
        script.setWorldId(QWebEngineScript::MainWorld);
        script.setInjectionPoint(QWebEngineScript::DocumentCreation);
        script.setRunsOnSubFrames(false);

        ui->webView_studio->page()->scripts().insert(script);
        ui->webView_studio->page()->setWebChannel(channel);
        channel->registerObject("MainWindow", this);

    }
}


/////////////////////////////////////////////////////////////////////////////////////////////////////////////
void MainWindow::fillSettingPage()  {

    qDebug() << "fillSettingPage -  Set the pmUseForCadence " << account->use_pm_for_cadence;

    QString jsCode = QString("$('#switch-pm-cadence').bootstrapSwitch('state', %1);").arg(account->use_pm_for_cadence);
    jsCode += QString("$('#switch-pm-speed').bootstrapSwitch('state', %1);").arg(account->use_pm_for_speed);

    //using trainer curve?
    if (account->powerCurve.getId() > 0) {
        jsCode += QString("$('#power-data-source').trigger('change');");
        jsCode += QString("$('#select-company').trigger('change');");
    }

    qDebug() << "jsCodeISL:" << jsCode;


    ui->webView_settings->page()->runJavaScript(jsCode);
}



/////////////////////////////////////////////////////////////////////////////////////////////////////////////
void MainWindow::updateVecStudio(QVector<UserStudio> vecUserStudio) {

    qDebug() << "zzzzz MainWindow::updateVecStudio";
    this->vecUserStudio = vecUserStudio;

    fillStudioPage();
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
void MainWindow::fillStudioPage() {

    qDebug() << "fillStudioPage -  Set the Checkbox to " << account->enable_studio_mode;

    QString jsCode = QString("$('#switch-enable-studio').bootstrapSwitch('state', %1);").arg(account->enable_studio_mode);
    jsCode += QString("$('#select-number-workout').val(%1);").arg(account->nb_user_studio);
    jsCode += "$('#select-number-workout').trigger('change');";


    qDebug() << "JSTOEXECUTE IS:" << jsCode;
    ui->webView_studio->page()->runJavaScript(jsCode);


    //-- Populate QwebView Studio
    QString jsToExecute = "";
    for (int i=0; i<vecUserStudio.size(); i++) {

        int userID = i+1;
        UserStudio userStudio = vecUserStudio.at(i);

        // UserStudio dummyUser("", -1, -1, -1, -1, -1, -1, -1, 2100, false, 0, 0);
        jsToExecute += QString("$('#input-display-name_" +QString::number(userID)+ "').val('%1');").arg(userStudio.getDisplayName());
        if (userStudio.getFTP() > 0)
            jsToExecute += QString("$('#input-ftp_" +QString::number(userID)+ "').val(%1);").arg(userStudio.getFTP());
        if (userStudio.getLTHR() > 0)
            jsToExecute += QString("$('#input-lthr_" +QString::number(userID)+ "').val(%1);").arg(userStudio.getLTHR());
        if (userStudio.getHrID() > 0)
            jsToExecute += QString("$('#sensor-hr-id_" +QString::number(userID)+ "').val(%1);").arg(userStudio.getHrID());
        if (userStudio.getPowerID() > 0)
            jsToExecute += QString("$('#sensor-power-id_" +QString::number(userID)+ "').val(%1);").arg(userStudio.getPowerID());
        if (userStudio.getCadenceID() > 0)
            jsToExecute += QString("$('#sensor-cadence-id_" +QString::number(userID)+ "').val(%1);").arg(userStudio.getCadenceID());
        if (userStudio.getSpeedID() > 0)
            jsToExecute += QString("$('#sensor-speed-id_" +QString::number(userID)+ "').val(%1);").arg(userStudio.getSpeedID());
        if (userStudio.getFecID() > 0)
            jsToExecute += QString("$('#sensor-fec-id_" +QString::number(userID)+ "').val(%1);").arg(userStudio.getFecID());
        if (userStudio.getWheelCircMM() > 0)
            jsToExecute += QString("$('#wheelcirc_" +QString::number(userID)+ "').val(%1);").arg(userStudio.getWheelCircMM());
        jsToExecute += QString("$('#switch-power-curve_" +QString::number(userID)+ "').bootstrapSwitch('state', %1);").arg(userStudio.getUsingPowerCurve());

        if (userStudio.getUsingPowerCurve()) {
            jsToExecute += QString("$('#select-company_" +QString::number(userID)+ "').val(%1);").arg(userStudio.getCompanyID());
            jsToExecute += "$('#select-company_"  +QString::number(userID)+ "').selectpicker('refresh');";
            jsToExecute += "$('#select-company_"  +QString::number(userID)+ "').trigger('change');";
        }
    }


    qDebug() << "JSTOEXECUTE IS:" << jsToExecute;
    ui->webView_studio->page()->runJavaScript(jsToExecute);
}



/////////////////////////////////////////////////////////////////////////////////////////////
void MainWindow::companyLoadedForUser(int riderID) {

    if (riderID >= vecUserStudio.size())
        return;

    qDebug() << "companyLoadedForUser" << riderID;

    UserStudio myUserStudio = vecUserStudio.at(riderID-1);

    if (myUserStudio.getBrandID() > 0) {

        qDebug() << "OK This one got a brand ID, send to QWebView brand id!";

        QString jsToExecute = "";
        jsToExecute += QString("$('#select-trainer_" +QString::number(riderID)+ "').val(%1);").arg(myUserStudio.getBrandID());
        jsToExecute += "$('#select-trainer_"  +QString::number(riderID)+ "').selectpicker('refresh');";
        jsToExecute += "$('#select-trainer_"  +QString::number(riderID)+ "').trigger('change');";
        //        ui->webView_studio->page()->mainFrame()->documentElement().evaluateJavaScript(jsToExecute + "; null");
        ui->webView_studio->page()->runJavaScript(jsToExecute);
    }

}


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void MainWindow::setPowerCurveForUser(int riderID, int company_id, int trainer_id, QString companyName, QString trainerName,
                                      double coef0, double coef1, double coef2, double coef3, int formulaInCode) {

    if (riderID >= vecUserStudio.size())
        return;

    UserStudio myUserStudio = vecUserStudio.at(riderID-1);

    qDebug() << "setPowerCurveForUser" << "riderID" << riderID << "company_id" << company_id << "trainer_id" << trainer_id << "companyName" << companyName << "trainerName" << trainerName <<
                "coef0" << coef0 << "coef1" << coef1 << "coef2" << coef2 << "coef3" << coef3 << "formulaInCode" << formulaInCode ;

    myUserStudio.setUsingPowerCurve(true);
    myUserStudio.setCompanyID(company_id);
    myUserStudio.setBrandID(trainer_id);

    PowerCurve myCurve = myUserStudio.getPowerCurve();
    myCurve.setId(trainer_id);
    myCurve.setName(companyName, trainerName);
    myCurve.setCoefs(coef0, coef1, coef2, coef3);
    myCurve.setFormulaInCode(formulaInCode);

    myUserStudio.setPowerCurve(myCurve);
    vecUserStudio.replace(riderID-1, myUserStudio);


    //    UserStudio testUserStudio = vecUserStudio.at(riderID-1);
    //    qDebug() << "PowerCurve for user 2 is now:" << testUserStudio.getPowerCurve().getFullName();

    for (int i=0; i<vecUserStudio.size(); i++) {

        UserStudio userStudio = vecUserStudio.at(i);
        qDebug() << "User Studio power Curve is_WORKOUTDIALOG:" << userStudio.getPowerCurve().getFullName() << userStudio.getPowerCurve().getId() << "test att:" << userStudio.getDisplayName();
    }

}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void MainWindow::disablePowerCurveForUser(int riderID) {

    qDebug() << "disablePowerCurveForUser" << riderID;
    if (riderID >= vecUserStudio.size())
        return;

    UserStudio myUserStudio = vecUserStudio.at(riderID-1);
    myUserStudio.setUsingPowerCurve(false);
    myUserStudio.setCompanyID(0);
    myUserStudio.setBrandID(0);
    PowerCurve powerCurve;
    myUserStudio.setPowerCurve(powerCurve);
    vecUserStudio.replace(riderID-1, myUserStudio);

}


//QString displayName;  = 0
//int FTP;              = 1
//int LTHR;             = 2
//int hrID;             = 3
//int power             = 4
//int cadenceID;        = 5
//int speedID;          = 6
//int fecID;            = 7
//int wheelCircMM;      = 8
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void MainWindow::updateFieldForUser(int riderID, int fieldNumber, QVariant value) {

    qDebug() << "updateDisplayNameForUser" << riderID << "fieldNumber" << fieldNumber << "value" << value;


    qDebug() << "disablePowerCurveForUser" << riderID;
    if (riderID >= vecUserStudio.size())
        return;
    UserStudio myUserStudio = vecUserStudio.at(riderID-1);

    if (fieldNumber == 0) {
        myUserStudio.setDisplayName(value.toString());
    }
    else if (fieldNumber == 1) {
        myUserStudio.setFTP(value.toInt());
    }
    else if (fieldNumber == 2) {
        myUserStudio.setLTHR(value.toInt());
    }
    else if (fieldNumber == 3) {
        myUserStudio.setHrID(value.toInt());
    }
    else if (fieldNumber == 4) {
        myUserStudio.setPowerID(value.toInt());
    }
    else if (fieldNumber == 5) {
        myUserStudio.setCadenceID(value.toInt());
    }
    else if (fieldNumber == 6) {
        myUserStudio.setSpeedID(value.toInt());
    }
    else if (fieldNumber == 7) {
        myUserStudio.setFecID(value.toInt());
    }
    else if (fieldNumber == 8) {
        myUserStudio.setWheelCircMM(value.toInt());
    }

    vecUserStudio.replace(riderID-1, myUserStudio);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void MainWindow::enableStudioMode(bool enable) {

    account->enable_studio_mode = enable;

    //    nb_user_studio

    qDebug() << "Enable Studio Mode!" << enable;


    if (enable) {
        this->setWindowTitle("MaximumTrainer - Studio");
    }
    else {
        this->setWindowTitle("MaximumTrainer");
    }

    ftb->setTabEnabled(4, !enable);
    ftb->setTabEnabled(5, !enable);

}

//////////////////////////////////////////////////////////////////////////////////////////////////////
void MainWindow::setNumberUserStudio(int nbUser) {

    qDebug() << "setNumberUserStudio" << nbUser;
    account->nb_user_studio = nbUser;

}

/////////////////////////////////////////////////////////////////////////////////////////////
void MainWindow::loadConfigStudio() {

    qDebug() << "loadConfigStudio";

    //load path
    QSettings settings;
    settings.beginGroup("studiopath");
    QString path = settings.value("loadPath", Util::getMaximumTrainerDocumentPath() ).toString();
    settings.endGroup();

    QString file = QFileDialog::getOpenFileName(this, tr("Load Studio Profile"),
                                                path,
                                                tr("Studio Save File(*.xml)"));
    if (file.isEmpty())
        return;



    //Parse File and reset QWebView with QVector
    XmlUtil *xmlUtil = new XmlUtil(this->settings->language, this);
    vecUserStudio = xmlUtil->parseUserStudioFile(file);
    fillStudioPage();
    ui->widget_bottomMenu->setGeneralMessage(QString(tr("Studio Profile %1 loaded")).arg(file), 5000);


    //save path
    settings.beginGroup("studiopath");
    settings.setValue("loadPath", file);
    settings.endGroup();
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void MainWindow::saveConfigStudio() {

    qDebug() << "saveConfigStudio";

    //load path
    QSettings settings;
    settings.beginGroup("studiopath");
    QString path = settings.value("loadPath", Util::getMaximumTrainerDocumentPath() ).toString();
    settings.endGroup();

    QString file = QFileDialog::getSaveFileName(this, tr("Save Studio Profile As"),
                                                path,
                                                tr("Studio Save File(*.xml)"));

    if (file.isEmpty())
        return;

    qDebug() << "Saving FILE" << file;

    //Save Studio User to XML File
    bool success = XmlUtil::saveUserStudioFile(vecUserStudio, file);
    if (success) {
        ui->widget_bottomMenu->setGeneralMessage(QString(tr("Studio Profile saved as %1")).arg(file), 5000);
    }

    //save path
    settings.beginGroup("studiopath");
    settings.setValue("loadPath", file);
    settings.endGroup();
}


/////////////////////////////////////////////////////////////////////////////////////////////
void MainWindow::updateZoneInterface() {

    qDebug() << "update zone interface!********************";

    QString jsToExecute = QString("$('#FTP').val( '%1' ); ").arg((QString::number(account->FTP)));
    jsToExecute += QString("$('#LTHR').val( '%1' ); ").arg((QString::number(account->LTHR)));
    //    ui->webView_zones->page()->mainFrame()->documentElement().evaluateJavaScript(jsToExecute + "; null");
    ui->webView_zones->page()->runJavaScript(jsToExecute);

}

/////////////////////////////////////////////////////////////////////////////////////////////
void MainWindow::showWorkoutCreator() {

    ui->tabWidget_workout->setCurrentIndex(1);
}

/////////////////////////////////////////////////////////////////////////////////////////////
void MainWindow::showWorkoutList() {

    qDebug() << "showWorkoutList!*";
    ui->tabWidget_workout->setCurrentIndex(0);
}


/////////////////////////////////////////////////////////////////////////////////////////////
void MainWindow::loadSettings() {



    QSettings settings;


    settings.beginGroup("MainWindow");
    resize(settings.value("size", QSize(1150, 700)).toSize());
    move(settings.value("pos", QPoint(200, 200)).toPoint());
    bool wasMaximized = settings.value("maximized", false).toBool();
    //    resize( QSize(1024, 768));
    //    move(QPoint(200, 200));
    settings.endGroup();


    if (wasMaximized) {
        this->showMaximized();
    }
    else {
        this->showNormal();
    }
}




/////////////////////////////////////////////////////////////////////////////////////////////
void MainWindow::saveSettings() {


    QSettings settings;

    settings.beginGroup("MainWindow");
    settings.setValue("size", size());
    settings.setValue("pos", pos());

    if (this->isMaximized()) {
        settings.setValue("maximized", true);
    }
    else {
        settings.setValue("maximized", false);
    }

    settings.endGroup();



}




////////////////////////////////////////////////////////////////////////////////////////////
void MainWindow::setSensorFound(int deviceType, int numberDeviceFound, QList<int> lstDevicePairedr, QList<int> lstTypeDevicePairedr, bool fromStudioPage) {

    qDebug() << "setSensorFound" << deviceType << "numberDeviceFound" << numberDeviceFound << lstDevicePairedr.size() << lstTypeDevicePairedr.size() << fromStudioPage;

    pairingResponseNumber++;


    // Check if we already got theses sensors, if not, add to mainList
    for(int i=0; i<lstDevicePairedr.size(); i++) {

        int antID = lstDevicePairedr.at(i);
        int deviceType = lstTypeDevicePairedr.at(i);

        if (!this->lstDevicePaired.contains(antID)) {
            qDebug() << "This One is a new! add it" << antID;
            this->lstDevicePaired.append(antID);
            this->lstTypeDevicePaired.append(deviceType);
        }
    }


    if (!pairingResponseAlreadySent) {

        // We got one ID already, stop pairing!
        if (account->stop_pairing_on_found && this->lstDevicePaired.size() > 0) {
            //Stop looking in other Thread
            qDebug() << "Ok Stop other pairing thread!";
            pairingResponseAlreadySent = true;
            emit signal_stopPairing();
            sendDataToSettingsOrStudioPage(deviceType, this->lstDevicePaired.size(), this->lstDevicePaired, this->lstTypeDevicePaired, fromStudioPage);


        }

        // Last hub replied, pairing over!
        if (pairingResponseNumber == numberInitStickDone && !pairingResponseAlreadySent) {

            qDebug() << "last hub replied! pairing done!";
            sendDataToSettingsOrStudioPage(deviceType, this->lstDevicePaired.size(), this->lstDevicePaired, this->lstTypeDevicePaired, fromStudioPage);
        }

    }
    else {
        qDebug() << "We already sent response, ignore this!";
    }





}


void MainWindow::sendDataToSettingsOrStudioPage(int deviceType, int numberDeviceFound, QList<int> lstDevicePaired, QList<int> lstTypeDevicePaired, bool fromStudioPage) {

    qDebug() << "sendDataToSettingsOrStudioPage" << deviceType << numberDeviceFound << "..." << fromStudioPage;
    //transform List to JS format
    QStringList temp;
    std::transform(lstDevicePaired.begin(), lstDevicePaired.end(), std::back_inserter(temp), [](int i){ return QString::number(i); });

    QStringList temp2;
    std::transform(lstTypeDevicePaired.begin(), lstTypeDevicePaired.end(), std::back_inserter(temp2), [](int i){ return QString::number(i); });

    QString script("foundSensor(%1, %2, [%3], [%4], %5);");
    QString arg1= QString::number(deviceType);
    QString arg2= QString::number(lstDevicePaired.size());
    QString arg3 = temp.join(',');
    QString arg4 = temp2.join(',');
    QString arg5 = QString::number(fromStudioPage);

    QString jsToRun = script.arg(arg1, arg2, arg3, arg4, arg5);
    qDebug() << "here is the script to run:" << jsToRun;

    if (fromStudioPage) {
        qDebug() << "send the script to studio page";
        ui->webView_studio->page()->runJavaScript(jsToRun);
    }
    else {
        qDebug() << "send the script to settings page";
        ui->webView_settings->page()->runJavaScript(jsToRun);
    }

}

//----------------------------------------------------------------
void MainWindow::setStickFound(bool found, QString serial, int stickNumber) {


    numberInitStickDone++;
    QString lineMsgStick;
    if (found) {
        numberStickFound++;
        lineMsgStick = serial;
        vecStickIdUsed.append(stickNumber);

        descSerialSticks += tr("Stick#") + QString::number(stickNumber) +  " : " + lineMsgStick + "<br/>";

        qDebug() << "Found a stick, then continue searching for more!";
        startHub();
    }
    else {
        qDebug() << "No more stick, stop searching!";
        hubStickInitFinished = true;

        ui->widget_bottomMenu->hubStickFound(numberStickFound, descSerialSticks);
        ui->tab_workout1->setHubStickFound(numberStickFound);

        checkToEnableWindow();
    }




}




//////////////////////////////////////////////////////////////////////////////////////////////
void MainWindow::closeEvent(QCloseEvent *event) {


    qDebug() << "closeEvent";

    //    Qt::WindowFlags flags = Qt::Window;
    //    if (settings->forceWorkoutWindowOnTop)
    //        flags = flags | Qt::WindowStaysOnTopHint;
    //    this->setWindowFlags(flags);


    if (isInsideWorkout) {
        QMessageBox msgBox;
        msgBox.setWindowFlags(msgBox.windowFlags() | Qt::WindowStaysOnTopHint);
        msgBox.setIcon(QMessageBox::Question);
        msgBox.setText(tr("A workout is still active."));
        msgBox.setInformativeText(tr("Please close any active workout before leaving MaximumTrainer."));
        msgBox.setStandardButtons(QMessageBox::Ok);
        msgBox.setDefaultButton(QMessageBox::Ok);
        msgBox.exec();
        event->ignore();
        return;
    }

    // Wait for hub to close
    for (int i=0; i<vecHub.size(); i++) {
        Hub *hub = vecHub.at(i);
        connect(hub, SIGNAL(closeCSMFinished()), this, SLOT(closedCSMFinished()));
    }

    //Stop Hub thread
    this->closeCSM(true);


    //Save Studio User to XML File
    XmlUtil::saveUserStudioFile(vecUserStudio, "");

    //Save filter Field
    ui->tab_workout1->saveFilterFields();

    Q_UNUSED(event);
    saveSettings();
    this->setVisible(false);

    savingWindow.show();

    //Save Settings and Account List workout done to xml file
    XmlUtil::saveLocalSaveFile(account);


    // Put updated account data to server
    replySaveAccount = UserDAO::putAccount(account);
    QObject::connect(replySaveAccount, SIGNAL(finished()), this, SLOT(postDataAccountFinished()) );
    loop.exec(); //dont leave until data uploaded to server


    //Wait for hub to close
    qDebug() << "how many already closed?" << closedCSMFinishedNb << " hub size is " << vecHub.size();
    if (closedCSMFinishedNb != vecHub.size()) {
        qDebug() << "MainWindow, Wait for hub to close..";
        timerCloseCsm.setSingleShot(true);
        QEventLoop loop2;
        timerCloseCsm.start(10000);
        QObject::connect(&timerCloseCsm, SIGNAL(timeout()), &loop2, SLOT(quit()));
        loop2.exec();
    }


    savingWindow.hide();
    qDebug () << "closeEvent Done mainWindow";
}

/////////////////////////////////////////////////////////////////////////////////////////
void MainWindow::postDataAccountFinished() {

    //success, process data
    if (replySaveAccount->error() == QNetworkReply::NoError) {
        qDebug() << "no error postDataAccountFinished!";
        loop.quit();
    }

    // error, retry request
    else {
        if (saveAccountTry > 5) {
            savingWindow.setMessage("Could not save on server");
            qDebug() << "could not save 5 time in a row, leaving!";
            loop.quit();
        }
        else {
            saveAccountTry++;
            qDebug() << "postDataAccountFinished error" << replySaveAccount->errorString();
            replySaveAccount = UserDAO::putAccount(account);
            connect(replySaveAccount, SIGNAL(finished()), this, SLOT(postDataAccountFinished()) );
        }
    }

}

//QMENUBAR
/////////////////////////////////////////////////////////////////////////////////////////
void MainWindow::on_actionExit_triggered()
{
    this->close();
}


//----------------------------------------------------------------------------------------
void MainWindow::on_actionLog_off_Exit_triggered()
{

    QMessageBox msgBox(this);
    msgBox.setIcon(QMessageBox::Question);

    msgBox.setText(tr("This will log you out of MaximumTrainer. You will need to re-enter your password to use MaximumTrainer again."));
    msgBox.setInformativeText(tr("Do you wish to continue?"));

    msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::Cancel);
    msgBox.setDefaultButton(QMessageBox::Yes);
    msgBox.setButtonText(QMessageBox::Yes, tr("Logout"));
    int reply = msgBox.exec();
    if (reply == QMessageBox::Yes) {
        qDebug() << "Yes was clicked";
        Settings *settings = qApp->property("User_Settings").value<Settings*>();
        settings->rememberMyPassword = false;
        settings->lastLoggedKey = "";
        settings->saveGeneralSettings();
        this->close();

    } else { //Cancel
        qDebug() << "Cancel was clicked";
        return;
    }


    /////////////////////


}



//----------------------------------------------------------------------------------------
void MainWindow::on_actionAbout_MT_triggered()
{
    QString nameWithVersion = "MaximumTrainer " + Environnement::getVersion();
    QString copyright = tr("Copyright 2013-2019 Max++ inc. All rights reserved");
    this->setStyleSheet("QMessageBox { messagebox-text-interaction-flags: 5; }");
    QMessageBox::about(this,
                       tr("About MaximumTrainer"),
                       "<b>"+ nameWithVersion + "</b> - " + tr("Build on ")  + Environnement::getDateBuilded() + "<br/>" +
                       copyright + "<br/><hr/>" +
                       tr("Normalized Power®(NP), Training Stress Score®(TSS) and Intensity Factor®(IF) are registered trademarks of Peaksware LLC.") +
                       "<hr/><b>Donation</b><br/>" +
                       "BTC:<br/>3LKr2aZabvCn8yadtfPNmLo5GyGiGft6yw<br/>ETH:<br/>0x51B7D1dDE1b3B4c8bfdb70D4B3E4dA2f8511F8AD");


}
//-----------------------------------------------
void MainWindow::on_actionAbout_Qt_triggered()
{
    QMessageBox::aboutQt(this);



}
//-----------------------------------------------
void MainWindow::on_actionRequest_Help_triggered()
{
    DialogInfoWebView infoAntStick;
    infoAntStick.setUrlWebView(Environnement::getUrlSupport());
    qDebug() << "URL IS : " << Environnement::getUrlSupport();
    infoAntStick.exec();
}
//-----------------------------------------------
void MainWindow::on_actionPreferences_triggered()
{

    dconfig->exec();

}
//-----------------------------------------------
void MainWindow::on_actionWorkout_triggered()
{
    Util::openWorkoutFolder("null");
}
//-----------------------------------------------
void MainWindow::on_actionOpen_Course_Folder_triggered()
{
    Util::openCourseFolder("null");
}
//-----------------------------------------------
void MainWindow::on_actionHistory_triggered()
{
    Util::openHistoryFolder();
}



//------------------------------------------------------
void MainWindow::on_actionCreate_New_triggered()
{


    ui->tabWidget_workout->setCurrentIndex(1);
    ftb->setCurrentIndex(0);
    ui->tab_create->resetWorkout();


}



//------------------------------------------------------
void MainWindow::on_actionOpen_Ride_triggered()
{

    qDebug() << "Doing free ride!";


    QList<Interval> lstInterval;
    Interval interval;
    lstInterval.append(interval);

    Workout freeRide("null", Workout::OPEN_RIDE, lstInterval,
                     tr("Free Ride"), "MaximumTrainer", tr("Go and ride as long as you want"), "-", Workout::T_OTHERS);

    executeWorkout(freeRide);
}


//////////////////////////////////////////////////////////////////////////////////////////////////////////////
void MainWindow::executeWorkout(Workout workout) {

    //wait cursor
    QApplication::setOverrideCursor(Qt::WaitCursor);

    for (int i=0; i<vecUserStudio.size(); i++) {

        UserStudio userStudio = vecUserStudio.at(i);
        qDebug() << "Before Execute__ User Studio power Curve is_WORKOUTDIALOG:" << userStudio.getPowerCurve().getFullName() << userStudio.getPowerCurve().getId() << "test att:" << userStudio.getDisplayName();
    }

    WorkoutDialog w(vecHub, vecStickIdUsed, workout, lstRadio, vecUserStudio);

    connect(&w, SIGNAL(fitFileReady(QString, QString, QString)), this, SLOT(checkToUploadFile(QString,QString,QString)) );
    connect(&w, SIGNAL(ftp_lthr_changed()), this, SLOT(updateZoneInterface()));
    connect(&w, SIGNAL(ftp_lthr_changed()), ui->tab_workout1, SLOT(updateTableViewMetrics()));


    connect(&w, SIGNAL(ftpUserStudioChanged(QVector<UserStudio>)), this, SLOT(updateVecStudio(QVector<UserStudio>)) );


    workoutExecuting();
    w.exec();
    workoutOver();

    ui->webView_achiev->reload();
}


////////////////////////////////////////////////////////////////////////
void MainWindow::on_actionImportCourse_triggered()
{
    //    bool result = ui->tab_edit_course->loadCourseTrigger();

    //    if (result) {
    //        ui->tabWidget_course->setCurrentIndex(1);
    //        ftb->setCurrentIndex(1);
    //    }

}




//-------------------------------------------------------
QString MainWindow::loadPathImport() const {

    QSettings settings;

    settings.beginGroup("Importer");
    QString path = settings.value("loadPath", QDir::homePath() ).toString();
    settings.endGroup();

    return path;

}
//------------------------------------------------------------------
void MainWindow::savePathImport(const QString& filepath) {

    QSettings settings;

    settings.beginGroup("Importer");
    settings.setValue("loadPath", filepath);
    settings.endGroup();
}


//------------------------------------------------------------------------------
void MainWindow::on_actionSingle_Workout_triggered()
{
    QString file = QFileDialog::getOpenFileName(this, tr("Import"),
                                                loadPathImport(),
                                                tr("Workout Files(*.erg *.mrc)"));

    if (file.isEmpty())
        return;

    Workout importedWorkout = ImporterWorkout::importWorkoutFromFile(file, account->FTP);

    savePathImport(file);


    //open workout editor with Workout
    ui->tabWidget_workout->setCurrentIndex(1);
    ftb->setCurrentIndex(0);
    ui->tab_create->editWorkout(importedWorkout);


    ui->widget_bottomMenu->setGeneralMessage(QString(tr("Workout %1 successfully imported")).arg(file), 5000);


}

//------------------------------------------------------------------------------
void MainWindow::on_actionMultiple_Workouts_triggered()
{
    qDebug() << "Multiple Workouts importer";


    QString folder =  QFileDialog::getExistingDirectory(this, tr("Select Folder Containing Workout Files To Import (.erg and .mrc)"),
                                                        loadPathImport());


    if (folder.isEmpty())
        return;

    bool result = ImporterWorkout::batchImportWorkoutFromFolder(folder, account->FTP);

    if (result) {
        //Open Folder in explorer (finder)
        QDesktopServices::openUrl(QUrl("file:///" + folder + "/MT Workouts/"));
    }
}







////////////////////////////////////////////////////////////////////////////////////////////////////
void MainWindow::checkToUploadFile(const QString& filename, const QString& nameOnly, const QString& description) {

    qDebug() << "check to upload Fit file";

    if (account->strava_access_token != "") {

        QString msgUploading = tr("Uploading your activity to Strava...");
        ui->widget_bottomMenu->setGeneralMessage(msgUploading);
        replyStravaUpload = ExtRequest::stravaUploadFile(account->strava_access_token, nameOnly, description,
                                                         true, account->strava_private_upload, "workout", filename);
        connect(replyStravaUpload, SIGNAL(finished()), this, SLOT(slotStravaUploadFinished()) );
    }


    if (account->training_peaks_access_token != "" && account->training_peaks_refresh_token != "") {

        //1- Refresh Token
        //2- Upload File
        nameWorkout = nameOnly;
        descriptionWorkout = description;
        filepathWorkout = filename;

        QString msgUploading = tr("Uploading your activity to TrainingPeaks...");
        ui->widget_bottomMenu->setGeneralMessage(msgUploading);
        replyTrainingPeaksRefreshStatus = ExtRequest::trainingPeaksRefreshToken(account->training_peaks_access_token, account->training_peaks_refresh_token);
        connect(replyTrainingPeaksRefreshStatus, SIGNAL(finished()), this, SLOT(slotTrainingPeaksRefreshFinished()) );
    }

    //SelfLoops
    if (account->selfloops_user.size() > 0 && account->selfloops_pw.size() > 0 ) {

        QString msgUploading = tr("Uploading your activity to SelfLoops...");
        ui->widget_bottomMenu->setGeneralMessage(msgUploading);
        replySelfLoopsUpload = ExtRequest::selfloopsUploadFile(account->selfloops_user, account->selfloops_pw, filename, description);
        connect(replySelfLoopsUpload, SIGNAL(finished()), this, SLOT(slotSelfLoopsUploadFinished()) );
    }


}

////////////////////////////////////////////////////////////////////////////////////////////////////
void MainWindow::slotSelfLoopsUploadFinished()
{
    qDebug() << "slotSelfLoopsUploadFinished";

    QString msgReady = tr("Your activity was successfully uploaded to SelfLoops");
    QString msgPresent = tr("This activity is already present on SelfLoops");
    QString msgEmpty = tr("Your activity is empty, it was not uploaded to SelfLoops");
    QString msgNotUploaded = tr("Your activity was not uploaded to SelfLoops");


    //success, process data
    if (replySelfLoopsUpload->error() == QNetworkReply::NoError) {
        qDebug() << "no error selfLoops!";
        QByteArray arrayData =  replySelfLoopsUpload->readAll();
        QString msgReply(arrayData);
        qDebug() << "data is:" << msgReply;

        if (msgReply.contains("Success", Qt::CaseInsensitive)) {
            ui->widget_bottomMenu->setGeneralMessage(msgReady, 5000);
        }
        else if (msgReply.contains("already", Qt::CaseInsensitive)) {
            ui->widget_bottomMenu->setGeneralMessage(msgPresent, 5000);
        }
        else if (msgReply.contains("empty", Qt::CaseInsensitive)) {
            ui->widget_bottomMenu->setGeneralMessage(msgEmpty, 5000);
        }
        else {
            ui->widget_bottomMenu->setGeneralMessage(msgNotUploaded, 5000);
        }

    }
    else {
        qDebug() << replySelfLoopsUpload->errorString();
        ui->widget_bottomMenu->setGeneralMessage("SelfLoops : " + replySelfLoopsUpload->errorString(), 5000);
    }
    replySelfLoopsUpload->deleteLater();

}


////////////////////////////////////////////////////////////////////////////////////////////////////
void MainWindow::slotTrainingPeaksRefreshFinished()
{

    qDebug() << "slotTrainingPeaksRefreshFinished";

    //success, process data (new access Token)
    if (replyTrainingPeaksRefreshStatus->error() == QNetworkReply::NoError) {
        qDebug() << "no error TP!";
        QByteArray arrayData =  replyTrainingPeaksRefreshStatus->readAll();
        qDebug() << "response is :" << arrayData;
        Util::parseJsonTPObject(QString(arrayData));

        //TODO: post file now here, call TP Rest WS
        replyTrainingPeaksPostFile = ExtRequest::trainingPeaksUploadFile(account->training_peaks_access_token, account->training_peaks_public_upload,
                                                                         nameWorkout, descriptionWorkout, filepathWorkout);
        connect(replyTrainingPeaksPostFile, SIGNAL(finished()), this, SLOT(slotTrainingPeaksUploadFinished()) );
    }
    else {
        qDebug() << replyTrainingPeaksRefreshStatus->errorString();
        ui->widget_bottomMenu->setGeneralMessage("TrainingPeaks : " + replyTrainingPeaksRefreshStatus->errorString(), 5000);
    }
    replyTrainingPeaksRefreshStatus->deleteLater();
}


////////////////////////////////////////////////////////////////////////////////////////////////////
void MainWindow::slotTrainingPeaksUploadFinished()
{
    qDebug() << "slotTrainingPeaksUploadFinished";

    QString msgReady = tr("Your activity was successfully uploaded to TrainingPeaks");
    //    QString msgError = tr("There was an error processing your activity to TrainingPeaks");


    //success, process data
    if (replyTrainingPeaksPostFile->error() == QNetworkReply::NoError) {
        qDebug() << "no error slotTrainingPeaksUploadFinished!";
        QByteArray arrayData =  replyTrainingPeaksPostFile->readAll();
        qDebug() << arrayData;
        ui->widget_bottomMenu->setGeneralMessage(msgReady, 5000);

    }
    else {
        qDebug() << replyTrainingPeaksPostFile->errorString();
        ui->widget_bottomMenu->setGeneralMessage("TrainingPeaks : " + replyTrainingPeaksPostFile->errorString(), 5000);
    }
    replyTrainingPeaksPostFile->deleteLater();



}



////////////////////////////////////////////////////////////////////////////////////////////////////
void MainWindow::slotStravaUploadFinished()
{
    qDebug() << "slotStravaUploadFinished";


    //success, process data
    if (replyStravaUpload->error() == QNetworkReply::NoError) {
        qDebug() << "no error strava!";
        QByteArray arrayData =  replyStravaUpload->readAll();
        stravaUploadID = Util::parseIdJsonStravaUploadObject(QString(arrayData));
        qDebug() << "UPLOAD ID IS:" << stravaUploadID;
        timerCheckUploadStatus = new QTimer(this);
        connect(timerCheckUploadStatus, SIGNAL(timeout()), this, SLOT(slotStravaCheckUploadStatus()) );
        timerCheckUploadStatus->start(3000);
    }
    else {
        qDebug() << replyStravaUpload->errorString();
        ui->widget_bottomMenu->setGeneralMessage("Strava : " + replyStravaUpload->errorString(), 5000);
    }
    replyStravaUpload->deleteLater();



}

////////////////////////////////////////////////////////////////////////////////////////////////////
void MainWindow::slotStravaCheckUploadStatus() {

    qDebug() << "slotStravaCheckUploadStatus";

    replyStravaUploadStatus = ExtRequest::stravaCheckUploadStatus(account->strava_access_token, stravaUploadID );
    connect(replyStravaUploadStatus, SIGNAL(finished()), this, SLOT(slotStravaUploadStatusFinished()) );
}


////////////////////////////////////////////////////////////////////////////////////////////////////
void MainWindow::slotStravaUploadStatusFinished() {

    qDebug() << "slotStravaUploadStatusFinished";


    QString msgReady = tr("Your activity was successfully uploaded to Strava");
    QString msgError = tr("There was an error processing your activity to Strava");


    //success, process data
    if (replyStravaUploadStatus->error() == QNetworkReply::NoError) {

        QByteArray arrayData =  replyStravaUploadStatus->readAll();
        int codeReturn = Util::parseStravaUploadStatus(QString(arrayData));
        // -1 = Not normal, stop checking for status...
        //  0 = Completed (Ready)
        //  1 = Still In process
        //  2 = Error
        if (codeReturn == -1) {
            timerCheckUploadStatus->stop();
            ui->widget_bottomMenu->removeGeneralMessage();
        }
        else if (codeReturn == 0) {
            timerCheckUploadStatus->stop();
            ui->widget_bottomMenu->setGeneralMessage(msgReady, 5000);
        }
        else if (codeReturn == 2) {
            timerCheckUploadStatus->stop();
            ui->widget_bottomMenu->setGeneralMessage(msgError, 5000);

        }

    }
    else {
        timerCheckUploadStatus->stop();
        qDebug() << replyStravaUploadStatus->errorString();
        ui->widget_bottomMenu->setGeneralMessage("Strava : " + replyStravaUploadStatus->errorString(), 5000);
    }
    replyStravaUploadStatus->deleteLater();
}









/// -------------------------------------Hub Stick ------------------------------------------------------------------
void MainWindow::closeCSM(bool closeShop) {
    qDebug() << "SettingsObject::closeCSM()";
    emit signal_hubCloseChannelCSM(closeShop);
}
//----------------------------------------
void MainWindow::closedCSMFinished() {

    qDebug() << "closedCSMFinished" << closedCSMFinishedNb;
    closedCSMFinishedNb++;

    qDebug() << "how many already closed?" << closedCSMFinishedNb << " hub size is " << vecHub.size();
    if (closedCSMFinishedNb == vecHub.size()) {
        qDebug() << "force timerToShoot timeout";
        timerCloseCsm.setInterval(0);
    }
}

///////////////////////////////////////////////////////////////
void MainWindow::setPmForCadence(bool usedFor) {

    qDebug() << "setPmForCadence" << usedFor;
    account->use_pm_for_cadence = usedFor;
}
///////////////////////////////////////////////////////////////
void MainWindow::setPmForSpeed(bool usedFor) {

    qDebug() << "setPmForSpeed" << usedFor;
    account->use_pm_for_speed = usedFor;
}
///////////////////////////////////////////////////////////////

//const int hrDeviceType = 120;
//const int speedCadDeviceType = 121; //not used here
//const int cadDeviceType= 122;
//const int speedDeviceType = 123;
//const int powerDeviceType = 11;
//const int fecDeviceType = 17;
//const int oxyDeviceType = 31;
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void MainWindow::searchSensor(int typeSensor, bool fromStudioPage) {

    qDebug() << "searchSensor" << typeSensor <<  "fromStudioPage?" << fromStudioPage;

    //reset counter (counter increase when a hub send a response to the pairing)
    pairingResponseAlreadySent = false;
    pairingResponseNumber = 0;
    lstDevicePaired.clear();
    lstTypeDevicePaired.clear();


    QString noAntStickDetected = tr("No Ant Stick detected!\\nPlease insert one or more and restart MaximumTrainer.");
    QString jsCode = "alert('" + noAntStickDetected + "');";

    QList<int> lstBidon;

    if (numberStickFound == 0 && fromStudioPage) {
        qDebug() << "NO STICK STUDIO PAGE, Show alert";
        ui->webView_studio->page()->runJavaScript(jsCode);
        sendDataToSettingsOrStudioPage(typeSensor, 0, lstBidon, lstBidon, true);
    }
    else if (numberStickFound == 0) {
        qDebug() << "NO STICK SEttings PAGE, Show alert";
        ui->webView_settings->page()->runJavaScript(jsCode);
        sendDataToSettingsOrStudioPage(typeSensor, 0, lstBidon, lstBidon, false);
    }
    else {
        qDebug() << "Start a new Pairing";
        emit signal_searchForSensor(typeSensor, account->stop_pairing_on_found, account->nb_sec_pairing, fromStudioPage);
    }


}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void MainWindow::updateTrainerCurve(int trainer_id, QString companyName, QString trainerName,
                                    double coef0, double coef1, double coef2, double coef3, int formulaInCode) {

    qDebug() << "UPDATE_TRAINER_CURVE INFO" << formulaInCode;

    account->powerCurve.setId(trainer_id);
    account->powerCurve.setName(companyName, trainerName);
    account->powerCurve.setCoefs(coef0, coef1, coef2, coef3);
    account->powerCurve.setFormulaInCode(formulaInCode);


    qDebug() << "ok trainerID is now:" << account->powerCurve.getId();
}

