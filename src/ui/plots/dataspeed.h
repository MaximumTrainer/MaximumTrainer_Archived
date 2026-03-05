#ifndef DATASPEED_H
#define DATASPEED_H

#include <qrect.h>

class DataSpeed
{
public:
    static DataSpeed &instance();

    void append( const QPointF &pos );
    void clearStaleValues( double min );

    int size() const;
    QPointF value( int index ) const;

    QRectF boundingRect() const;

    void lock();
    void unlock();

    void clearData();


private:
    DataSpeed();
    DataSpeed( const DataSpeed & );
    DataSpeed &operator=( const DataSpeed & );

    virtual ~DataSpeed();

    class PrivateData;
    PrivateData *d_data;
};

#endif // DATASPEED_H










