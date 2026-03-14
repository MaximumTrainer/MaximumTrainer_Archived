#include "dialoglogin.h"
#include "ui_dialoglogin.h"

#include <QDebug>
#include <QMessageBox>
#include <QKeyEvent>
#include <QDesktopServices>

#include "logger.h"



#include "updatedialog.h"
#include "userdao.h"
#include "versiondao.h"
#include "extrequest.h"
#include "environnement.h"
#include "util.h"
#include "simplecrypt.h"
#include "xmlutil.h"
#include "userdao.h"
#include "myqwebenginepage.h"




void DialogLogin::changeEvent(QEvent *event) {

    if (event->type() == QEvent::LanguageChange)
        ui->label_version->setText(Environnement::getVersion() );

}

//////////////////////////////////////////////////////////////////////////////////////////////////////////
DialogLogin::DialogLogin(QWidget *parent) : QDialog(parent), ui(new Ui::DialogLogin) {

    ui->setupUi(this);

    Qt::WindowFlags flags(Qt::WindowTitleHint | Qt::WindowCloseButtonHint);
    this->setWindowFlags(flags);

    // External links
    QStringList lstExternal = {"signup", "forgotpw" };
    MyQWebEnginePage *myPage = new MyQWebEnginePage(ui->webView_login);
    myPage->setExternalList(lstExternal);
    ui->webView_login->setPage(myPage);


    gotUpdateDialog = false;
    firstLogin = true;


    ///Set loading icon (wait for login page to show)
    QMovie *movie = new QMovie(":/image/icon/loading", QByteArray(), this);
    movie->setPaused(false);
    ui->label_loading->setMovie(movie);
    movie->setSpeed(150);
    movie->start();
    /////////////////////////////////////////////////////////////////



    this->account = qApp->property("Account").value<Account*>();
    this->settings = qApp->property("User_Settings").value<Settings*>();


    ///Remember me, language, version
    ui->checkBox_autoLogin->setChecked(settings->rememberMyPassword);
    ui->comboBox_language->setCurrentIndex(settings->language_index);
    ui->label_version->setText(Environnement::getVersion() );



    // --------------------------------------------
    /// CHECK FOR INTERNET CONNECTION
    ui->label_process->setText(tr("Connecting to server..."));
    replyGoogle = ExtRequest::checkGoogleConnection();
    connect(replyGoogle, SIGNAL(finished()), this, SLOT(slotFinishedGoogle()) );



    /// Check if new version APP available
    replyVersion = VersionDAO::getVersion();
    connect(replyVersion, SIGNAL(finished()), this, SLOT(slotFinishedGetVersion()));

    // Safety-net: if the version check doesn't complete within 10 s (e.g. network
    // latency, firewall, TLS failure) abort the request and proceed to login so the
    // app never hangs on this dialog.
    m_versionTimeout = new QTimer(this);
    m_versionTimeout->setSingleShot(true);
    connect(m_versionTimeout, &QTimer::timeout, this, &DialogLogin::onVersionTimeout);
    m_versionTimeout->start(10000);
    // --------------------------------------------

    connect(ui->webView_login, SIGNAL(loadProgress(int)), this, SLOT(showLoadingProgress(int)));

    ui->label_loading->setVisible(false);
    ui->webView_login->setVisible(false);
    ui->widget_bottom->setVisible(false);
    ui->widget_loading->setVisible(true);

}




/////////////////////////////////////////////////////////////////////////////////////////////
void DialogLogin::clearLstUsername() {

    qDebug() << "CLEAR LST!";


    for (int i=0; i<settings->lstUsername.size(); i++) {
        settings->lstUsername.replace(i, "");
    }
    settings->saveGeneralSettings();

}



/////////////////////////////////////////////////////////////////////////////////////////////
void DialogLogin::showLoadingProgress(int prog) {
    qDebug() << "show loading icon for this url" << ui->webView_login->url() << "prog" << prog;

    if (prog != 100)
        ui->label_loading->setVisible(true);
    else
        ui->label_loading->setVisible(false);

}



