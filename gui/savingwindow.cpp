#include "savingwindow.h"
#include "ui_savingwindow.h"

#include <QMovie>


SavingWindow::~SavingWindow()
{
    delete ui;
}



SavingWindow::SavingWindow(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::SavingWindow)
{
    ui->setupUi(this);

    this->setWindowFlags(Qt::SplashScreen);



    ///Set loading icon (wait for login page to show)
    QMovie *movie = new QMovie(":/image/icon/loading", QByteArray(), this);
    movie->setPaused(false);
    ui->label_spinner->setMovie(movie);
    movie->setSpeed(150);
    movie->start();

}


/////////////////////////////////////////////////////////////////
void SavingWindow::setMessage(QString msg) {
    ui->label_msg->setText(msg);
}
