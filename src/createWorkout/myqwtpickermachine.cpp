#include "myqwtpickermachine.h"
#include <QEvent>
#include "qwt_event_pattern.h"
#include <QMouseEvent>

#include <QDebug>

//! Constructor
MyQwtPickerMachine::MyQwtPickerMachine():
    QwtPickerMachine( PointSelection )
{
    rightClickPressed = false;
}


//! Transition
QList<QwtPickerMachine::Command> MyQwtPickerMachine::transition(
        const QwtEventPattern &eventPattern, const QEvent *event )
{
    QList<QwtPickerMachine::Command> cmdList;

    switch ( event->type() )
    {
    case QEvent::MouseButtonPress:
    {
        if ( eventPattern.mouseMatch( QwtEventPattern::MouseSelect1,
                                      static_cast<const QMouseEvent *>( event ) ) )
        {
            cmdList += Begin;
            cmdList += Append;
            cmdList += End;
            rightClickPressed = false;
        }
        else if ( eventPattern.mouseMatch( QwtEventPattern::MouseSelect2,
                                           static_cast<const QMouseEvent *>( event ) ) )
        {
            cmdList += Begin;
            cmdList += Append;
            cmdList += End;
            rightClickPressed = true;
        }
        else if ( eventPattern.mouseMatch( QwtEventPattern::MouseSelect3,
                                           static_cast<const QMouseEvent *>( event ) ) )
        {
            cmdList += Begin;
            cmdList += Append;
            cmdList += End;
            rightClickPressed = true;
        }
        else {
            cmdList += Begin;
            cmdList += Append;
            cmdList += End;
            rightClickPressed = false;
        }
        break;
    }

    case QEvent::KeyPress:
    {
        if ( eventPattern.keyMatch( QwtEventPattern::KeySelect1,
                                    static_cast<const QKeyEvent *> ( event ) ) )
        {
            cmdList += Begin;
            cmdList += Append;
            cmdList += End;
        }
        break;
    }
    default:
        break;
    }

    return cmdList;
}
