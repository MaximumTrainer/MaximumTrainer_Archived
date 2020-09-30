#ifndef MYTABLEVIEW_H
#define MYTABLEVIEW_H

#include <QTableView>
#include <QMouseEvent>
#include <QWheelEvent>
#include "delegaterowhover.h"


class MyTableView : public QTableView
{
    Q_OBJECT

public:
    explicit MyTableView(QWidget *parent = 0);




private :
    void mouseMoveEvent(QMouseEvent* event);
    void leaveEvent(QEvent *event);


    //    void focusOutEvent(QFocusEvent *event);

    delegateRowHover *m_background_delegate_hover;
    delegateRowHover *m_background_delegate_notHover;
    int curr_row_hovered;



};

#endif // MYTABLEVIEW_H
