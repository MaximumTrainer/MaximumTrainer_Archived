#ifndef MYQWTDIAL_H
#define MYQWTDIAL_H

#include "qwt_dial.h"


class myQwtDial : public QwtDial
{
    Q_OBJECT
    

public:
    explicit myQwtDial( QWidget *parent = NULL );
    void setLabelText(QString text);


private :
    void drawScaleContents( QPainter *painter, const QPointF &center, double radius ) const;
    

private :

    QString d_label;
};

#endif // MYQWTDIAL_H
