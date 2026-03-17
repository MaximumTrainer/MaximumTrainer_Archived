#ifndef DIALOGMAINWINDOWCONFIG_H
#define DIALOGMAINWINDOWCONFIG_H

#include <QDialog>
#include <QNetworkReply>

#include "settings.h"
#include "account.h"
#include "dialoginfowebview.h"
#include "intervalsicuservice.h"


namespace Ui {
class DialogMainWindowConfig;
}

class DialogMainWindowConfig : public QDialog
{
    Q_OBJECT

public:
    explicit DialogMainWindowConfig(QWidget *parent = 0);
    ~DialogMainWindowConfig();

    void accept();
    void reject();



signals:
    void folderWorkoutChanged();
    void folderCourseChanged();
    /// Emitted when Intervals.icu credentials are saved so the tab can refresh.
    void intervalsIcuCredentialsChanged();


public slots:
    void stravaLinked(bool);
    void trainingPeaksLinked(bool);
    /// Update the Intervals.icu section visibility based on network state.
    /// When \a isOnline is false the groupBox_intervals is hidden so the
    /// user cannot attempt to test credentials that will time out.
    void setOnlineMode(bool isOnline);


private slots:
    void currentListViewSelectionChanged(int section);
    void on_pushButton_browseWorkoutDir_clicked();
    void on_pushButton_browseHistoryDir_clicked();
    void on_pushButton_browseCourseDir_clicked();

    void stravaLabelClicked();
    void unlinkStravaClicked();
    void stravaUnlinkFinished();

    void trainingPeaksLabelClicked();
    void unlinkTrainingPeaksClicked();

    void onTestIntervalsConnectionClicked();
    void onTestIntervalsConnectionFinished();


private:
    void initUI();


private:
    Ui::DialogMainWindowConfig *ui;

    Settings *settings;
    Account *account;

    QNetworkReply *replyStravaDeauthorization;
    QNetworkReply *replyTPDeauthorization;

    QNetworkReply *replyIntervalsTest = nullptr;
    IntervalsIcuService *m_intervalsService = nullptr;

    DialogInfoWebView *stravaConnectView;
    bool stravaConnectViewAlreadyUsed;

};

#endif // DIALOGMAINWINDOWCONFIG_H
