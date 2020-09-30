#include "webbrowser.h"
#include "ui_webbrowser.h"
#include <QToolBar>
#include <QDebug>
#include <QVBoxLayout>
#include <QUrl>
#include "settings.h"

#include "mywebpage.h"


WebBrowser::~WebBrowser() {
    delete ui;
}


/////////////////////////////////////////////////////////////////////////////////////////////////////////
WebBrowser::WebBrowser(QWidget *parent) : QWidget(parent), ui(new Ui::Widget) {

    ui->setupUi(this);

    MyWebPage *myWebPage = new MyWebPage(this);
    ui->webView->setPage(myWebPage);


    /// get the language
    langPlayer = "en";



    smallscreenURL = "";
    errorLastLoad = false;


    ///// QWEBVIEW -----------------------------------------
    QNetworkAccessManager *nam = qApp->property("NetworkManager").value<QNetworkAccessManager*>();
    ui->webView->page()->setNetworkAccessManager(nam);
    ui->webView->setUrl(QUrl("https://www.google.com"));
    /////////////////////////



    locationEdit = new QLineEdit(this);
    locationEdit->setSizePolicy(QSizePolicy::Expanding, locationEdit->sizePolicy().verticalPolicy());
    connect(locationEdit, SIGNAL(returnPressed()), SLOT(loadUrlFromLineEdit()));



    widgetLoading = new QWidget(this);
    QVBoxLayout *vLayout = new QVBoxLayout(widgetLoading);
    vLayout->setContentsMargins(0,0,0,0);

    QSpacerItem *spacer = new QSpacerItem(200, 200, QSizePolicy::Expanding, QSizePolicy::Expanding);

    loadingMsg = new FaderLabel(widgetLoading);
    loadingMsg->setMaximumHeight(25);
    loadingMsg->setMaximumWidth(300);
    loadingMsg->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
    loadingMsg->setAlignment(Qt::AlignBottom | Qt::AlignLeft);
    loadingMsg->hide();
    loadingMsg->setAttribute(Qt::WA_TransparentForMouseEvents,true);

    vLayout->addSpacerItem(spacer);
    vLayout->addWidget(loadingMsg, Qt::AlignBottom);
    ui->gridLayout->addWidget(widgetLoading, 1, 0, 1, 1);


    widgetLoading->setAttribute(Qt::WA_TransparentForMouseEvents,true);
    loadingMsg->setAttribute(Qt::WA_TransparentForMouseEvents,true);


    ui->webView->settings()->setAttribute(QWebSettings::PluginsEnabled, true);
    ui->webView->settings()->setAttribute(QWebSettings::JavascriptEnabled,true);


    connect(ui->webView, SIGNAL(loadStarted()), this, SLOT(loadStartedSlot()));
    connect(ui->webView, SIGNAL(loadFinished(bool)), this, SLOT(loadFinished(bool)));
    connect(ui->webView, SIGNAL(loadProgress(int)), this, SLOT(loadProgressToShow(int)));
    connect(ui->webView, SIGNAL(urlChanged(QUrl)), this, SLOT(updateUrlOfLineEdit(QUrl)));
    connect(ui->webView, SIGNAL(loadFinished(bool)), this, SLOT(adjustLocation()));


    ui->widget_toolbar->addAction(ui->webView->pageAction(QWebPage::Back));
    ui->widget_toolbar->addAction(ui->webView->pageAction(QWebPage::Forward));
    ui->widget_toolbar->addAction(ui->webView->pageAction(QWebPage::Reload));
    ui->widget_toolbar->addAction(ui->webView->pageAction(QWebPage::Stop));
    ui->widget_toolbar->addWidget(locationEdit);
    ui->widget_toolbar->setMaximumHeight(50);






    timer_hideWidgets = new QTimer(this);
    showFullscreenButton = false;
    connect(timer_hideWidgets, SIGNAL(timeout()), this, SLOT(hideWidgets()));
    hideWidgets();


    ui->webView->installEventFilter(this);
    ui->pushButton_fullscreen->installEventFilter(this);;
    ui->widget_toolbar->installEventFilter(this);
    widgetLoading->installEventFilter(this);
    locationEdit->installEventFilter(this);





}




