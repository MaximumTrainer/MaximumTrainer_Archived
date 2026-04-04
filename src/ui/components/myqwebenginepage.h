#ifndef MYQWEBENGINEPAGE_H
#define MYQWEBENGINEPAGE_H

#include <QWebEnginePage>
#include <QDesktopServices>
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
#include <QWebEngineCertificateError>
#endif
#include "logger.h"

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

protected:
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    /// In Qt 6, override the JavaScript console message handler to log
    /// console output, including errors, with source and line information.
    void javaScriptConsoleMessage(JavaScriptConsoleMessageLevel level,
                                  const QString &message,
                                  int lineNumber,
                                  const QString &sourceID) override
    {
        if (level == QWebEnginePage::ErrorMessageLevel) {
            LOG_WARN("WebEngine",
                     QStringLiteral("JS error [%1:%2] %3")
                     .arg(sourceID).arg(lineNumber).arg(message));
        } else {
            LOG_DEBUG("WebEngine",
                      QStringLiteral("JS [%1:%2] %3")
                      .arg(sourceID).arg(lineNumber).arg(message));
        }
    }
#else
    bool certificateError(const QWebEngineCertificateError &error) override
    {
        LOG_WARN("WebEngine",
                 QStringLiteral("SSL certificate error for %1: %2")
                 .arg(error.url().toDisplayString(), error.errorDescription()));
        return false; // reject — do not bypass certificate errors
    }
#endif

private:
    QStringList listExternalLink;


};

#endif // MYQWEBENGINEPAGE_H
