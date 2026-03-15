#ifndef DIALOGLOGIN_H
#define DIALOGLOGIN_H

#include <QDialog>
#include <QLabel>
#include <QMovie>
#include <QNetworkReply>
#include <QTimer>


#include <QtWebEngineWidgets/QWebEngineView>

#include "account.h"
#include "settings.h"


namespace Ui {
class DialogLogin;
}

class DialogLogin : public QDialog
{
    Q_OBJECT

public:
    explicit DialogLogin(QWidget *parent = 0);
    //    ~DialogLogin();


    void changeEvent(QEvent *event);


    bool getGotUpdate() {
        return this->gotUpdateDialog;
    }




public slots:
    //void provideAuthentication(QNetworkReply*,QAuthenticator*);

    void clearLstUsername();




private slots:
    void showLoadingProgress(int prog);


    void loginLoaded(bool ok);

    void slotFinishedGoogle();
    void slotFinishedGetVersion();
    void onVersionTimeout();
    void onGoogleTimeout();



    void on_comboBox_language_currentIndexChanged(int index);
    void on_checkBox_autoLogin_clicked(bool checked);
    void on_checkBox_workOffline_clicked(bool checked);
    void on_pushButton_startOffline_clicked();


private:
    void loginOffline();

    Ui::DialogLogin *ui;
    QTranslator     m_translator;   /**< contains the translations for this application */
    QString         m_currLang;     /**< contains the currently loaded language */


    Account *account;
    Settings *settings;
    QString last_ip_tmp;
    bool firstLogin;


    QMovie* movie;  //loading gif

    QNetworkReply *replyGoogle;
    QNetworkReply *replyVersion;
    QNetworkReply *replyGetAccount;
    QTimer        *m_versionTimeout;
    QTimer        *m_googleTimeout;

    bool gotUpdateDialog;



};

#endif // DIALOGLOGIN_H
