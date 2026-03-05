#include "intervalviewstyle.h"
#include <QPen>
#include <QBrush>


IntervalViewStyle::IntervalViewStyle(QStyle* style)
    :QProxyStyle(style)
{}


void IntervalViewStyle::drawPrimitive ( PrimitiveElement element, const QStyleOption * option, QPainter * painter, const QWidget * widget) const{
    if (element == QStyle::PE_IndicatorItemViewItemDrop && !option->rect.isNull())
    {
        QPen pen;
        pen.setWidth(3);
        pen.setColor(Qt::red);
        painter->setPen(pen);

        QStyleOption opt(*option);
        opt.rect.setLeft(0);
        if (widget) opt.rect.setRight(widget->width());
        QProxyStyle::drawPrimitive(element, &opt, painter, widget);
        return;
    }
    QProxyStyle::drawPrimitive(element, option, painter, widget);
}

