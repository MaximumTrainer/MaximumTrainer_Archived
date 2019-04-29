#ifndef COURSETABLEMODEL_H
#define COURSETABLEMODEL_H


#include <QAbstractTableModel>
#include <QPersistentModelIndex>

#include "course.h"
#include "account.h"


class CourseTableModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    CourseTableModel(QObject *parent = 0);
    void addListCourse(QList<Course> lstCourse);

    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    int columnCount(const QModelIndex &parent = QModelIndex()) const;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;

    QVariant data(const QModelIndex &index, int role) const;
    bool setData(const QModelIndex &index, const QVariant &value, int role);

    Course getCourseAtRow(const QModelIndex &index);
    void deleteCourseWithName(QString name);
    void deleteAllUserMadeCourses();
    void addCourse(Course course);

    bool removeRows(int row, int count, const QModelIndex & parent);

signals :
    void dataChanged();


public slots :
    void changeRowHovered(int row);

private:
    QList<Course> lstCourse;
    int rowHovered;

    Account *account; //to keep reference of QSet course done


};

#endif // COURSETABLEMODEL_H
