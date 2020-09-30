#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTranslator>
#include <QNetworkReply>
#include <QWebEngineView>
#include <QThread>

#include "zoneobject.h"
#include "planobject.h"
#include "fancytabbar.h"
#include "radio.h"
#include "workout.h"
#include "settings.h"
#include "dialogmainwindowconfig.h"
#include "savingwindow.h"
#include "userstudio.h"
#include "myconstants.h"
#include "hub.h"


namespace Ui {
class MainWindow;
}



class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();
    void closeEvent(QCloseEvent *event);



signals :
    void isExpanded(bool isExpanded);
    void ftpAndTabProfileChanged();


    // -------------  Ant Stick--------
    void signal_hubInitUsbStick(int stickNumber);
    void signal_hubCloseChannelCSM(bool closeShop);

    void signal_stopPairing();
    void signal_searchForSensor(int deviceType, bool, int, bool);
    //-------------------------------------





public slots:

    void downloadRequested(QWebEngineDownloadItem*);

    void executeWorkout(Workout workout);

    //Coming from Studio QWebView ----------
    void enableStudioMode(bool enable);
    void setNumberUserStudio(int numberUser);
    void loadConfigStudio();
    void saveConfigStudio();
    void updateVecStudio(QVector<UserStudio>);
    void fillStudioPage();
    void fillSettingPage();
    void companyLoadedForUser(int riderID);
    void setPowerCurveForUser(int riderID, int company_id, int trainer_id, QString companyName, QString trainerName,
                              double coef0, double coef1, double coef2, double coef3, int formulaInCode);
    void disablePowerCurveForUser(int riderID);
    void updateFieldForUser(int riderID, int fieldNumber, QVariant value);



    void updateZoneInterface();
    void leftMenuChanged(int tabSelected);

    void goToWorkoutPlanFilter(const QString& plan);
    void goToWorkoutNameFilter();
    void exportWorkoutToPdf(const Workout& workout);


    void showWorkoutList();
    void showWorkoutCreator();

    void setFlagFtpChanged();



    // Hub --------------------------
    void closeCSM(bool closeShop);
    void closedCSMFinished();

    void setPmForCadence(bool usedFor);
    void setPmForSpeed(bool usedFor);
    void searchSensor(int typeSensor, bool fromStudioPage);

    void updateTrainerCurve(int trainer_id, QString companyName, QString trainerName,
                            double coef0, double coef1, double coef2, double coef3, int formulaInCode);
    // Coming from Hub
    void setStickFound(bool found, QString desc, int stickNumber);
    void setSensorFound(int,int,QList<int>,QList<int>,bool);
    //-------------------------------------------------------------------



    void workoutExecuting();
    void workoutOver();

    void checkToUploadFile(const QString& filename, const QString& nameOnly, const QString& description);

    //tempo
    void postDataAccountFinished();



private slots:
    void slotFinishedGetRadio();


    void on_actionAbout_MT_triggered();
    void on_actionAbout_Qt_triggered();
    void on_actionRequest_Help_triggered();
    void on_actionPreferences_triggered();
    void on_actionWorkout_triggered();
    void on_actionOpen_Course_Folder_triggered();
    void on_actionHistory_triggered();


    void on_actionCreate_New_triggered();

    void on_actionOpen_Ride_triggered();
    void on_actionExit_triggered();
    void on_actionLog_off_Exit_triggered();


    void on_actionSingle_Workout_triggered();
    void on_actionMultiple_Workouts_triggered();

    //-Strava
    void slotStravaUploadFinished();
    void slotStravaCheckUploadStatus();
    void slotStravaUploadStatusFinished();
    //- TrainingPeaks
    void slotTrainingPeaksRefreshFinished();
    void slotTrainingPeaksUploadFinished();
    //-SelfLoops
    void slotSelfLoopsUploadFinished();

    void on_actionImportCourse_triggered();

    void createWebChannelPlan();
    void createWebChannelZone();
    void createWebChannelSettings();
    void createWebChannelStudio();

private:
    void loadSettings();
    void saveSettings();

    void savePathImport(const QString& filepath);
    QString loadPathImport() const;


    void checkToEnableWindow();
    void startHub();
    void sendDataToSettingsOrStudioPage(int deviceType, int numberDeviceFound, QList<int> lstDevicePairedr, QList<int> lstTypeDevicePairedr, bool fromStudioPage);

private:
    QVector<UserStudio> vecUserStudio;

    //--------- Hub --------------
    int numberInitStickDone;
    int numberStickFound;
    QString descSerialSticks; //lst of Serial to show on toolip
    bool hubStickInitFinished;
    QVector<Hub*> vecHub; //One thread per ANT Stick
    QVector<QThread*> vecThread;
    QVector<int> vecStickIdUsed;  //To keep track of Hub with an active stick
    int closedCSMFinishedNb;
    QTimer timerCloseCsm;

    //pairing
    QList<int> lstDevicePaired;
    QList<int> lstTypeDevicePaired;
    int pairingResponseNumber;
    bool pairingResponseAlreadySent;
    //-----------------------



    Ui::MainWindow *ui;
    DialogMainWindowConfig *dconfig;

    bool replyRadioDone;
    QNetworkReply *replyRadio;
    QList<Radio> lstRadio;

    ZoneObject *zoneObject;
    PlanObject *planObject;

    QEventLoop loop;
    QNetworkReply *replySaveAccount;
    int saveAccountTry;
    SavingWindow savingWindow;

    //Strava
    QNetworkReply *replyStravaUpload;
    QNetworkReply *replyStravaUploadStatus;
    int stravaUploadID;
    QTimer *timerCheckUploadStatus;
    //TrainingPeaks
    QString nameWorkout;
    QString descriptionWorkout;
    QString filepathWorkout;
    QNetworkReply *replyTrainingPeaksRefreshStatus;
    QNetworkReply *replyTrainingPeaksPostFile;
    //SelfLoops
    QNetworkReply *replySelfLoopsUpload;


    Settings *settings;
    Account *account;
    FancyTabBar *ftb;

    int currentIndexLeftMenu;
    bool ftpChanged;
    bool isInsideWorkout;

    QString lastWorkoutNameDownloaded;

};

#endif // MAINWINDOW_H
