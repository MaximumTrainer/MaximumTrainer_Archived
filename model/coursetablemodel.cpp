#include "coursetablemodel.h"

#include <QDebug>
#include <QFont>
#include <QBrush>


CourseTableModel::CourseTableModel(QObject *parent) : QAbstractTableModel(parent) {

    qDebug() << "CourseTableModel";
    this->account = qApp->property("Account").value<Account*>();

    rowHovered = -2;
}


void CourseTableModel::changeRowHovered(int row) {
    this->rowHovered = row;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CourseTableModel::addListCourse(QList<Course> lstCourse) {

    beginResetModel();
    this->lstCourse.append(lstCourse);
    endResetModel();

    emit dataChanged();
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
Course CourseTableModel::getCourseAtRow(const QModelIndex &index) {

    return (lstCourse.at(index.row()));
}

// ------------------------------------------------------------------------------------------
int CourseTableModel::rowCount(const QModelIndex &parent) const {

    Q_UNUSED(parent);
    return lstCourse.size();
}
// ------------------------------------------------------------------------------------------
int CourseTableModel::columnCount(const QModelIndex &parent) const {

    Q_UNUSED(parent);
    return 5;
}


//--------------------------------------------------------------------------------------------------------
bool CourseTableModel::setData(const QModelIndex &index, const QVariant &value, int role) {


    if (role == Qt::UserRole) {
        Course course = value.value<Course>();
        lstCourse.replace(index.row(), course);
    }

    //    emit dataChanged();
    return true;
}




/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
QVariant CourseTableModel::data(const QModelIndex &index, int role) const {


    if (!index.isValid())
        return QVariant();

    if (index.row() >= lstCourse.size() || index.row() < 0)
        return QVariant();


    Course course = lstCourse.at(index.row());


    /// TOOLTIP
    if (role == Qt::ToolTipRole) {
        return "<qt/>"+course.getDescription();
    }

    ///Background
//    qDebug() << "courseNameIs:" << course.getName();
    if (role == Qt::BackgroundColorRole && account->hashCourseDone.contains(course.getName())) {
        //        QColor colorCadenceShapeTarget(150,150,150);
        QLinearGradient linearGrad(QPointF(0, 0), QPointF(0, 125));
        linearGrad.setColorAt(1, Qt::white);
        linearGrad.setColorAt(0, QColor(150,150,150));

        return QBrush(linearGrad);
    }


    ///BlueName for UserCreatedCourse
    if (role == Qt::TextColorRole && index.column() == 0 && course.getCourseType() == Course::USER_MADE) {
        return QVariant::fromValue(QColor(Qt::blue));
    }


    if (role == Qt::DisplayRole) {

        if(index.column() == 0) {
            return course.getName();
        }
        else if(index.column() == 1) {
            return course.getLocation();
        }
        else if(index.column() == 2) { //TOFIX HERE not showing correctly ?! wtf..
            return QString::number(course.getDistanceMeters()/1000, 'f', 3);
        }
        else if(index.column() == 3) {
            return course.getMaxSlopePercentage();
        }
        else if(index.column() == 4) {
            return qRound(course.getElevationDiff());
        }

        /// custom paint in delegate
        else {
            return tr("---");
        }
    }


    // Graph
    if (role == Qt::UserRole) {
        QVariant stored;
        stored.setValue(course);
        return stored;
    }

    return QVariant();
}



//////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CourseTableModel::deleteCourseWithName(QString name) {

    //TOFIX; Use hashtable instead QList, faster access
    for (int i=0; i<lstCourse.size(); i++)
    {
        Course course = lstCourse.at(i);
        if (course.getName() == name) {
            lstCourse.removeAt(i);
            break;
        }
    }
    emit dataChanged();
}


//---------------------------------------------------------------------
void CourseTableModel::deleteAllUserMadeCourses() {

    beginResetModel();
    QMutableListIterator<Course> i(lstCourse);
    while (i.hasNext()) {
        if (i.next().getCourseType() == Course::USER_MADE) {
            i.remove();
        }
    }
    endResetModel();
}



//////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CourseTableModel::addCourse(Course w) {

    beginResetModel();
    lstCourse.append(w);
    endResetModel();

    emit dataChanged();
}


//////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool CourseTableModel::removeRows(int position, int rows, const QModelIndex &index)
{
    qDebug() << "REMOVE ROW";
    Q_UNUSED(index);
    beginRemoveRows(QModelIndex(), position, position+rows-1);

    for (int row=0; row < rows; ++row) {
        lstCourse.removeAt(position);
    }
    endRemoveRows();
    emit dataChanged();
    return true;

}


//////////////////////////////////////////////////////////////////////////////////////////////////////////////
QVariant CourseTableModel::headerData(int section, Qt::Orientation  orientation, int role) const {


    if (role == Qt::ToolTipRole && orientation == Qt::Horizontal) {

        if (section == 0) {
            return tr("Name");
        }
        else if (section == 1) {
            return tr("Location");
        }
        else if (section == 2) {
            return tr("Distance");
        }
        else if (section == 3) {
            return tr("Max Slope %");
        }
        else if (section == 4) {
            return tr("Elevation");
        }
        else if (section == 5) {
            return tr("Graph");
        }
        else {
            return tr("colum not defined");
        }
    }

    if (role != Qt::DisplayRole)
        return QVariant();

    if (orientation == Qt::Vertical) {
        return QString::number(section);
    }
    else {
        if (section == 0) {
            return tr("Name");
        }
        else if (section == 1) {
            return tr("Location");
        }
        else if (section == 2) {
            return tr("Distance");
        }
        else if (section == 3) {
            return tr("Max Slope %");
        }
        else if (section == 4) {
            return tr("Elevation");
        }
        else if (section == 5) {
            return tr("Graph");
        }
        else {
            return tr("colum not defined");
        }
    }

}





