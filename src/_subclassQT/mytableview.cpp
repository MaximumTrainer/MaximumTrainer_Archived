#include "mytableview.h"

#include <QDebug>
#include <sortfilterproxymodel.h>




MyTableView::MyTableView(QWidget *parent) : QTableView(parent) {

    this->setMouseTracking(true);


    m_background_delegate_hover = new delegateRowHover(true, this);
    m_background_delegate_notHover = new delegateRowHover(false, this);

    curr_row_hovered = -1;
}





void MyTableView::mouseMoveEvent(QMouseEvent* event) {

    QPoint pos = event->pos();
    QModelIndex index = indexAt(pos);
    int row = index.row();


    if (index.isValid() && row != curr_row_hovered) {

        SortFilterProxyModel *_model = static_cast<SortFilterProxyModel*>(model());

        for (int i=0; i<_model->rowCount(); i++) {
            for (int j=0; j<_model->columnCount(); j++) {

                if (i == row) {
                    QModelIndex inn = _model->index(i, j);
                    setItemDelegateForRow(inn.row(), m_background_delegate_hover);
                }
                else if (i == curr_row_hovered) {
                    QModelIndex inn = _model->index(i, j);
                    setItemDelegateForRow(inn.row(), m_background_delegate_notHover);
                }
            }
        }
        curr_row_hovered = row;
    }


    else if (!index.isValid() && curr_row_hovered != -1 ){

        SortFilterProxyModel *_model = static_cast<SortFilterProxyModel*>(model());
        //        qDebug() << "mouse index not valid";

        for (int i=0; i<_model->rowCount(); i++) {
            for (int j=0; j<_model->columnCount(); j++) {

                if (i == curr_row_hovered)  {
                    QModelIndex inn = _model->index(i, j);
                    setItemDelegateForRow(inn.row(), m_background_delegate_notHover);
                }
            }
        }
        curr_row_hovered = -1;
    }
    QTableView::mouseMoveEvent(event);
}



void MyTableView::leaveEvent(QEvent *event) {

    if (curr_row_hovered != -1 ) {

        SortFilterProxyModel *_model = static_cast<SortFilterProxyModel*>(model());

        for (int i=0; i<_model->rowCount(); i++) {
            for (int j=0; j<_model->columnCount(); j++) {

                if (i == curr_row_hovered)  {
                    QModelIndex inn = _model->index(i, j);
                    setItemDelegateForRow(inn.row(), m_background_delegate_notHover);
                }
            }
        }
        curr_row_hovered = -1;
    }
    QTableView::leaveEvent(event);
}




