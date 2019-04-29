#include "myqwtplotpicker.h"
#include "util.h"



MyQwtPlotPicker::MyQwtPlotPicker( int xAxis, int yAxis,
                                  RubberBand rubberBand, DisplayMode trackerMode,
                                  QWidget *canvas ):
    QwtPlotPicker( xAxis,  yAxis,
                   rubberBand,  trackerMode,
                   canvas )
{
}



/*!
    \brief Translate a position into a position string

    In case of HLineRubberBand the label is the value of the
    y position, in case of VLineRubberBand the value of the x position.
    Otherwise the label contains x and y position separated by a ',' .

    The format for the double to string conversion is "%.4f".

    \param pos Position
    \return Position string
  */
QwtText MyQwtPlotPicker::trackerTextF( const QPointF &pos ) const
{
    QString text;


    switch ( rubberBand() )
    {
    case HLineRubberBand:
        text.sprintf( "%.4f", pos.y() );
        break;
    case VLineRubberBand:
        text.sprintf( "%.4f", pos.x() );
        break;
    default:
//        double sec = pos.x()/60.0;
//        QTime time = Util::convertMinutesToQTime(sec);

        QTime time0(0,0,0);
        time0 = time0.addSecs(pos.x());


        QString timeToShow = Util::showQTimeAsString(time0);
        text = QString("%1, %2").arg(timeToShow).arg((int)pos.y());
//        text.sprintf( "%s, %.0f", timeToShow, pos.y() );
    }


    return QwtText( text );
}
