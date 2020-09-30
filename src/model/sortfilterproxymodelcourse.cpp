#include "sortfilterproxymodelcourse.h"

#include <QDebug>






SortFilterProxyModelCourse::SortFilterProxyModelCourse(QObject *parent) : QSortFilterProxyModel(parent)
{

    showIncludedCourse = true;
}


//----------------------------------------------------------------------------------
void SortFilterProxyModelCourse::setFilterKeyColumns(const QList<qint32> &filterColumns)
{
    m_columnPatterns.clear();

    foreach(qint32 column, filterColumns)
        m_columnPatterns.insert(column, QString());
}

//----------------------------------------------------------------------------------
void SortFilterProxyModelCourse::addFilterColumn(qint32 column) {

    m_columnPatterns.insert(column, QString());
}


//----------------------------------------------------------------------------------
void SortFilterProxyModelCourse::addFilterFixedString(qint32 column, const QString &pattern)
{
    if(!m_columnPatterns.contains(column)) {
        return;
    }

    m_columnPatterns[column] = pattern;

}


//----------------------------------------------------------------------------------
void SortFilterProxyModelCourse::addFilterCourseType(Course::COURSE_TYPE courseType) {

    if (courseType == Course::COURSE_TYPE::INCLUDED) {
        showIncludedCourse = true;
    }
    else {
        showIncludedCourse = false;
    }

}



//----------------------------------------------------------------------------------
bool SortFilterProxyModelCourse::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const
{

    if(m_columnPatterns.isEmpty())
        return true;



    ///Check Id of workout
    QModelIndex index = sourceModel()->index(sourceRow, 2, sourceParent);
    QVariant variant = sourceModel()->data(index, Qt::UserRole);
    Course course;
    course = variant.value<Course>();
    //        qDebug() << "Course NAME I" << course.getName()<< "typecrouse" << course.getCourseType();

    if (!showIncludedCourse && course.getCourseType() == Course::COURSE_TYPE::INCLUDED)
        return false;
    //--------------


    bool ret = false;

    for(QMap<qint32, QString>::const_iterator iter = m_columnPatterns.constBegin();
        iter != m_columnPatterns.constEnd();
        ++iter)
    {
        QModelIndex index = sourceModel()->index(sourceRow, iter.key(), sourceParent);
        ret = index.data().toString().startsWith(iter.value(), Qt::CaseInsensitive);
        //        compare ( const QString & other, Qt::CaseSensitivity cs ) const
        //        contains ( const QString & str, Qt::CaseSensitivity cs = Qt::CaseSensitive ) const
        //        startsWith ( const QString & s, Qt::CaseSensitivity cs = Qt::CaseSensitive ) const

        //        QString data = index.data().toString();
        //        QString key = iter.value();
        //        bool resultat =  data.startsWith(key, Qt::CaseInsensitive);
        //        qDebug() << "startWITH?:" << resultat;


        //        qDebug() << "data:" << index.data().toString();
        //        qDebug() << "value:" << iter.value();
        //        qDebug() << "equals?:" << ret;
        if(!ret)
            return ret;
    }

    return ret;
}



