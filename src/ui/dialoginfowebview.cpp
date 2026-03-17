#include "dialoginfowebview.h"
#include "ui_dialoginfowebview.h"

#include <QDesktopServices>
#include <QUrlQuery>
#include <QWebEngineView>
#include <QWebEngineProfile>
#include "util.h"
#include "account.h"
#include "extrequest.h"
#include "environnement.h"

#include "myqwebenginepage.h"

DialogInfoWebView::~DialogInfoWebView()
{
    // Abort any in-flight client-side token exchange so it doesn't update
    // the global Account after the dialog has been destroyed.
    if (m_intervalsTokenReply) {
        m_intervalsTokenReply->abort();
        m_intervalsTokenReply->deleteLater();
        m_intervalsTokenReply = nullptr;
    }
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
    usedForIntervalsIcu = false;

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
void DialogInfoWebView::setUsedForIntervalsIcu(bool used) {

    this->usedForIntervalsIcu = used;
}

/////////////////////////////////////////////////////////////////////////
void DialogInfoWebView::setExpectedOAuthState(const QString &state) {

    m_expectedOAuthState = state;
}



/////////////////////////////////////////////////////////////////////////
void DialogInfoWebView::pageLoaded(bool ok){

    qDebug() << "DialogInfoWebView pageLoaded";

    if (!ok || (!usedForStrava && !usedForTrainingPeaks && !usedForIntervalsIcu) )
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
    /// ------ Intervals.icu OAuth2 callback ------------------------------------------------
    else if (usedForIntervalsIcu
             && ui->webView->url().toDisplayString().contains("/intervals_icu_token_exchange"))  {

        const QString urlStr = ui->webView->url().toDisplayString();

        // If the user denied authorization, bail out early.
        if (urlStr.contains("error=")) {
            qDebug() << "Intervals.icu OAuth: user denied authorization or error occurred";
            emit intervalsIcuLinked(false);
            this->reject();
            return;
        }

        // Case 1: The MaximumTrainer.com backend proxy exchanged the code and
        // returned a JSON body — read it directly as plain text.
        // We verify the response contains a JSON object with an "access_token"
        // field before accepting the result.
        if (!urlStr.contains("code=")) {
            ui->webView->page()->toPlainText([=](const QString &response){
                // Validate that the response looks like a JSON object before parsing.
                const QString trimmed = response.trimmed();
                if (!trimmed.startsWith('{')) {
                    qWarning() << "Intervals.icu token exchange: unexpected non-JSON response";
                    emit intervalsIcuLinked(false);
                    this->reject();
                    return;
                }
                Util::parseJsonIntervalsIcuOAuthToken(response);
                Account *account = qApp->property("Account").value<Account*>();
                if (account && !account->intervals_icu_access_token.isEmpty()) {
                    emit intervalsIcuLinked(true);
                    this->accept();
                } else {
                    emit intervalsIcuLinked(false);
                    this->reject();
                }
            });
            return;
        }

        // Case 2: The redirect URI still has ?code= in it (i.e. the backend
        // proxy forwarded it unchanged).  Extract the code and exchange it
        // client-side by POSTing directly to the Intervals.icu token endpoint.
        const QUrl redirectUrl(urlStr);
        const QUrlQuery redirectQuery(redirectUrl);
        const QString code = redirectQuery.queryItemValue("code");
        if (code.isEmpty()) {
            qDebug() << "Intervals.icu OAuth: no code in redirect URL";
            emit intervalsIcuLinked(false);
            this->reject();
            return;
        }

        // Validate CSRF state if one was set at authorization time.
        if (!m_expectedOAuthState.isEmpty()) {
            const QString returnedState = redirectQuery.queryItemValue("state");
            if (returnedState != m_expectedOAuthState) {
                qWarning() << "Intervals.icu OAuth: state mismatch — possible CSRF attack";
                emit intervalsIcuLinked(false);
                this->reject();
                return;
            }
        }

        qDebug() << "Intervals.icu OAuth: exchanging code client-side";
        const QString redirectUri = Environnement::getURLEnvironnement() + "intervals_icu_token_exchange";
        m_intervalsTokenReply = ExtRequest::intervalsIcuOAuthExchange(code, redirectUri);
        if (!m_intervalsTokenReply) {
            emit intervalsIcuLinked(false);
            this->reject();
            return;
        }
        connect(m_intervalsTokenReply, &QNetworkReply::finished,
                this, &DialogInfoWebView::slotIntervalsTokenExchangeFinished);
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

/////////////////////////////////////////////////////////////////////////
/// Called when the direct client-side Intervals.icu token exchange POST completes.
void DialogInfoWebView::slotIntervalsTokenExchangeFinished()
{
    if (!m_intervalsTokenReply) return;

    if (m_intervalsTokenReply->error() == QNetworkReply::NoError) {
        const QByteArray data = m_intervalsTokenReply->readAll();
        Util::parseJsonIntervalsIcuOAuthToken(QString::fromUtf8(data));
        Account *account = qApp->property("Account").value<Account*>();
        if (account && !account->intervals_icu_access_token.isEmpty()) {
            m_intervalsTokenReply->deleteLater();
            m_intervalsTokenReply = nullptr;
            emit intervalsIcuLinked(true);
            this->accept();
            return;
        }
    } else {
        qWarning() << "Intervals.icu token exchange failed:"
                   << m_intervalsTokenReply->errorString();
    }

    m_intervalsTokenReply->deleteLater();
    m_intervalsTokenReply = nullptr;
    emit intervalsIcuLinked(false);
    this->reject();
}




