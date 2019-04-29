#include "webviewwidget.h"


WebViewWidget::~WebViewWidget()
{

}



WebViewWidget::WebViewWidget(QWidget *parent) : QWidget(parent)
{

    vLayout = new QVBoxLayout(this);
    vLayout->setContentsMargins(0,0,0,0);

    webEngineView = new QWebEngineView(this);

    vLayout->addWidget(webEngineView);


}

