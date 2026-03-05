#ifndef ICONITEM_H
#define ICONITEM_H

#include <QPainter>
#include "qwt_plot_item.h"

class IconItem : public QwtPlotItem
{
public:
    IconItem();
    void draw( QPainter *painter, const QwtScaleMap &, const QwtScaleMap &, const QRectF &canvasRect ) const;
    QString iconType;
};





#endif // ICONITEM_H

