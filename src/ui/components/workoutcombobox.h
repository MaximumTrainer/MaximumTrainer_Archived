#ifndef WORKOUTCOMBOBOX_H
#define WORKOUTCOMBOBOX_H

#include <QEvent>
#include <QComboBox>

class WorkoutComboBox : public QComboBox
{
     Q_OBJECT

public:
    explicit WorkoutComboBox(QWidget *parent = 0);

    void changeEvent(QEvent *event);

private:
    void updateUi();
};

#endif // WORKOUTCOMBOBOX_H
