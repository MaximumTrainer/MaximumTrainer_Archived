#include "dialogmainwindowconfig.h"
#include "ui_dialogmainwindowconfig.h"

#include <QDebug>
#include <QFileDialog>
#include <QMessageBox>

#include "util.h"
#include "dialoginfowebview.h"
#include "environnement.h"
#include "extrequest.h"


DialogMainWindowConfig::~DialogMainWindowConfig()
{
    delete ui;
}




DialogMainWindowConfig::DialogMainWindowConfig(QWidget *parent) : QDialog(parent), ui(new Ui::DialogMainWindowConfig)
{
    stravaConnectView = new DialogInfoWebView(this);
    stravaConnectView->setTitle(tr("Connect MaximumTrainer with your Strava account"));
    stravaConnectView->setUsedForStrava(true);
    connect(stravaConnectView, SIGNAL(stravaLinked(bool)), this, SLOT(stravaLinked(bool)) );
    stravaConnectView->setUrlWebView(Environnement::getURLStravaAuthorize());
    stravaConnectViewAlreadyUsed = false;

    ui->setupUi(this);


    this->settings = qApp->property("User_Settings").value<Settings*>();
    this->account = qApp->property("Account").value<Account*>();


    /// List widgets
    ui->listWidget_settings->setIconSize(QSize(24, 24));

    QListWidgetItem *item1 = new QListWidgetItem(QIcon(":/image/icon/general"), tr("General"), ui->listWidget_settings);
    QListWidgetItem *item2 = new QListWidgetItem(QIcon(":/image/icon/ant"), tr("ANT+"), ui->listWidget_settings);
    QListWidgetItem *item3 = new QListWidgetItem(QIcon(":/image/icon/folder"), tr("Folders"), ui->listWidget_settings);
    QListWidgetItem *item4 = new QListWidgetItem(QIcon(":/image/icon/upload"), tr("Auto Upload"), ui->listWidget_settings);
    item1->setSizeHint(QSize(35,35));
    item2->setSizeHint(QSize(35,35));
    item3->setSizeHint(QSize(35,35));
    item4->setSizeHint(QSize(35,35));

    ui->listWidget_settings->addItem(item1);
    ui->listWidget_settings->addItem(item2);
    ui->listWidget_settings->addItem(item3);
    ui->listWidget_settings->addItem(item4);


    connect(ui->listWidget_settings, SIGNAL(currentRowChanged(int)), this, SLOT(currentListViewSelectionChanged(int)) );

    ui->listWidget_settings->setCurrentRow(0);

    initUI();


    ///Keyword: RemoveCOURSE Temp
    ui->lineEdit_courseDir->setVisible(false);
    ui->pushButton_browseCourseDir->setVisible(false);
    ui->label_courseTxt->setVisible(false);

    ui->checkBox_trainingPeaksPrivate->setVisible(false);
}



