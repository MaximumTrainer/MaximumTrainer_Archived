#include "curvedatapower.h"
#include "datapower.h"

const DataPower &CurveDataPower::values() const
{
    return DataPower::instance();
}

DataPower &CurveDataPower::values()
{
    return DataPower::instance();
}

QPointF CurveDataPower::sample( size_t i ) const
{
    return DataPower::instance().value( i );
}

size_t CurveDataPower::size() const
{
    return DataPower::instance().size();
}

QRectF CurveDataPower::boundingRect() const
{
    return DataPower::instance().boundingRect();
}

