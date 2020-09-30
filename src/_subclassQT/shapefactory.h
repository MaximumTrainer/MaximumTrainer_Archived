#ifndef SHAPEFACTORY_H
#define SHAPEFACTORY_H

#include <qpainterpath.h>


namespace ShapeFactory
{
    enum Shape
    {
        Rect,
        Parallelogram,
        Triangle,
        Ellipse,
        Ring,
        Star,
        Hexagon
    };

    QPainterPath path( Shape, const QPointF &, const QSizeF & );
    QPainterPath path( QList<QPointF> lstPoints );

};

#endif  // SHAPEFACTORY_H