/////////////////////////////////////////////////////////////////////////////////////////////
void DialogLogin::loginLoaded(bool ok){
    if (!ok)
        return;

    ui->widget_loading->setVisible(false);
    ui->label_loading->setVisible(false);
    ui->webView_login->setVisible(true);
    ui->widget_bottom->setVisible(true);

    qDebug() << "got here login loaded!" << ui->webView_login->url().toString();
    if (ui->webView_login->url().toDisplayString().contains("insideMT#"))  {
        qDebug() << "ok ignore this!";
        return;
    }

    /// ------------------------------- Login sucess! ---------------------------------------
    if (ui->webView_login->url().toDisplayString().contains("/account_rest/"))  {
        qDebug() << "Parse Json Object Account";
        ///  ------------------ Parse JSON object  ----------------------------

        //        this->setVisible(false);

        ui->webView_login->page()->toPlainText([=](const QString &response){
            qDebug() << "HERE IS THE RESP" << response;

            // Add data inside global variable "Account" and "Settings"
            Util::parseJsonObjectAccount(response);

            // Add local settings data (Folder, etc.) to "Account" object
            XmlUtil::parseLocalSaveFile(account);


            // Could not update session_mt_id, should not happen unless problem with DB
            if (account->id == -1)  {
                ui->webView_login->load(QUrl(Environnement::getUrlLogin()));
                return;
            }


            ///------------------ Good login, save password locally if 'remember me' is checked ---------------
            QString pw = account->password;
            SimpleCrypt crypto(Q_UINT64_C(0xdd85116f2b81d85f)); //some random number
            QString encryptedPw = crypto.encryptToString(pw);
            //        QString decrypted = crypto.decryptToString(encryptedPw);
            //        qDebug() << pw << endl << encryptedPw << endl << decrypted;

            //        foreach (QString username, settings->lstUsername)
            //            qDebug() << "User:" << username;


            if (!settings->lstUsername.contains(account->email)) {
                settings->lstUsername.append(account->email);
                if (settings->lstUsername.size() > 5)
                    settings->lstUsername.removeFirst();
            }

            //        foreach (QString username, settings->lstUsername)
            //            qDebug() << "User2:" << username;

            settings->lastLoggedUsername = account->email;
            settings->lastLoggedKey = encryptedPw;
            settings->saveGeneralSettings();

            this->accept();
        });

    }


    /// ----------------- Fill default email and set focus on password field ----------------
    /// if first login
    else  {

        // Fill other username used
        //lstUsernameSorted.sort(Qt::CaseInsensitive);

        QString jsToExecute;
        for (int i=0; i<settings->lstUsername.size(); i++) {
            QString username = settings->lstUsername.at(i);
            QString liToAdd = QString("\"<li class='li-username'><a>%1</a></li>\"").arg(username);
            jsToExecute += QString("$(\"#dropdown-select-username\").prepend(%1);").arg(liToAdd);
        }
        //        ui->webView_login->page()->mainFrame()->documentElement().evaluateJavaScript(jsToExecute + "; null" );
        ui->webView_login->page()->runJavaScript(jsToExecute);
        //--------------------



        //more than 1 user in the list? show caret
        if (settings->lstUsername.size() <= 1) {
            QString jsToExecute = "$(\"#button-caret-username\").attr('disabled', true);";
            //            ui->webView_login->page()->mainFrame()->documentElement().evaluateJavaScript(jsToExecute + "; null" );
            ui->webView_login->page()->runJavaScript(jsToExecute);
        }
        else {
            QString jsToExecute = "$(\"#button-caret-username\").attr('disabled', false);";

            QString liToAdd = "\"<li class='divider'></li>\"";
            jsToExecute += QString("$(\"#dropdown-select-username\").append(%1);").arg(liToAdd);

            QString clearList = tr("Clear List");
            liToAdd  = QString("\"<li class='li-username' data-id='clearer'><a>%1</a></li>\"").arg(clearList);;
            jsToExecute += QString("$(\"#dropdown-select-username\").append(%1);").arg(liToAdd);
            //            ui->webView_login->page()->mainFrame()->documentElement().evaluateJavaScript(jsToExecute + "; null" );
            ui->webView_login->page()->runJavaScript(jsToExecute);
        }



        qDebug() << "LAST LOGGER USER NAME IS" << settings->lastLoggedUsername;
        if (settings->lastLoggedUsername != "") {

            /// Name
            if (firstLogin) {


                // Fill Last logged Username
                QString jsToExecute = QString("$('#email').val( '%1' ); ").arg((settings->lastLoggedUsername));
                //                ui->webView_login->page()->mainFrame()->documentElement().evaluateJavaScript(jsToExecute + "; null" );
                qDebug() << "OK we should set it here! pw" << jsToExecute;
                ui->webView_login->page()->runJavaScript(jsToExecute);


                if (settings->rememberMyPassword && settings->lastLoggedKey != "") {


                    // Decrypt password
                    SimpleCrypt crypto(Q_UINT64_C(0xdd85116f2b81d85f)); //some random number
                    QString decrypted = crypto.decryptToString(settings->lastLoggedKey);
                    //                qDebug() << "TEST:" << savedEncryptedPassword << endl << decrypted;

                    // Fill password and click on submit
                    QString jsToExecute = QString("$('#password').val( '%1' ); ").arg((decrypted));
                    jsToExecute += "$('#login-btn').click(); ";
                    //
                    qDebug() << "OK AUTOFILL PARSSWORD WITH " << jsToExecute;
                    ui->webView_login->page()->runJavaScript(jsToExecute);
                }

                ui->webView_login->setFocus();
                QKeyEvent *event = new QKeyEvent ( QEvent::KeyPress, Qt::Key_Tab, Qt::NoModifier);
                QCoreApplication::postEvent (ui->webView_login, event);

                firstLogin = false;
            }
        }
    }


}




