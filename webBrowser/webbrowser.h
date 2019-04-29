#ifndef WEBBROWSER_H
#define WEBBROWSER_H

#include <QWidget>
#include <QIcon>
#include <QUrl>
#include <QPixmap>
#include <QLineEdit>
#include <QTimer>
#include <QMouseEvent>
#include <QNetworkReply>


#include "faderlabel.h"

namespace Ui {
class Widget;
}

class WebBrowser : public QWidget
{
    Q_OBJECT

public:
    explicit WebBrowser(QWidget *parent = 0);
    ~WebBrowser();
    bool eventFilter(QObject *watched, QEvent *event);

    void pauseVideo();
    void playVideo();




private slots:
    void updateUrlOfLineEdit(const QUrl url);
    void adjustLocation();

    void loadUrlFromLineEdit();

    void loadStartedSlot();
    void loadProgressToShow(int perc);
    void loadFinished(bool status);

    void hideWidgets();



    void on_pushButton_fullscreen_clicked();






private :
    Ui::Widget *ui;
    QLineEdit *locationEdit;

    QWidget *widgetLoading;
    FaderLabel *loadingMsg;
    bool errorLastLoad;

    QTimer *timer_hideWidgets;
    bool widgetControlhidden;

    QString smallscreenURL;
    bool showFullscreenButton;

    QString langPlayer;




};

#endif // WEBBROWSER_H
