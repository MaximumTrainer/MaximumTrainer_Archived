#ifndef INTERVALVIEWSTYLE_H
#define INTERVALVIEWSTYLE_H

#include <QProxyStyle>
#include <QStyle>
#include <QStyleOption>
#include <QPainter>

class IntervalViewStyle : public QProxyStyle
{

public:
    IntervalViewStyle(QStyle* style = 0);
    void drawPrimitive(PrimitiveElement element, const QStyleOption * option, QPainter * painter, const QWidget * widget = 0) const;
};

#endif // INTERVALVIEWSTYLE_H


