#include "addworkoutwidget.h"
#include "ui_addworkoutwidget.h"
#include <QDebug>

AddWorkoutWidget::AddWorkoutWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::AddWorkoutWidget)
{
    ui->setupUi(this);
}

AddWorkoutWidget::~AddWorkoutWidget()
{
    delete ui;
}

void AddWorkoutWidget::on_pushButton_addWorkout_clicked()
{
    emit addWorkoutClicked();
}
