#include "curvedatacadence.h"
#include "datacadence.h"

const DataCadence &CurveDataCadence::values() const
{
    return DataCadence::instance();
}

DataCadence &CurveDataCadence::values()
{
    return DataCadence::instance();
}

QPointF CurveDataCadence::sample( size_t i ) const
{
    return DataCadence::instance().value( i );
}

size_t CurveDataCadence::size() const
{
    return DataCadence::instance().size();
}

QRectF CurveDataCadence::boundingRect() const
{
    return DataCadence::instance().boundingRect();
}

