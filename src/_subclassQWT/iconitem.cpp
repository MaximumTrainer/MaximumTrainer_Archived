#include "iconitem.h"

IconItem::IconItem() {}




void IconItem::draw( QPainter *painter,
                               const QwtScaleMap &, const QwtScaleMap &,
                               const QRectF &canvasRect ) const

{

    QPixmap pixmap;
    if (iconType == "POWER") {
        pixmap = QPixmap(":/image/icon/power2");
    }
    else if (iconType == "CADENCE") {
        pixmap = QPixmap(":/image/icon/crank2");
    }
    else if (iconType == "HEART_RATE") {
        pixmap = QPixmap(":/image/icon/heart2");
        pixmap = pixmap.scaled(35,35, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    }
    else if (iconType == "SPEED") {
        pixmap = QPixmap(":/image/icon/speed");
    }
//    else if (iconType == "DONE_CHECK") {
//        pixmap = QPixmap(":/image/icon/check");
//    }
//    pixmap = pixmap.scaled(QSize(30,30),  Qt::KeepAspectRatio);


    QPointF p(canvasRect.center().x()+30, canvasRect.top());
    painter->drawPixmap(p, pixmap);

}
