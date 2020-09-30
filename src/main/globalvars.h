#ifndef GLOBALVARS_H
#define GLOBALVARS_H

#include <QObject>
#include <QApplication>

#include <QNetworkReply>

class GlobalVars : public QObject
{
    Q_OBJECT
public:
    explicit GlobalVars(QObject *parent = 0);

signals:

public slots:
    void sslErrorHandler(QNetworkReply* qnr, const QList<QSslError> & errlist);

};

#endif // GLOBALVARS_H
