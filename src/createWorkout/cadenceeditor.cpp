#include "cadenceeditor.h"
#include "ui_cadenceeditor.h"
#include <QDebug>

CadenceEditor::~CadenceEditor()
{
    delete ui;
}

CadenceEditor::CadenceEditor(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::CadenceEditor)
{

    ui->setupUi(this);




#ifdef Q_OS_WIN32
    ui->comboBox_cadence->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
#endif
#ifdef Q_OS_MAC
    ui->comboBox_cadence->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
#endif



    setAutoFillBackground(true);
    setAttribute(Qt::WA_NoMousePropagation);
    setWindowFlags(Qt::WindowStaysOnTopHint);



    ui->gridLayout->setRowMinimumHeight(0,45);
    ui->gridLayout->setRowMinimumHeight(1,45);
}



//////////////////////////////////////////////////////////////////////////////////////////
void CadenceEditor::setInterval(const Interval &interval) {
    this->myInterval = interval;

    qDebug() << "CadenceStart is now" << myInterval.getCadence_start();

    ui->comboBox_cadence->setCurrentIndex(myInterval.getCadenceStepType());
    on_comboBox_cadence_currentIndexChanged(ui->comboBox_cadence->currentIndex());

    ui->spinBox_cadenceStart->setValue(myInterval.getCadence_start());
    ui->spinBox_cadenceEnd->setValue(myInterval.getCadence_end());
    ui->spinBox_rangeCadence->setValue(myInterval.getCadence_range());

}

/////////////////////////////////////////////////////////////////////////////////
void CadenceEditor::on_comboBox_cadence_currentIndexChanged(int index)
{
    Interval::StepType typeStep = static_cast<Interval::StepType>(index);
    myInterval.setCadenceStepType(typeStep);


    /// change interface (hide/show widgets)
    if (index == 0) { ///None
        ui->spinBox_cadenceStart->setVisible(false);
        ui->label_toCadenceTxt->setVisible(false);
        ui->spinBox_cadenceEnd->setVisible(false);
        ui->label_cadenceRpmTxt->setVisible(false);
        ui->label_acceptedCadence->setVisible(false);
        ui->spinBox_rangeCadence->setVisible(false);
        ui->label_accptedRpmTxt->setVisible(false);
    }
    else if (index == 1) { ///FLAT
        ui->spinBox_cadenceEnd->setVisible(false);
        ui->label_toCadenceTxt->setVisible(false);

        ui->spinBox_cadenceStart->setVisible(true);
        ui->label_cadenceRpmTxt->setVisible(true);
        ui->label_acceptedCadence->setVisible(true);
        ui->spinBox_rangeCadence->setVisible(true);
        ui->label_accptedRpmTxt->setVisible(true);
    }
    else { ///2- Progressive
        ui->spinBox_cadenceStart->setVisible(true);
        ui->label_toCadenceTxt->setVisible(true);
        ui->spinBox_cadenceEnd->setVisible(true);
        ui->label_cadenceRpmTxt->setVisible(true);
        ui->label_acceptedCadence->setVisible(true);
        ui->spinBox_rangeCadence->setVisible(true);
        ui->label_accptedRpmTxt->setVisible(true);
    }
}


void CadenceEditor::on_spinBox_cadenceStart_valueChanged(int arg1)
{
    myInterval.setTargetCadence_start(arg1);
}

void CadenceEditor::on_spinBox_cadenceEnd_valueChanged(int arg1)
{
    myInterval.setTargetCadence_end(arg1);
}

void CadenceEditor::on_spinBox_rangeCadence_valueChanged(int arg1)
{
    qDebug() << "on_spinBox_rangeCadence_valueChanged" << arg1;
    myInterval.setTargetCadence_range(arg1);
}

void CadenceEditor::on_pushButton_ok_clicked()
{
    emit endEdit();
}

void CadenceEditor::on_pushButton_default_clicked()
{
    this->myInterval.setCadenceStepType(Interval::StepType::NONE);
    this->myInterval.setTargetCadence_start(90);
    this->myInterval.setTargetCadence_end(90);
    this->myInterval.setTargetCadence_range(5);
    emit endEdit();

}
