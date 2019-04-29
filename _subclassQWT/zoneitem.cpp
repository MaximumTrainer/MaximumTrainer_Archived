#include "zoneitem.h"


/* Used to put background color indication on the graphs */
ZoneItem::ZoneItem( const QString &title )
{
    setTitle( title );
    setZ( 0 ); // on bottom the the grid
    setOrientation( Qt::Vertical );
    setItemAttribute( QwtPlotItem::Legend, true );
}

void ZoneItem::setColor( const QColor &color )
{
    QColor c = color;

//    c.setAlpha( 100 );
//    setPen( c );

//    c.setAlpha( 10 );
    setBrush( c );
}

//void ZoneItem::setInterval( const QDate &date1, const QDate &date2 )
//{
//    const QDateTime dt1( date1, QTime(), Qt::UTC );
//    const QDateTime dt2( date2, QTime(), Qt::UTC );

//    QwtPlotZoneItem::setInterval( QwtDate::toDouble( dt1 ),
//        QwtDate::toDouble( dt2 ) );
//}
