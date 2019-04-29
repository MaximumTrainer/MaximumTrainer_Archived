#ifndef DIALOGINFOWEBVIEW_H
#define DIALOGINFOWEBVIEW_H

#include <QDialog>
#include <QUrl>

namespace Ui {
class DialogInfoWebView;
}

class DialogInfoWebView : public QDialog
{
    Q_OBJECT

public:
    explicit DialogInfoWebView(QWidget *parent = 0);
    ~DialogInfoWebView();


    void setUrlWebView(QString url);
    void setTitle(QString title);
    void setUsedForStrava(bool used);
    void setUsedForTrainingPeaks(bool used);


signals:
    void stravaLinked(bool linked);
    void trainingPeaksLinked(bool linked);




private slots:
    void pageLoaded(bool ok);

private:
    Ui::DialogInfoWebView *ui;


    bool usedForStrava;
    bool usedForTrainingPeaks;
    QString emailUser;
};

#endif // DIALOGINFOWEBVIEW_H
