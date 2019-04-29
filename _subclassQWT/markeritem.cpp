#include "markeritem.h"
#include "util.h"

MarkerItem::MarkerItem()
{
}





void MarkerItem::draw( QPainter *painter,
                               const QwtScaleMap &, const QwtScaleMap &,
                               const QRectF &canvasRect ) const

{


    QPen p( Qt::white, 1 );
    p.setStyle(Qt::DotLine);
    painter->setPen(p);

    painter->drawLine( canvasRect.center().x(), canvasRect.top()+35,  canvasRect.center().x(), canvasRect.bottom() );


}
