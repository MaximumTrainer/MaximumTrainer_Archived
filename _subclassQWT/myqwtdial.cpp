#include "myqwtdial.h"
#include "qwt_round_scale_draw.h"
#include "util.h"
#include <QPainter>



myQwtDial::myQwtDial( QWidget* parent ): QwtDial( parent ) {

    d_label = tr("watts");


    scaleDraw()->enableComponent( QwtAbstractScaleDraw::Backbone, false );
    scaleDraw()->setPenWidth( 2 );

    setReadOnly(true);
    setTracking( false );
    setWrapping( false );


    setOrigin( 90.0 );
    setScaleArc( 36.0, 324.0 );


    setScaleMaxMajor( 12 );
    setScaleMaxMinor( 2 );

}


void myQwtDial::setLabelText(QString text) {

    d_label = text;
}


//////////////////////////////////////////////////////////////////////////////////////////////////
/*!
  Draw the contents inside the scale
  Paints nothing.

  \param painter Painter
  \param center Center of the contents circle
  \param radius Radius of the contents circle
*///////////////////////////////////////////////////////////////////////////////////////////////////
void myQwtDial::drawScaleContents( QPainter *painter, const QPointF &center, double radius ) const {


    QColor fillColorLinePerfect = Util::getColor(Util::SQUARE_CADENCE);
    QColor fillColor25 = Util::getColor(Util::SQUARE_POWER);
    QColor niceBlue = Util::getColor(Util::TOO_LOW);
    QColor brownColor = Util::getColor(Util::TOO_HIGH);



    QRectF rectangle(center.x()-radius, center.y()-radius, radius*2, radius*2);

    //180 GREEN OK +-25
    painter->setBrush(QBrush(fillColor25));
    painter->setPen( QPen(fillColor25, 1 ) );
    painter->drawPie(rectangle, (0 * 16), (180 * 16) );

    //1,8 MILIEU GREEN PERFECT +-1
    painter->setBrush(QBrush(fillColorLinePerfect));
    painter->setPen(QPen( fillColorLinePerfect, 1 ));
    painter->drawPie(rectangle, (89.1 * 16), (1.8 * 16) );


    //BLEU -25
    painter->setBrush(QBrush(niceBlue));
    painter->setPen(QPen( niceBlue, 1 ));
    painter->drawPie(rectangle, (180 * 16), (54 * 16) );

    //NORMAL COLOR DEADZONE
    painter->setBrush(QBrush(QColor( Qt::darkGray ).dark( 150 )));
    painter->setPen( QPen( QColor( Qt::darkGray ).dark( 150 ), 2 ) );
    painter->drawPie(rectangle, (234 * 16), (72 * 16) );

    //    setPalette( colorTheme( QColor( Qt::darkGray ).dark( 150 ) ) );
    //    palette.setColor( QPalette::Base, base );
    //    palette.setColor( QPalette::Window, base.dark( 150 ) );
    //    palette.setColor( QPalette::Mid, base.dark( 110 ) );
    //    palette.setColor( QPalette::Light, base.light( 170 ) );
    //    palette.setColor( QPalette::Dark, base.dark( 170 ) );
    //    palette.setColor( QPalette::Text, base.dark( 200 ).light( 800 ) );
    //    palette.setColor( QPalette::WindowText, base.dark( 200 ) );

    //ROUGE +25
    painter->setBrush(QBrush(brownColor));
    painter->setPen(QPen( brownColor, 1 ));
    painter->drawPie(rectangle, (306 * 16), (54 * 16) );


//    painter->setPen( Qt::NoPen );
    /// TODO: TEST  -- LABEL WATTS
    QRectF rect( 0.0, 0.0, 2.0 * radius, 2.0 * radius - 10.0 );
    rect.moveCenter( center );

    const QColor color = palette().color( QPalette::Text );
    painter->setPen( color );

    const int flags = Qt::AlignBottom | Qt::AlignHCenter;
    painter->drawText( rect, flags, d_label );

}





//void SpeedoMeter::drawScaleContents( QPainter *painter,
//    const QPointF &center, double radius ) const
//{
//    QRectF rect( 0.0, 0.0, 2.0 * radius, 2.0 * radius - 10.0 );
//    rect.moveCenter( center );

//    const QColor color = palette().color( QPalette::Text );
//    painter->setPen( color );

//    const int flags = Qt::AlignBottom | Qt::AlignHCenter;
//    painter->drawText( rect, flags, d_label );
//}
