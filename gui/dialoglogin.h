#ifndef DIALOGLOGIN_H
#define DIALOGLOGIN_H

#include <QDialog>
#include <QLabel>
#include <QMovie>
#include <QNetworkReply>


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



    void on_comboBox_language_currentIndexChanged(int index);
    void on_checkBox_autoLogin_clicked(bool checked);


private:
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

    bool gotUpdateDialog;



};

#endif // DIALOGLOGIN_H
