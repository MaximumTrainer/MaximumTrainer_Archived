#include "webbrowserv2.h"


#include <QTextEdit>
#include <QKeyEvent>
#include <QApplication>
#include <QWebEngineSettings>

#include "environnement.h"


WebBrowserV2::~WebBrowserV2()
{

}



WebBrowserV2::WebBrowserV2(QWidget *parent) : QWidget(parent)
{

    /// get the language
    langPlayer = "en";
    this->settings = qApp->property("User_Settings").value<Settings*>();
    langPlayer = settings->language;  //en, fr


    /// build the UI
    gLayout = new QGridLayout(this);
    gLayout->setContentsMargins(0,0,0,0);


    locationEdit = new QLineEdit(this);
    locationEdit->setSizePolicy(QSizePolicy::Expanding, locationEdit->sizePolicy().verticalPolicy());
    //    connect(locationEdit, SIGNAL(returnPressed()), SLOT(changeLocation()));

    webEngineView = new QWebEngineView(this);
    webEngineView->settings()->setAttribute(QWebEngineSettings::PluginsEnabled, true);

    connect(webEngineView, SIGNAL(loadFinished(bool)), SLOT(adjustLocation()));
    connect(webEngineView, SIGNAL(urlChanged(QUrl)), SLOT(updateUrlOfLineEdit(QUrl)));



    toolBar = new QToolBar(this);
    toolBar->addAction(webEngineView->pageAction(QWebEnginePage::Back));
    toolBar->addAction(webEngineView->pageAction(QWebEnginePage::Forward));
    toolBar->addAction(webEngineView->pageAction(QWebEnginePage::Reload));
    toolBar->addAction(webEngineView->pageAction(QWebEnginePage::Stop));
    toolBar->addSeparator();
    actionFullScreen = toolBar->addAction(QIcon(":/image/icon/fullscreen"), tr("Toggle Fullscreen"));
    actionFullScreen->setCheckable(true);
    actionPlayStop = toolBar->addAction(tr("PlayStop"));
    actionPlayStop->setCheckable(true);

    toolBar->addWidget(locationEdit);


    connect(actionFullScreen, SIGNAL(triggered(bool)), SLOT(fullScreenTrigger(bool)));
    connect(actionPlayStop, SIGNAL(triggered(bool)), SLOT(playVideo(bool)));

    gLayout->addWidget(toolBar, 0, 0, 1, 1);
    gLayout->addWidget(webEngineView, 1, 0, 1, 1);

    timerHideWidgets = new QTimer(this);
    connect(timerHideWidgets, SIGNAL(timeout()),SLOT(hideWidgets()));
    hideWidgets();


    locationEdit->installEventFilter(this);
    toolBar->installEventFilter(this);
    webEngineView->installEventFilter(this);


    QSettings settings;
    settings.beginGroup("webBrowserWorkout");
    QString urlSaved = settings.value("defaultUrl", "http://netflix.com" ).toString();
    settings.endGroup();
    webEngineView->load(QUrl(urlSaved));


    QWebEngineSettings::globalSettings()->setAttribute(QWebEngineSettings::PluginsEnabled, true);


}



//----------------------------------------------------------------------------------------
void WebBrowserV2::hideWidgets() {

    qDebug() << "hideWidgets";

    timerHideWidgets->stop();
    toolBar->setVisible(false);

}

//------------------------------------------------------------------------------------------------
bool WebBrowserV2::eventFilter(QObject *watched, QEvent *event) {


    Q_UNUSED(watched);


//    qDebug() << "watched object WebBrowserV2" << watched << "event:" << event << "eventType" << event->type();



    if(event->type() == QEvent::MouseMove){
        timerHideWidgets->start(7000);
        toolBar->setVisible(true);
    }
    else if (event->type() == QEvent::KeyPress) {
        timerHideWidgets->start(7000);
        toolBar->setVisible(true);
    }
    else if (event->type() == QEvent::Enter || event->type() == QEvent::HoverEnter  || event->type() == QEvent::HoverMove ) {
        timerHideWidgets->start(7000);
        toolBar->setVisible(true);
    }

    if (event->type() == QEvent::KeyPress) {
        timerHideWidgets->start(7000);
        toolBar->setVisible(true);

        //stop event propagation (that started workout when enter pressed on QLineEdit Url)
        QKeyEvent* key = static_cast<QKeyEvent*>(event);
        if ( (key->key()==Qt::Key_Enter) || (key->key()==Qt::Key_Return) ) {
            changeLocation();
            return true;
            //        watched object WebBrowserV2 QLineEdit(0x1a795e00) event: QKeyEvent(KeyPress, Key_Return, text="\r") eventType QEvent::Type(KeyPress)
        }
    }


    return false;

}


