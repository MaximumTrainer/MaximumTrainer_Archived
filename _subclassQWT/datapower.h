#ifndef DATAPOWER_H
#define DATAPOWER_H

#include <qrect.h>


class DataPower
{
public:
    static DataPower &instance();

    void append( const QPointF &pos );
    void clearStaleValues( double min );

    int size() const;
    QPointF value( int index ) const;

    QRectF boundingRect() const;

    void lock();
    void unlock();

    void clearData();


private:
    DataPower();
    DataPower( const DataPower & );
    DataPower &operator=( const DataPower & );

    virtual ~DataPower();

    class PrivateData;
    PrivateData *d_data;
};
#endif // DATAPOWER_H







