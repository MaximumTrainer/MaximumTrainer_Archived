#include "oxygenwidget.h"
#include "ui_oxygenwidget.h"

OxygenWidget::OxygenWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::OxygenWidget)
{
    ui->setupUi(this);
}

OxygenWidget::~OxygenWidget()
{
    delete ui;
}



void OxygenWidget::oxygenValueChanged(double percentageSaturatedHemoglobin, double totalHemoglobinConcentration) {     // %, g/d;


    if(percentageSaturatedHemoglobin < 0) {
        ui->label_saturatedHemo->setText("-");
    }
    else {
        ui->label_saturatedHemo->setText(QString::number(qRound(percentageSaturatedHemoglobin)) + "%");
    }

    if(totalHemoglobinConcentration < 0) {
        ui->label_hemoConc->setText("-");
    }
    else {
        ui->label_hemoConc->setText(QString::number(totalHemoglobinConcentration, 'f', 1));
    }





}