//----------------------------------------------------------------------------------------
void WebBrowserV2::fullScreenTrigger(bool trigger) {

    qDebug() << "fullScreenTrigger" << trigger;


    QString url = webEngineView->url().toString();
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
                    "&enablejsapi=1" +
                    "&hl="+ langPlayer;
            //
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
        webEngineView->load(realURL);
    }




}


//----------------------------------------------------------------------------------------
void WebBrowserV2::changeLocation()
{
    qDebug () << "changeLocation start..";
    QUrl url = QUrl::fromUserInput(locationEdit->text());
    webEngineView->load(url);
    //    webEngineView->setFocus();

    qDebug () << "changeLocation end..";
}


//----------------------------------------------------------------------------------------
void WebBrowserV2::adjustLocation()
{
    qDebug () << "adjustLocation start..";
    locationEdit->setText(webEngineView->url().toString());
    qDebug () << "adjustLocation start..";
}


//------------------------------------------------------------------------------
void WebBrowserV2::updateUrlOfLineEdit(const QUrl url) {

    qDebug () << "updateUrlOfLineEdit start..";
    ///TOFIX: Url not updating sometimes, example : youtube video clicked doesn't update lineedit
    //    qDebug() << "GOT HER EUPDATE URL" << url;
    locationEdit->setText(url.toString());
    locationEdit->repaint();
    locationEdit->update();
    qDebug () << "updateUrlOfLineEdit end..";
}

//////////////////////////////////////////////////////////////////////////////////////////////
void WebBrowserV2::playVideo(bool play) {

    qDebug () << "playVideo start..";

    if (play)
        playVideo();
    else
        pauseVideo();


    qDebug () << "playVideo end..";
}



//-----------------------------------------------------------------
void WebBrowserV2::pauseVideo() {

    qDebug() << "WebBrowser::pauseVideo()";

    QString functionJs = "function findFirstVideoTag() {"
                         "var all = document.getElementsByTagName('video');"
                         "if (all.length > 0)"
                         "return all[0];"
                         "else {"
                         "var i, frames;"
                         "frames = document.getElementsByTagName('iframe');"
                         "for (i = 0; i < frames.length; ++i) {"
                         "try { var childDocument = frame.contentDocument } catch (e) { continue };"
                         "all = frames[i].contentDocument.document.getElementsByTagName('video');"
                         "if (all.length > 1)"
                         "return all[0];"
                         "}"
                         "return null;"
                         "}"
                         "}"
                         "video_element = findFirstVideoTag();"
                         "video_element.pause();";

    webEngineView->page()->runJavaScript(functionJs);
}



//-----------------------------------------------------------------
void WebBrowserV2::playVideo() {

    qDebug() << "WebBrowser::playVideo()";

    QString functionJs = "function findFirstVideoTag() {"
                         "var all = document.getElementsByTagName('video');"
                         "if (all.length > 0)"
                         "return all[0];"
                         "else {"
                         "var i, frames;"
                         "frames = document.getElementsByTagName('iframe');"
                         "for (i = 0; i < frames.length; ++i) {"
                         "try { var childDocument = frame.contentDocument } catch (e) { continue };"
                         "all = frames[i].contentDocument.document.getElementsByTagName('video');"
                         "if (all.length > 1)"
                         "return all[0];"
                         "}"
                         "return null;"
                         "}"
                         "}"
                         "video_element = findFirstVideoTag();"
                         "video_element.play();";

    webEngineView->page()->runJavaScript(functionJs);


}



