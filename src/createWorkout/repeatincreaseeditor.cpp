#include "repeatincreaseeditor.h"
#include "ui_repeatincreaseeditor.h"

#include <QDebug>


RepeatIncreaseEditor::~RepeatIncreaseEditor()
{
    delete ui;
}




RepeatIncreaseEditor::RepeatIncreaseEditor(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::RepeatIncreaseEditor)
{



    ui->setupUi(this);


    setAutoFillBackground(true);
    setAttribute(Qt::WA_NoMousePropagation);
    setWindowFlags(Qt::WindowStaysOnTopHint);





    ui->gridLayout->setRowMinimumHeight(0,80);
    ui->gridLayout->setRowMinimumHeight(1,10);

}


//////////////////////////////////////////////////////////////////////////////////////////
void RepeatIncreaseEditor::setInterval(const Interval &interval) {

    qDebug() << "RepeatIncreaseEditor start setInterval!";
    this->myInterval = interval;

    ui->doubleSpinBox_increaseFTP->setValue(myInterval.getRepeatIncreaseFTP());
    ui->spinBox_increaseCadence->setValue(myInterval.getRepeatIncreaseCadence());
    ui->doubleSpinBox_increaseLTHR->setValue(myInterval.getRepeatIncreaseLTHR());

    qDebug() << "setInterval RepeatIncreaseEditor done!";

}


//////////////////////////////////////////////////////////////////////////////////////////
void RepeatIncreaseEditor::on_pushButton_ok_clicked()
{
    emit endEdit();
}


//////////////////////////////////////////////////////////////////////////////////////////
void RepeatIncreaseEditor::on_doubleSpinBox_increaseFTP_valueChanged(double arg1)
{
    myInterval.setRepeatIncreaseFTP(arg1);

}

void RepeatIncreaseEditor::on_spinBox_increaseCadence_valueChanged(int arg1)
{
    myInterval.setRepeatIncreaseCadence(arg1);
}

void RepeatIncreaseEditor::on_doubleSpinBox_increaseLTHR_valueChanged(double arg1)
{
    myInterval.setRepeatIncreaseLTHR(arg1);
}

void RepeatIncreaseEditor::on_pushButton_default_clicked()
{
    this->myInterval.setRepeatIncreaseFTP(0);
    this->myInterval.setRepeatIncreaseCadence(0);
    this->myInterval.setRepeatIncreaseLTHR(0);
    emit endEdit();
}