////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void DialogMainWindowConfig::initUI() {

    // Strava
    ui->label_stravaUnlink->setText(tr("Unlink"));
    ui->label_stravaUnlink->setStyleSheet("background-color : transparent; color : blue; text-decoration: underline;");
    connect(ui->label_stravaUnlink, SIGNAL(clicked(bool)), this, SLOT(unlinkStravaClicked()) );

    if (account->strava_access_token != "") {
        stravaLinked(true);
    }
    else {
        stravaLinked(false);
    }

    // TrainingPeaks
    ui->label_TrainingPeaksUnlink->setText(tr("Unlink"));
    ui->label_TrainingPeaksUnlink->setStyleSheet("background-color : transparent; color : blue; text-decoration: underline;");
    connect(ui->label_TrainingPeaksUnlink, SIGNAL(clicked(bool)), this, SLOT(unlinkTrainingPeaksClicked()) );

    if (account->training_peaks_refresh_token != "") {
        trainingPeaksLinked(true);
    }
    else {
        trainingPeaksLinked(false);
    }

    ui->checkBox_stravaPrivate->setChecked(account->strava_private_upload);
    ui->checkBox_trainingPeaksPrivate->setChecked(!account->training_peaks_public_upload);
    ui->lineEdit_historyDir->setText(Util::getSystemPathHistory());
    ui->lineEdit_workoutDir->setText(Util::getSystemPathWorkout());
    ui->lineEdit_courseDir->setText(Util::getSystemPathCourse());
    ui->lineEdit_historyDir->setReadOnly(true);
    ui->lineEdit_workoutDir->setReadOnly(true);
    ui->lineEdit_courseDir->setReadOnly(true);

    if (account->distance_in_km)
        ui->comboBox_distance->setCurrentIndex(0);
    else //MPH
        ui->comboBox_distance->setCurrentIndex(1);

    ui->checkBox_forceOnTop->setChecked(account->force_workout_window_on_top);
    ui->checkBox_controlTrainer->setChecked(account->control_trainer_resistance);

    ui->checkBox_stopPairingOnFound->setChecked(account->stop_pairing_on_found);
    ui->checkBox_pairNbOfSeconds->setChecked(!account->stop_pairing_on_found);
    ui->comboBox_timePairingSec->setCurrentText(QString::number(account->nb_sec_pairing));


    ui->lineEdit_userSelfloops->setText(account->selfloops_user);
    ui->lineEdit_pwSelfloops->setText(account->selfloops_pw);

}



///////////////////////////////////////////////////////////////////////////////////////////////
void DialogMainWindowConfig::stravaLinked(bool linked) {

    qDebug() << "strava linked!";

    if (linked) {
        ui->label_connectStrava->setCursor(Qt::ArrowCursor);
        ui->label_connectStrava->setVisible(false);

        qDebug() << "before disc linked end!";
        disconnect(ui->label_connectStrava, SIGNAL(clicked(bool)), this, SLOT(stravaLabelClicked()) );

        ui->label_stravaUnlink->setVisible(true);
        ui->label_stravaUnlink->setCursor(Qt::PointingHandCursor);
        ui->label_stravaUnlink->fadeIn(1000);
        ui->checkBox_stravaPrivate->setVisible(true);

        qDebug() << "after disc linked end!";
    }
    else {
        ui->label_connectStrava->setStyleSheet("image: url(:/image/icon/strava);");
        ui->label_connectStrava->setCursor(Qt::PointingHandCursor);
        ui->label_connectStrava->setVisible(true);
        connect(ui->label_connectStrava, SIGNAL(clicked(bool)), this, SLOT(stravaLabelClicked()) );

        ui->label_stravaUnlink->setVisible(false);
        ui->checkBox_stravaPrivate->setVisible(false);
    }
    ui->label_connectStrava->fadeIn(1000);

    qDebug() << "strava linked end!";

    stravaConnectView->accept();
}


//---------------------------------------------------------------------------------------------
void DialogMainWindowConfig::trainingPeaksLinked(bool linked) {

    qDebug() << "TP linked!";

    if (linked) {
        qDebug() << "Access token for TP is: " << account->training_peaks_access_token;
        qDebug() << "refresh token for TP is: " << account->training_peaks_refresh_token;

        ui->label_connectTrainingPeaks->setCursor(Qt::ArrowCursor);
        ui->label_connectTrainingPeaks->setVisible(false);
        disconnect(ui->label_connectTrainingPeaks, SIGNAL(clicked(bool)), this, SLOT(trainingPeaksLabelClicked()) );

        ui->label_TrainingPeaksUnlink->setVisible(true);
        ui->label_TrainingPeaksUnlink->setCursor(Qt::PointingHandCursor);
        ui->label_TrainingPeaksUnlink->fadeIn(1000);
        //        ui->checkBox_trainingPeaksPrivate->setVisible(true);
    }
    else {
        ui->label_connectTrainingPeaks->setStyleSheet("image: url(:/image/icon/trainingpeaks);");
        ui->label_connectTrainingPeaks->setCursor(Qt::PointingHandCursor);
        ui->label_connectTrainingPeaks->setVisible(true);
        connect(ui->label_connectTrainingPeaks, SIGNAL(clicked(bool)), this, SLOT(trainingPeaksLabelClicked()) );

        ui->label_TrainingPeaksUnlink->setVisible(false);
        //        ui->checkBox_trainingPeaksPrivate->setVisible(false);
    }
    ui->label_connectTrainingPeaks->fadeIn(1000);
}


