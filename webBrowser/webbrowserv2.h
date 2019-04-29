#ifndef WEBBROWSERV2_H
#define WEBBROWSERV2_H

#include <QWidget>
#include <QWebEngineView>
#include <QLineEdit>
#include <QGridLayout>
#include <QAction>
#include <QToolBar>
#include <QTimer>
#include <QEvent>
#include <QLabel>

#include "settings.h"



class WebBrowserV2 : public QWidget
{
    Q_OBJECT
public:
    explicit WebBrowserV2(QWidget *parent = 0);
    ~WebBrowserV2();
     bool eventFilter(QObject *watched, QEvent *event);

signals:


public slots:
    void playVideo(bool play);

    void pauseVideo();
    void playVideo();

private slots:
     void changeLocation();
     void adjustLocation();
     void updateUrlOfLineEdit(QUrl);

     void fullScreenTrigger(bool);
     void hideWidgets();



private :
    Settings *settings;

    QGridLayout *gLayout;
    QToolBar *toolBar;
    QWebEngineView *webEngineView;
    QLineEdit *locationEdit;

    QAction *actionFullScreen;
    QAction *actionPlayStop;

    QTimer *timerHideWidgets;

    QString smallscreenURL;
    QString langPlayer;

};

#endif // WEBBROWSERV2_H
