#ifndef SORTFILTERPROXYMODELCOURSE_H
#define SORTFILTERPROXYMODELCOURSE_H


#include <QSortFilterProxyModel>
#include "course.h"

class SortFilterProxyModelCourse : public QSortFilterProxyModel
{
    Q_OBJECT
public:
    explicit SortFilterProxyModelCourse(QObject *parent = 0);
    void setFilterKeyColumns(const QList<qint32> &filterColumns);
    void addFilterColumn(qint32 column);
    void addFilterFixedString(qint32 column, const QString &pattern);

    void addFilterCourseType(Course::COURSE_TYPE);


protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const;

private:
    QMap<qint32, QString> m_columnPatterns;


    bool showIncludedCourse;

};

#endif // SORTFILTERPROXYMODELCOURSE_H








