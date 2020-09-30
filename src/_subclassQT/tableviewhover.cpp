#include "tableviewhover.h"

#include <QDebug>
#include <QHeaderView>
#include <QScrollBar>
#include <QEvent>

TableViewHover::TableViewHover(QWidget *parent) :
    QTableView(parent)
{

    curr_row_selected = -1;
    curr_row_hovered = -1;
    bandHover = new QRubberBand(QRubberBand::Rectangle, this);
    bandHover->setVisible(false);

    this->setMouseTracking(true);
    //    installEventFilter(this);
}



//////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool TableViewHover::eventFilter(QObject *watched, QEvent *event) {


    ///Check if mouse is on the scroll bar, remove hover effect on the rows
    QScrollBar *ptrScroll = qobject_cast<QScrollBar*>(watched);

    if (ptrScroll != NULL && (event->type() == QEvent::HoverEnter || event->type() == QEvent::HoverMove) ) {
//        qDebug() << "got here EventFilter...";
        bandHover->setVisible(false);
        curr_row_hovered = -1;
    }

    return false;
}


//////////////////////////////////////////////////////////////////////////////////////////////////////////////
void TableViewHover::selectionChanged (const QItemSelection & selected, const QItemSelection & deselected ) {

//    qDebug() << "selectionChanged TableViewHover!";

    QModelIndexList lstSelected = selected.indexes();
//    qDebug() << lstSelected.size();

    if (lstSelected.size() < 1) {
        this->selectionModel()->clearSelection();
        curr_row_selected = -1;
        this->repaint();
    }
    else {
        QModelIndex modelIndex = lstSelected.at(0);
        curr_row_selected = modelIndex.row();
        QTableView::selectionChanged(selected, deselected);
    }





}


//////////////////////////////////////////////////////////////////////////////////////////////////////////////
void TableViewHover::mouseMoveEvent(QMouseEvent* event) {


    //fdsafsdfdsaf fix rectangle map to parent grid only not header?

    QPoint pos = event->pos();
    QModelIndex index = indexAt(pos);
    int row = index.row();

    /// Hover row
    if (index.isValid() && row != curr_row_hovered) {


        if (row ==  curr_row_selected) {
            bandHover->setVisible(false);
            return;
        }

        QModelIndex newIndex =  this->model()->index(row, 0,  QModelIndex());
        int headerHeight = this->horizontalHeader()->size().height();

        QRect recIndex(this->visualRect(newIndex));

#ifdef Q_OS_MAC
        recIndex.setWidth(this->width());
#else
        if (this->verticalScrollBar()->isVisible())
            recIndex.setWidth(this->width()-20);
        else
            recIndex.setWidth(this->width());
#endif
        QPoint topLeft = recIndex.topLeft();
        QPoint tempPt(topLeft);
        topLeft.setY(tempPt.y() + headerHeight);


        curr_row_hovered = row;
        bandHover->move(topLeft);
        bandHover->resize(recIndex.size().width(), recIndex.size().height());
        bandHover->setVisible(true);
    }
    else if (!index.isValid() && curr_row_hovered != -1 ){

        curr_row_hovered = -1;
        bandHover->setVisible(false);
    }

    QTableView::mouseMoveEvent(event);
}


//////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// leaveEvent - Fire when mouse leave the widget
//////////////////////////////////////////////////////////////////////////////////////////////////////////////
void TableViewHover::leaveEvent(QEvent *event) {

//    qDebug() << "LeaveEvent" << curr_row_hovered;


    if (curr_row_hovered != -1) {
        bandHover->setVisible(false);
        curr_row_hovered = -1;

    }



    QTableView::leaveEvent(event);
}



//-------------------------------------------------------------
void TableViewHover::focusOutEvent(QFocusEvent* event) {


//    qDebug() << "FOCUS OUT EVENT" << curr_row_hovered << "EventType" << event;

    //avoid right clicking lost focus on row
    if (event->reason() == Qt::PopupFocusReason )
        return;


    if (curr_row_selected != -1) {
        this->selectionModel()->clearSelection();
        curr_row_selected = -1;
    }
    if (curr_row_hovered != -1) {
        bandHover->setVisible(false);
        curr_row_hovered = -1;
    }


    QTableView::focusOutEvent(event);

}
