#include "languagecombobox.h"

#include <QDebug>



LanguageComboBox::LanguageComboBox(QWidget *parent) : QComboBox(parent) {


    addItem("English");
    addItem("Français");

    this->model()->sort(Qt::AscendingOrder);

}
