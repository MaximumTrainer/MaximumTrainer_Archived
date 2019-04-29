#ifndef INTERVALTABLEMODEL_H
#define INTERVALTABLEMODEL_H

#include <QAbstractTableModel>
#include <QPersistentModelIndex>
#include <QMimeData>
#include <QStringList>
#include <QModelIndexList>

#include "interval.h"
#include "account.h"

class IntervalTableModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    explicit IntervalTableModel(QObject *parent = 0);

    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    int columnCount(const QModelIndex &parent = QModelIndex()) const;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;


    QVariant data(const QModelIndex &index, int role) const;
    bool setData(const QModelIndex &index, const QVariant &value, int role);

    Qt::DropActions supportedDropActions() const;
    Qt::ItemFlags flags(const QModelIndex &index) const;
    QStringList mimeTypes() const;
    QMimeData* mimeData(const QModelIndexList &indexes) const;
    bool dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent);

    bool insertRows(int position, int rows, const QModelIndex &index);
    bool removeRows(int row, int count, const QModelIndex & parent);
    bool copyRows(int row);

    void removeIntervalRepeatData(int firstRow, int lastRow);



    void updateRowSourceInterval();
    Interval getIntervalAtRow(const QModelIndex &index);
    void setListInterval(QList<Interval> lstInterval);
    QList<Interval> getLstInterval();
    void resetLstInterval();

signals :
    void dataChanged();

private:
    Account *account;

    QList<Interval> lstInterval;

};

#endif // INTERVALTABLEMODEL_H
