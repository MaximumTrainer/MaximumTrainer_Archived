#include "bottombar.h"
#include "ui_bottombar.h"

#include <QSizeGrip>
#include <QTimer>
#include <QMenuBar>

#include "util.h"
#include "dialogmainwindowconfig.h"
#include "environnement.h"
#include "dialoginfowebview.h"
#include "account.h"


BottomBar::~BottomBar()
{
    delete ui;
}


BottomBar::BottomBar(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::BottomBar)
{
    ui->setupUi(this);


    ui->label_circle->setVisible(false);


    connect(ui->label_msgHubStatus, SIGNAL(clicked(bool)), this, SLOT(aboutANT_Stick_Not_Found()) );
    connect(ui->label_circle, SIGNAL(clicked(bool)), this, SLOT(aboutANT_Stick_Not_Found()) );
}




////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void BottomBar::updateHubStatus(int currentHubStarting) {

    ui->label_msgHubStatus->setText(tr("Starting ANT+ Stick # ") + QString::number(currentHubStarting) + "...");
    ui->label_msgHubStatus->fadeIn(500);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void BottomBar::hubStickFound(int numberFound, QString descriptionSticks) {


    qDebug() << "numberFound" << numberFound << "descriptionSticks" << descriptionSticks;

    if (numberFound > 0) {
        ui->label_circle->setVisible(true);
        ui->label_circle->setStyleSheet("QLabel#label_circle{image: url(:/image/icon/greenc);border-radius: 1px;}");
        if (numberFound == 1) {
            ui->label_msgHubStatus->setText(QString::number(numberFound) + " " +  tr("ANT+ Stick Ready"));
        }
        else {
            ui->label_msgHubStatus->setText(QString::number(numberFound) + " " +  tr("ANT+ Sticks Ready"));
        }
        ui->label_msgHubStatus->fadeIn(500);

        ui->label_msgHubStatus->setToolTip(descriptionSticks);

        disconnect(ui->label_msgHubStatus, SIGNAL(clicked(bool)), this, SLOT(aboutANT_Stick_Not_Found()) );
        ui->label_msgHubStatus->setCursor(Qt::ArrowCursor);
    }
    else {
        ui->label_circle->setVisible(true);
        ui->label_circle->setStyleSheet("QLabel#label_circle{image: url(:/image/icon/redc);border-radius: 1px;}");
        ui->label_msgHubStatus->setText(tr("No ANT+ Stick found"));
        ui->label_msgHubStatus->fadeIn(500);
    }


}





//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void BottomBar::setGeneralMessage(const QString& text, int timeToDisplay) {

    ui->label_generalMsg->setText( text );
    ui->label_generalMsg->show();
    ui->label_generalMsg->fadeInAndFadeOutAfterPause(400, 1000, timeToDisplay);
}
//----------------------------------------------
void BottomBar::setGeneralMessage(const QString& text) {

    ui->label_generalMsg->setText( text );
    ui->label_generalMsg->show();
    ui->label_generalMsg->fadeIn(300);


}
//---------------------------------------
void BottomBar::removeGeneralMessage() {

    ui->label_generalMsg->fadeOut(500);
}



//-----------------------------------------------------------
void BottomBar::aboutANT_Stick_Not_Found() {

    DialogInfoWebView infoAntStick;
    infoAntStick.setUrlWebView(Environnement::getUrlSupportAntStick());
    infoAntStick.exec();
}