//////////////////////////////////////////////////////////////////////////////////////////////////////////////
void WebBrowser::hideWidgets() {

    timer_hideWidgets->stop();
    ui->pushButton_fullscreen->setVisible(false);
    ui->widget_toolbar->setVisible(false);

}


//------------------------------------------------------------------------------------------------
bool WebBrowser::eventFilter(QObject *watched, QEvent *event) {


    Q_UNUSED(watched);

    if(event->type() == QEvent::MouseMove)
    {
        //        qDebug() << "watched object" << watched << "event:" << event << "eventType" << event->type();
        timer_hideWidgets->start(3000);
        ui->widget_toolbar->setVisible(true);
        if (showFullscreenButton)
            ui->pushButton_fullscreen->setVisible(true);

    }
    if (event->type() == QEvent::KeyPress) {
        timer_hideWidgets->start(3000);
    }
    return false;

}





//------------------------------------------------------------------------------
void WebBrowser::updateUrlOfLineEdit(const QUrl url) {

    ///TOFIX: Url not updating sometimes, example : youtube video clicked doesn't update lineedit
    //    qDebug() << "GOT HER EUPDATE URL" << url;
    locationEdit->setText(url.toString());




}


//------------------------------------------------------------------------------
void WebBrowser::adjustLocation() {


    locationEdit->setText(ui->webView->url().toString());

}



//------------------------------------------------------------------------------
void WebBrowser::loadUrlFromLineEdit() {

    QString url = locationEdit->text();
    if (url.startsWith("WWW.", Qt::CaseInsensitive) || url.startsWith("HTTP", Qt::CaseInsensitive) ) {
        //do nothing
    }
    else {
        url.prepend("http://");
    }
    if (url.startsWith("WWW.", Qt::CaseInsensitive)) {
        url.prepend("http://");
    }
    if (url.indexOf('.') == -1) {
        url.append(".com");
    }


    //    qDebug() << "URL TO LOAD:" << url;
    ui->webView->load(QUrl(url));
}


/////////////////////////////////////////////////////////////////////////////////////////////////////////
void WebBrowser::loadStartedSlot() {

    loadingMsg->setText(tr(" Loading..."));
    loadingMsg->setStyleSheet("background-color : rgba(1,1,1,140);"
                              "color : white;");
    loadingMsg->show();
    loadingMsg->fadeIn(400);

}


