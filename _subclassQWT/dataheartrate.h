#ifndef DATAHEARTRATE_H
#define DATAHEARTRATE_H

#include <qrect.h>

class DataHeartRate
{
public:
    static DataHeartRate &instance();

    void append( const QPointF &pos );
    void clearStaleValues( double min );

    int size() const;
    QPointF value( int index ) const;

    QRectF boundingRect() const;

    void lock();
    void unlock();

    void clearData();


private:
    DataHeartRate();
    DataHeartRate( const DataHeartRate & );
    DataHeartRate &operator=( const DataHeartRate & );

    virtual ~DataHeartRate();

    class PrivateData;
    PrivateData *d_data;
};

#endif // DATAHEARTRATE_H







