#ifndef MYDIALBOX_H
#define MYDIALBOX_H

#include <qwidget.h>
#include "myqwtdial.h"
#include <QLabel>
#include <QString>
#include <QGridLayout>

class QLabel;
class QwtDial;

class MyDialBox : public QWidget
{
    Q_OBJECT
public:
    MyDialBox(QWidget *parent);
    void setTypeSpeedo(QString type);


/////////////////////////////////////////////////////////////////////////////////////////////
//// SLOTS
/////////////////////////////////////////////////////////////////////////////////////////////
public slots:
    void setValue( int v );
    void targetChanged( int target );


private:
    void createDial();


/////////////////////////////////////////////////////////////////////////////////////////////
private:


    QGridLayout *gridLayout;
    myQwtDial *d_dial;
    QLabel *d_label_current_value;
    QLabel *d_label_icon;


    QPalette paletteGood;
    QPalette paletteToLow;
    QPalette paleteToHigh;


    int target;
};

#endif // MYDIALBOX_H