/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void DialogLogin::onVersionTimeout() {
    if (!replyVersion) return;
    LOG_WARN("DialogLogin", QStringLiteral("Version check timed out after 10 s – aborting and proceeding to login"));
    ui->label_process->setText(tr("Version check timed out, loading login…"));
    // Aborting emits finished() → slotFinishedGetVersion() handles the rest.
    replyVersion->abort();
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void DialogLogin::slotFinishedGetVersion() {

    m_versionTimeout->stop();

    if (!replyVersion) return;

    const QNetworkReply::NetworkError netError = replyVersion->error();

    if (netError == QNetworkReply::NoError) {
        const QByteArray arrayData = replyVersion->readAll();
        LOG_DEBUG("DialogLogin", QStringLiteral("Version check response: ") + QString::fromUtf8(arrayData));

        const QString latestVersion = Util::parseJsonObjectVersion(QString::fromUtf8(arrayData));
        LOG_INFO("DialogLogin", QStringLiteral("Current: ") + Environnement::getVersion()
                 + QStringLiteral("  Latest: ") + latestVersion);

        if (!latestVersion.isEmpty() && Util::isVersionNewer(Environnement::getVersion(), latestVersion)) {
            gotUpdateDialog = true;
            LOG_INFO("DialogLogin", QStringLiteral("Update available – showing dialog"));
            ui->label_process->setText(tr("Update available!"));
            UpdateDialog up(latestVersion, this);
            up.exec();
        } else {
            ui->label_process->setText(tr("Loading page..."));
            ui->webView_login->setUrl(QUrl(Environnement::getUrlLogin()));
            connect(ui->webView_login, SIGNAL(loadFinished(bool)), this, SLOT(loginLoaded(bool)));
        }

        if (gotUpdateDialog) {
            replyVersion->deleteLater();
            replyVersion = nullptr;
            return QDialog::reject();
        }

    } else {
        const QString errMsg = replyVersion->errorString();
        LOG_WARN("DialogLogin", QStringLiteral("Version check failed (") + errMsg
                 + QStringLiteral(") – proceeding to login"));
        ui->label_process->setText(tr("Version check failed, loading login…"));
        // Don't block the user — proceed to login even if the check fails.
        ui->webView_login->setUrl(QUrl(Environnement::getUrlLogin()));
        connect(ui->webView_login, SIGNAL(loadFinished(bool)), this, SLOT(loginLoaded(bool)));
    }

    replyVersion->deleteLater();
    replyVersion = nullptr;
}





//----------------------------------------------------------------------------------------
void DialogLogin::slotFinishedGoogle() {


    //success, process data
    if (replyGoogle->error() == QNetworkReply::NoError) {
        ui->label_process->setText(tr("Checking for updates..."));
    }
    else {
        //Remove for now, testing local
        qDebug() << "Problem reaching google..." << replyGoogle->errorString();
        QMessageBox msgBox;
        msgBox.setIcon(QMessageBox::Critical);
        msgBox.setText(tr("No internet connection was detected."));
        msgBox.setInformativeText(tr("Please verify your internet connection."));
        msgBox.setStandardButtons(QMessageBox::Ok);
        msgBox.setDefaultButton(QMessageBox::Save);
        msgBox.exec();
    }
    replyGoogle->deleteLater();
}




/////////////////////////////////////////////////////////////////////////////////////////////////////////////
void DialogLogin::on_checkBox_autoLogin_clicked(bool checked)
{

    qDebug() << "on_checkBox_autoLogin_clicked";

    settings->rememberMyPassword = checked;
    settings->saveGeneralSettings();
}



/////////////////////////////////////////////////////////////////////////////////////////////////////////////
void DialogLogin::on_comboBox_language_currentIndexChanged(int index) {

    //    qDebug() << "LANGUAGE CHANGED!" <<index;
    Q_UNUSED(index);

    firstLogin = true;
    QString languageToPut = ui->comboBox_language->currentText();


    qApp->removeTranslator(&m_translator);

    bool success;
    if (languageToPut == "English") {
        qDebug() << "got here English";
        success = m_translator.load(":/language/language/powervelo_en.qm");
        settings->language_index = 0;
        settings->language = "en";
        settings->saveLanguage();

        ui->webView_login->setUrl(QUrl(Environnement::getUrlLogin()));
    }
    else if (languageToPut == "Français") {
        qDebug() << "got here French";
        success = m_translator.load(":/language/language/powervelo_fr.qm");
        settings->language_index = 1;
        settings->language = "fr";
        settings->saveLanguage();

        ui->webView_login->setUrl(QUrl(Environnement::getUrlLogin()));
    }
    else {
        success = m_translator.load(":/language/language/powervelo_en.qm");
        settings->language_index = 0;
        settings->language = "en";

        ui->webView_login->setUrl(QUrl(Environnement::getUrlLogin()));
    }

    qDebug() << "SUCCES?" << success;

    if(success) {
        qApp->installTranslator(&m_translator);
        ui->retranslateUi(this);
        m_currLang = languageToPut;
    }


}



