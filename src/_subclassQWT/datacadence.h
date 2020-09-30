#ifndef DATACADENCE_H
#define DATACADENCE_H

#include <qrect.h>

class DataCadence
{
public:
    static DataCadence &instance();

    void append( const QPointF &pos );
    void clearStaleValues( double min );

    int size() const;
    QPointF value( int index ) const;

    QRectF boundingRect() const;

    void lock();
    void unlock();

    void clearData();


private:
    DataCadence();
    DataCadence( const DataCadence & );
    DataCadence &operator=( const DataCadence & );

    virtual ~DataCadence();

    class PrivateData;
    PrivateData *d_data;
};

#endif // DATACADENCE_H




