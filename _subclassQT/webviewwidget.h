#ifndef WEBVIEWWIDGET_H
#define WEBVIEWWIDGET_H

#include <QWidget>
#include <QVBoxLayout>
#include <QtWebEngineWidgets/QWebEngineView>

class WebViewWidget : public QWidget
{
    Q_OBJECT
public:
    explicit WebViewWidget(QWidget *parent = 0);
    ~WebViewWidget();

    QWebEngineView *webEngineView;


private :
    QVBoxLayout *vLayout;


};

#endif // WEBVIEWWIDGET_H