//---------------------------------------------------------------------------------------------
void DialogMainWindowConfig::stravaLabelClicked() {

    qDebug() << "stravaLabelClicked1";

    if (stravaConnectViewAlreadyUsed) {
            stravaConnectView = new DialogInfoWebView(this);
            stravaConnectView->setTitle(tr("Connect MaximumTrainer with your Strava account"));
            stravaConnectView->setUsedForStrava(true);
            connect(stravaConnectView, SIGNAL(stravaLinked(bool)), this, SLOT(stravaLinked(bool)) );
            stravaConnectView->setUrlWebView(Environnement::getURLStravaAuthorize());
    }
    stravaConnectView->exec();

    stravaConnectViewAlreadyUsed = true;
    qDebug() << "stravaLabelClicked3 done";

}

//---------------------------------------------------------------------------------------------
void DialogMainWindowConfig::trainingPeaksLabelClicked() {


    DialogInfoWebView infoAntStick;

    infoAntStick.setTitle(tr("Connect MaximumTrainer with your TrainingPeaks account"));
    infoAntStick.setUsedForTrainingPeaks(true);
    connect(&infoAntStick, SIGNAL(trainingPeaksLinked(bool)), this, SLOT(trainingPeaksLinked(bool)) );
    infoAntStick.setUrlWebView(Environnement::getURLTrainingPeaksAuthorize());

    qDebug() << "TP URL IS : " << Environnement::getURLTrainingPeaksAuthorize();
    infoAntStick.exec();

}


//---------------------------------------------------------------------------------------------
void DialogMainWindowConfig::unlinkStravaClicked() {


    qDebug() << "unlinkStravaClicked";

    replyStravaDeauthorization = ExtRequest::stravaDeauthorization(account->strava_access_token);
    connect(replyStravaDeauthorization, SIGNAL(finished()), this, SLOT(stravaUnlinkFinished()) );
}

//---------------------------------------------------------------------------------------------
void DialogMainWindowConfig::unlinkTrainingPeaksClicked() {

    account->training_peaks_access_token = "";
    account->training_peaks_refresh_token = "";
    trainingPeaksLinked(false);

}



//---------------------------------------------------------------------------------------------
void DialogMainWindowConfig::stravaUnlinkFinished() {

    qDebug() << "stravaUnlinkFinished";

    //success, process data
    if (replyStravaDeauthorization->error() == QNetworkReply::NoError) {
        qDebug() << "UNlink done success with Strava!";
        account->strava_access_token = "";
        stravaLinked(false);
    }
    else {
        qDebug() << "Error with stravaUnlink" << replyStravaDeauthorization->errorString();
        account->strava_access_token = "";
        stravaLinked(false);
    }
    replyStravaDeauthorization->deleteLater();


}




//---------------------------------------------------------------------------------------------
void DialogMainWindowConfig::currentListViewSelectionChanged(int section) {

    qDebug() << "changed!" << section;

    ui->stackedWidget->setCurrentIndex(section);

    //    if (section == 0) {
    //        ui->label_headerSettings->setText(tr("Folders"));
    //    }
    //    if (section == 1) {
    //        ui->label_headerSettings->setText(tr("Units"));
    //    }
    //    else {
    //        ui->label_headerSettings->setText(tr("-"));
    //    }

}