/////////////////////////////////////////////////////////////////////////////////////////////////////////
void WebBrowser::loadProgressToShow(int perc) {

    if (!errorLastLoad)
        loadingMsg->setText(tr("Loading...") + QString::number(perc) + "%");
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////
void WebBrowser::loadFinished(bool status) {



    /// Remove message
    if (status) {
        errorLastLoad = false;
        loadingMsg->setText("");
        loadingMsg->fadeOut(300);
    }
    /// Error happened
    else {
        errorLastLoad = true;
        loadingMsg->setText(tr("Error Loading Page"));
        loadingMsg->fadeInAndFadeOutAfterPause(400, 400, 2500);
    }


    /// Show Youtube fullscreen bouton?
    QString url = locationEdit->text();
    if (url.indexOf("youtube") != -1) {
        if  (url.indexOf("watch?v=") != -1) {
            showFullscreenButton = true;
            ui->pushButton_fullscreen->setVisible(true);
            ui->pushButton_fullscreen->setText(tr("Show Fullscreen"));
            ui->pushButton_fullscreen->setIcon(QIcon(":/image/icon/fullscreen"));
        }
        else if (url.indexOf("embed") != -1) {
            ui->pushButton_fullscreen->setText(tr("Show Normal"));
            ui->pushButton_fullscreen->setIcon(QIcon());
        }
    }
    else {
        showFullscreenButton = false;
        ui->pushButton_fullscreen->setVisible(false);
    }



}




//-----------------------------------------------------------------
void WebBrowser::pauseVideo() {

    qDebug() << "WebBrowser::pauseVideo()";




//    QWebElement player1 = ui->webView->page()->mainFrame()->documentElement().findFirst("div[id=\"player\"]");
//    qDebug() << "WE FOUND PLAYER2!" << player1.toOuterXml();

//    qDebug() << "OK PAGE HERE:" << ui->webView->page()->mainFrame()->documentElement().toOuterXml();

//    qDebug() << "*****\nOK FULL PAGE HERE" << ui->webView->page()->mainFrame()->toHtml();



    //    if ( $( '#nonexistent' ).length > 0 ) {
    //      // Correct! This code will only run if there's an element in your page
    //      // with an ID of 'nonexistent'
    //    }




    /// Normal Youtube
    QString jsValue = "document.getElementById('movie_player').pauseVideo();";
    ui->webView->page()->mainFrame()->evaluateJavaScript(jsValue + "; null");

    /// Embedded Youtube
//    QWebElement player = ui->webView->page()->mainFrame()->documentElement().findFirst("div[id=\"player\"]");
//    QWebElement embed = player.findFirst("embed");
//    QString embedID = embed.attribute("id");
//    QString jsToExecute = QString("document.getElementById('%1').pauseVideo();").arg(embedID);
//    ui->webView->page()->mainFrame()->evaluateJavaScript(jsToExecute + "; null");


    QString jsToExecute = QString("document.getElementsByTagName('embed')[0].pauseVideo();");
    ui->webView->page()->mainFrame()->evaluateJavaScript(jsToExecute + "; null");




}



//-----------------------------------------------------------------
void WebBrowser::playVideo() {

    qDebug() << "WebBrowser::playVideo()";

    /// Normal Youtube
    QString jsValue = "document.getElementById('movie_player').playVideo();";
    ui->webView->page()->mainFrame()->evaluateJavaScript(jsValue + "; null");

    /// Embedded Youtube
//    QWebElement player = ui->webView->page()->mainFrame()->documentElement().findFirst("div[id=\"player\"]");
//    QWebElement embed = player.findFirst("embed");
//    QString embedID = embed.attribute("id");
//    QString jsToExecute = QString("document.getElementById('%1').playVideo();").arg(embedID);
//    ui->webView->page()->mainFrame()->evaluateJavaScript(jsToExecute + "; null");

    QString jsToExecute = QString("document.getElementsByTagName('embed')[0].playVideo();");
    ui->webView->page()->mainFrame()->evaluateJavaScript(jsToExecute + "; null");


}


//-----------------------------------------------------------------
void WebBrowser::on_pushButton_fullscreen_clicked() {



    QString url = locationEdit->text();
    QString newURL;
    bool reloadUrl = false;


    ///-- YOUTUBE WEB VIDEO API
    /// https://developers.google.com/youtube/player_parameters#enablejsapi
    /// https://developers.google.com/youtube/js_api_reference
    if (url.indexOf("youtube") != -1)
    {
        if (url.indexOf("watch?v=") != -1) {
            qDebug() << "YOUTUBE WEBPAGE, PUT TO EMBED URL";

            smallscreenURL = url;

            QString videoID;
            QStringList lstString = url.split("v=");
            if (lstString.size() >= 2) {

                videoID = lstString[1];
                int ampersandPosition = videoID.indexOf("&");
                if (ampersandPosition != -1) {
                    videoID = videoID.mid(0, ampersandPosition);
                }
            }

            //https://www.youtube.com/embed/7t0EtKlQxyo?autohide=1&autoplay=1&enablejsapi=1&hl=en&modestbranding=1&playerapiid=movie_player

            newURL = "http://www.youtube.com/embed/" + videoID +
                    "?autohide=1" +
                    "&autoplay=1" +
                    "&modestbranding=1" +
                    "&hl="+ langPlayer;
            //                    "&enablejsapi=1" +
            //                    "&origin=https://maximumtrainer.com" +
            //                    "&allowscriptaccess=always";


            reloadUrl = true;

        }
        else if (url.indexOf("embed") != -1) {
            qDebug() << "YOUTUBE WEBPAGE, normal URL";

            newURL = smallscreenURL;
            reloadUrl = true;
        }
    }
    ///    else if () {} Other Web video API?





    if (reloadUrl) {
        QUrl realURL(newURL);
        realURL.toEncoded();
        ui->webView->load(realURL);
    }



}








