#include "dialoginfowebview.h"
#include "ui_dialoginfowebview.h"

#include <QDesktopServices>
#include <QWebEngineView>
#include <QWebEngineProfile>
#include "util.h"
#include "account.h"

#include "myqwebenginepage.h"

DialogInfoWebView::~DialogInfoWebView()
{
    delete ui;
}



DialogInfoWebView::DialogInfoWebView(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::DialogInfoWebView)
{
    ui->setupUi(this);


    // External links
    QStringList lstExternal = {"forum", "maximumtrainer", };

    QWebEngineProfile* newProfile = new QWebEngineProfile(this);
    newProfile->setPersistentCookiesPolicy(QWebEngineProfile::NoPersistentCookies);

    MyQWebEnginePage *myPage = new MyQWebEnginePage(newProfile, this);

    myPage->setExternalList(lstExternal);
    ui->webView->setPage(myPage);

    usedForStrava = false;
    usedForTrainingPeaks = false;

    connect(ui->webView, SIGNAL(loadFinished(bool)), this, SLOT(pageLoaded(bool)));

}


/////////////////////////////////////////////////////////////////////////
void DialogInfoWebView::setTitle(QString title) {

    this->setWindowTitle(title);
}
/////////////////////////////////////////////////////////////////////////
void DialogInfoWebView::setUsedForStrava(bool used) {

    this->usedForStrava = used;
}
/////////////////////////////////////////////////////////////////////////
void DialogInfoWebView::setUsedForTrainingPeaks(bool used) {

    this->usedForTrainingPeaks = used;
}



/////////////////////////////////////////////////////////////////////////
void DialogInfoWebView::pageLoaded(bool ok){

    qDebug() << "DialogInfoWebView pageLoaded";

    if (!ok || (!usedForStrava && !usedForTrainingPeaks) )
        return;

    qDebug() << "Got here pageLoaded--" << ui->webView->url().toDisplayString();
    /// ------------------------------- Login sucess! ---------------------------------------
    if (ui->webView->url().toDisplayString().contains("/strava_token_exchange"))  {

        // Only try to parse the json object when request was successful
        if (!ui->webView->url().toDisplayString().contains("&error") && ui->webView->url().toDisplayString().contains("&code")) {
            qDebug() << "Parse Json object - 'access_token'";


            ui->webView->page()->toPlainText([=](const QString &response){
                qDebug() << "HERE IS THE RESP" << response;

                qDebug() << "response is:" << response;
                Util::parseJsonStravaObject(response);

                Account *account = qApp->property("Account").value<Account*>();
                qDebug() << "current Acccess token is" << account->strava_access_token;
                if (account->strava_access_token.size() > 2) {
                    qDebug() << "ok emit strava is linked!";
                    emit stravaLinked(true);
                }
                if (account->training_peaks_access_token.size() > 2) {
                    emit trainingPeaksLinked(true);
                }
                qDebug() << "before end accept here";
            });




        }
    }
    /// ------------------------------- TP Login sucess! ---------------------------------------
    else if (ui->webView->url().toDisplayString().contains("/trainingpeaks_token_exchange"))  {

        // Only try to parse the json object when request was successful
        if (ui->webView->url().toDisplayString().contains("code=")) {
            qDebug() << "Parse Json object - 'access_token'";


            ui->webView->page()->toPlainText([=](const QString &response){
                qDebug() << "HERE IS THE RESP" << response;

                qDebug() << "response is:" << response;
                Util::parseJsonTPObject(response);

                Account *account = qApp->property("Account").value<Account*>();
                if (account->training_peaks_access_token.size() > 2) {
                    emit trainingPeaksLinked(true);
                }
                this->accept();
            });



        }
    }

    qDebug() << "end of login here oK";


}





/////////////////////////////////////////////////////////////////////////
//void DialogInfoWebView::linkClickedWebView(QUrl url) {

//    if (this->usedForStrava || this->usedForTrainingPeaks) {
//        ui->webView->load(url);
//        return;
//    }

//    if (!url.toString().contains("maximumtrainer", Qt::CaseInsensitive)) {
//        QDesktopServices::openUrl(url);
//    }
//    else if (url.toString().contains("forum", Qt::CaseInsensitive)) {
//        QDesktopServices::openUrl(url);
//    }
//    else {
//        ui->webView->load(url);
//    }
//}



/////////////////////////////////////////////////////////////////////////
void DialogInfoWebView::setUrlWebView(QString url) {

    ui->webView->setUrl(QUrl(url));
}




