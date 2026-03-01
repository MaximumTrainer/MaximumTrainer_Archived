#include "dialog_connection_method.h"
#include "ui_dialog_connection_method.h"

DialogConnectionMethod::DialogConnectionMethod(bool antAvailable, QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::DialogConnectionMethod)
{
    ui->setupUi(this);
    setWindowTitle(tr("Connection Method"));

    // Pre-select ANT+ if a stick was found, otherwise pre-select BTLE
    if (antAvailable) {
        ui->radioButton_ant->setChecked(true);
    } else {
        ui->radioButton_ant->setEnabled(false);
        ui->radioButton_btle->setChecked(true);
    }
}

DialogConnectionMethod::~DialogConnectionMethod()
{
    delete ui;
}

void DialogConnectionMethod::accept()
{
    m_method = ui->radioButton_btle->isChecked() ? BTLE : ANTPlus;
    QDialog::accept();
}
