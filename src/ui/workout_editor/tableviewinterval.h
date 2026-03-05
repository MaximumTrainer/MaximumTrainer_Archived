#ifndef TABLEVIEWINTERVAL_H
#define TABLEVIEWINTERVAL_H

#include <QTableView>
#include <QMouseEvent>
#include <QDragEnterEvent>

class TableViewInterval : public QTableView
{
    Q_OBJECT

public:
    explicit TableViewInterval(QWidget *parent = 0);



signals:

public slots:
    //    void setCursor(QModelIndex);


private :
    void mouseMoveEvent(QMouseEvent* event);
    void leaveEvent(QEvent *event);
    bool eventFilter(QObject *watched, QEvent *event);



};

#endif // TABLEVIEWINTERVAL_H






