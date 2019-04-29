#ifndef DIALOGMAINWINDOWCONFIG_H
#define DIALOGMAINWINDOWCONFIG_H

#include <QDialog>
#include <QNetworkReply>

#include "settings.h"
#include "account.h"
#include "dialoginfowebview.h"


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


public slots:
    void stravaLinked(bool);
    void trainingPeaksLinked(bool);


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


private:
    void initUI();


private:
    Ui::DialogMainWindowConfig *ui;

    Settings *settings;
    Account *account;

    QNetworkReply *replyStravaDeauthorization;
    QNetworkReply *replyTPDeauthorization;

    DialogInfoWebView *stravaConnectView;
    bool stravaConnectViewAlreadyUsed;

};

#endif // DIALOGMAINWINDOWCONFIG_H
