#include "balancewidget.h"
#include "ui_balancewidget.h"

#include <QDebug>
#include "util.h"

BalanceWidget::~BalanceWidget()
{
    delete ui;
}


BalanceWidget::BalanceWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::BalanceWidget)
{
    ui->setupUi(this);


    zoneTarget = 50;
    zoneRange = 3;
    //    ui->label_unit_right->setText(tr("Right Balance:  "));
    //    ui->label_unit_left->setText(tr("Left Balance:  "));
    setStopped(true);
    showMode = 1;
    ui->horizontalSlider->setMinimum(0);
    ui->horizontalSlider->setMaximum(100);


}


//////////////////////////////////////////////////////////////////////////////
void BalanceWidget::setShowMode(int display) {
    this->showMode = display;
}



//////////////////////////////////////////////////////////////////////////////
void BalanceWidget::setStopped(bool b) {
    this->isStopped = b;
}


//////////////////////////////////////////////////////////////////////////////
void BalanceWidget::setValue(int value) {

    ui->horizontalSlider->setValue(value);

    ui->label_powerRight->setText(QString::number(value));
    ui->label_powerLeft->setText(QString::number(100-value));



    //    /// Check Value is outside of zone, change color of QLabel
    int diff = value - zoneTarget;
    int diffAbs = qAbs(diff);


    //    qDebug() << "is stopped?" << isStopped;
    //    qDebug() << "zone Target?" << zoneTarget;
    //    qDebug() << "diff is" << diff << "zoneRange is " << zoneRange;

    if (isStopped || zoneTarget == -1 || diffAbs <= zoneRange)
    {
        //        qDebug() << "GOT HERE NORMAL LABEL**";
        ui->label_unit_right->setStyleSheet("QLabel#label_unit_right { background-color :"+ Util::getColor(Util::DONE).name() + "; }");
        ui->label_unit_left->setStyleSheet("QLabel#label_unit_left { background-color :"+ Util::getColor(Util::DONE).name() + "; }");
    }

    else
    {
        qDebug() << "GOT HERE SHOULD COLOR LABEL!";
        /// Left is too strong is too strong (right too soft) inverse..
        if (diff > zoneRange) {
            qDebug() << "LEFT TOO SOFT**************";
            ui->label_unit_right->setStyleSheet("QLabel#label_unit_right { background-color :"+ Util::getColor(Util::TOO_HIGH).name() + "; }");
            ui->label_unit_left->setStyleSheet("QLabel#label_unit_left { background-color :"+ Util::getColor(Util::TOO_LOW).name() + "; }");

        }
        /// Left is too soft   (diff < zoneRange*-1)
        else {
            qDebug() << "LEFT TOO STRONG**************";
            ui->label_unit_right->setStyleSheet("QLabel#label_unit_right { background-color :"+ Util::getColor(Util::TOO_LOW).name() + "; }");
            ui->label_unit_left->setStyleSheet("QLabel#label_unit_left { background-color :"+ Util::getColor(Util::TOO_HIGH).name() + "; }");
        }
    }
}




/////////////////////////////////////////////////////////////////////////////////////
void BalanceWidget::removeZone() {

    zoneRange = -1;
    zoneTarget = -1;

    ui->horizontalSlider->setStyleSheet("QSlider::groove:horizontal { background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0.0 rgb(35, 35, 35), stop:1.0 rgb(35, 35, 35)); }");

}

/////////////////////////////////////////////////////////////////////////////////////
void BalanceWidget::setZone(int target, int range) {



    if (target < range)
        target = range;
    if (target + range > 100)
        target = 100 - range;

    zoneRange = range;
    zoneTarget = target;

    int low = target - range + 0.01;
    int high = target + range - 0.01;

    double zoneLow = low / 100.0;
    double zoneHigh = high / 100.0;
    //    double target_d = target / 100.0;



    ui->horizontalSlider->setStyleSheet("QSlider::groove:horizontal { background: qlineargradient(x1:0, y1:0, x2:1, y2:0, "
                                        ///  0 to zMin
                                        "stop:0.0 " + Util::getColor(Util::DONE).name() + ", stop:"+ QString::number(zoneLow - 0.01) + " " +  Util::getColor(Util::DONE).name() +
                                        /// zoneTarget
                                        ", stop:" + QString::number(zoneLow) + " " +  Util::getColor(Util::SQUARE_POWER).name() + ", stop:" + QString::number(zoneHigh) + " " + Util::getColor(Util::SQUARE_POWER).name() +
                                        /// zMax to 1.0
                                        ", stop:" + QString::number(zoneHigh + 0.01) + " " + Util::getColor(Util::DONE).name() + ", stop:1.0 " +   Util::getColor(Util::DONE).name()  + "); }");

}




/////////////////////////////////////////////////////////////////////////////////////////////////////////
void BalanceWidget::pedalMetricChanged(double leftTorqueEff, double rightTorqueEff,
                                       double leftPedalSmooth, double rightPedalSmooth, double combinedPedalSmooth) {


    if (leftTorqueEff != -1) {
        ui->label_leftTorque->setText(QString::number(leftTorqueEff));
    }
    if (rightTorqueEff != -1) {
        ui->label_rightTorque->setText(QString::number(rightTorqueEff));
    }

    if (combinedPedalSmooth != -1) {
        ui->label_leftSmoothness->setVisible(false);
        ui->label_unitLeftSmooth->setVisible(false);
        ui->label_rightSmoothness->setVisible(false);
        ui->label_unitRightSmooth->setVisible(false);
        ui->label_combinedSmoothness->setText(QString::number(combinedPedalSmooth));
    }
    else {
        if (leftPedalSmooth != -1) {
            ui->label_leftSmoothness->setText(QString::number(leftPedalSmooth));
            ui->label_leftSmoothness->setVisible(true);
            ui->label_combinedSmoothness->setVisible(false);
            ui->label_unitCombinedSmooth->setVisible(false);
        }
        if (rightPedalSmooth != -1) {
            ui->label_rightSmoothness->setText(QString::number(rightPedalSmooth));
            ui->label_rightSmoothness->setVisible(true);
            ui->label_combinedSmoothness->setVisible(false);
            ui->label_unitCombinedSmooth->setVisible(false);
        }
    }



}
