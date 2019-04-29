#ifndef MYQWTPICKERMACHINE_H
#define MYQWTPICKERMACHINE_H

#include "qwt_picker_machine.h"

class MyQwtPickerMachine : public QwtPickerMachine
{
public:
    MyQwtPickerMachine();

    virtual QList<Command> transition(
        const QwtEventPattern &, const QEvent * );

    bool rightClickPressed;
};

#endif // MYQWTPICKERMACHINE_H