//---------------------------------------------------------------------------------------------
void DialogMainWindowConfig::on_pushButton_browseWorkoutDir_clicked()
{
    QString path = QFileDialog::getExistingDirectory(this, tr("Select Workout Folder"), Util::getSystemPathWorkout(), QFileDialog::ShowDirsOnly);
    if (path == "")
        return;

    if (Util::checkFolderPathIsValidForWrite(path)) {
        ui->lineEdit_workoutDir->setText(path);
    }
    else {
        QMessageBox msgBox(this);
        msgBox.setIcon(QMessageBox::Warning);
        msgBox.setText(tr("The specified folder could not be used to write files in"));
        msgBox.setStandardButtons(QMessageBox::Close);
        msgBox.exec();
    }

}
//----------------------------------------------------------------------------------------
void DialogMainWindowConfig::on_pushButton_browseHistoryDir_clicked()
{
    QString path = QFileDialog::getExistingDirectory(this, tr("Select History Folder"), Util::getSystemPathHistory(), QFileDialog::ShowDirsOnly);
    if (path == "")
        return;

    if (Util::checkFolderPathIsValidForWrite(path)) {
        ui->lineEdit_historyDir->setText(path);
    }
    else {
        QMessageBox msgBox(this);
        msgBox.setIcon(QMessageBox::Warning);
        msgBox.setText(tr("The specified folder could not be used to write files in"));
        msgBox.setStandardButtons(QMessageBox::Close);
        msgBox.exec();
    }
}


//----------------------------------------------------------------------------------------
void DialogMainWindowConfig::on_pushButton_browseCourseDir_clicked()
{
    QString path = QFileDialog::getExistingDirectory(this, tr("Select Course Folder"), Util::getSystemPathCourse(), QFileDialog::ShowDirsOnly);
    if (path == "")
        return;

    if (Util::checkFolderPathIsValidForWrite(path)) {
        ui->lineEdit_courseDir->setText(path);
    }
    else {
        QMessageBox msgBox(this);
        msgBox.setIcon(QMessageBox::Warning);
        msgBox.setText(tr("The specified folder could not be used to write files in"));
        msgBox.setStandardButtons(QMessageBox::Close);
        msgBox.exec();
    }
}

///////////////////////////////////////////////////////////////////////
void DialogMainWindowConfig::reject() {

    qDebug() << "rejected, put back settings value!";

    initUI();
    QDialog::reject();
}


//---------------------------------------------------------------------------
void DialogMainWindowConfig::accept() {
    qDebug() << "ACCEPT, save settings";

    account->strava_private_upload = ui->checkBox_stravaPrivate->isChecked();
    account->training_peaks_public_upload = !ui->checkBox_trainingPeaksPrivate->isChecked();

    //Folder changed
    if (settings->workoutFolder != ui->lineEdit_workoutDir->text()) {
        settings->workoutFolder = ui->lineEdit_workoutDir->text();
        emit folderWorkoutChanged();
    }

    //Folder changed
    if (settings->courseFolder != ui->lineEdit_courseDir->text()) {
        settings->courseFolder = ui->lineEdit_courseDir->text();
        emit folderCourseChanged();
    }

    settings->historyFolder = ui->lineEdit_historyDir->text();
    account->force_workout_window_on_top = ui->checkBox_forceOnTop->isChecked();

    account->control_trainer_resistance = ui->checkBox_controlTrainer->isChecked();
    account->stop_pairing_on_found = ui->checkBox_stopPairingOnFound->isChecked();
    account->nb_sec_pairing = ui->comboBox_timePairingSec->currentText().toInt();




    if (ui->comboBox_distance->currentIndex() == 0)
        account->distance_in_km = true;
    else //MPH
        account->distance_in_km = false;

    account->selfloops_user = ui->lineEdit_userSelfloops->text();
    account->selfloops_pw = ui->lineEdit_pwSelfloops->text();

    qDebug() << "user Selfloop is" << account->selfloops_user << "pw is:" << account->selfloops_pw;


    settings->saveGeneralSettings();
    QDialog::accept();
}


