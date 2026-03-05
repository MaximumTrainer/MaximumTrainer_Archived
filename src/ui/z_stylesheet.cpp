#include "z_stylesheet.h"
#include "ui_z_stylesheet.h"

Z_StyleSheet::Z_StyleSheet(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::Z_StyleSheet)
{
    ui->setupUi(this);
}

Z_StyleSheet::~Z_StyleSheet()
{
    delete ui;
}
