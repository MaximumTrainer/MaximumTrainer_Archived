#ifndef MYQWEBENGINEPAGE_H
#define MYQWEBENGINEPAGE_H

#include <QWebEnginePage>
#include <QDesktopServices>

class MyQWebEnginePage : public QWebEnginePage
{
    Q_OBJECT

public:
    MyQWebEnginePage(QObject* parent = 0) : QWebEnginePage(parent){}

//    QWebEnginePage(QWebEngineProfile *profile, QObject *parent = Q_NULLPTR)
    using QWebEnginePage::QWebEnginePage;


    bool acceptNavigationRequest(const QUrl & url, QWebEnginePage::NavigationType type, bool isMainFrame) {

        Q_UNUSED(isMainFrame);
        if (type == QWebEnginePage::NavigationTypeLinkClicked)
        {
            foreach (QString urlToCheck, listExternalLink) {
                if (url.toString().contains(urlToCheck)) {
                    QDesktopServices::openUrl(url);
                    return false;
                }
            }

        }
        return true;
    }

//---------------------------------------------------
    void setExternalList(QStringList lst) {
        this->listExternalLink = lst;
    }

private:
    QStringList listExternalLink;


};

#endif // MYQWEBENGINEPAGE_H
