#include "workoutcombobox.h"
#include "workout.h"

#include <QDebug>




WorkoutComboBox::WorkoutComboBox(QWidget *parent) : QComboBox(parent) {

    updateUi();
}


/////////////////////////////////////////////////////////////////////////////////////////////////
void WorkoutComboBox::updateUi() {

    this->clear();
    addItem("", QVariant() );
//    addItem(Workout::getTypeToString(Workout::T_ENDURANCE), Workout::T_ENDURANCE);
//    addItem(Workout::getTypeToString(Workout::T_TEMPO), Workout::T_TEMPO);
//    addItem(Workout::getTypeToString(Workout::T_TEST), Workout::T_TEST);


    this->model()->sort(Qt::AscendingOrder);

//    addItem(Workout::getTypeToString(Workout::T_OTHERS), Workout::T_OTHERS);


}

/////////////////////////////////////////////////////////////////////////////////////////////////
void WorkoutComboBox::changeEvent(QEvent *event) {

    if (event->type() == QEvent::LanguageChange) {
        updateUi();
    }
    else {
        QWidget::changeEvent(event);
    }
}


