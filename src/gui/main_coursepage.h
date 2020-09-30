#ifndef MAIN_COURSEPAGE_H
#define MAIN_COURSEPAGE_H

#include <QWidget>
#include <QMenu>
#include <QAction>

#include "coursetablemodel.h"
#include "delegaterowhover.h"
#include "radio.h"
#include "settings.h"
#include "xmlutil.h"
#include "sortfilterproxymodelcourse.h"

namespace Ui {
class Main_CoursePage;
}

class Main_CoursePage : public QWidget
{
    Q_OBJECT

public:
    explicit Main_CoursePage(QWidget *parent = 0);
    ~Main_CoursePage();

    void parseIncludedCourses();
    void parseUserCourses();

    void paintEvent(QPaintEvent *);


signals :

    void editCourse(Course);



public slots:

    void tableViewSelectionChanged(QItemSelection,QItemSelection);
    void customMenuRequested(QPoint);
    void refreshUserCourse();


private slots:

    void addJSObject_CoursePage();

    void filterChangedCourseType(bool includeCourse);

    void editCourse();
    void deleteCourse();
    void setAsDone();
    void openFolderCourse();

    void on_pushButton_refresh_clicked();
    void on_checkBox_showIncludeCourse_clicked(bool checked);

private:
    Ui::Main_CoursePage *ui;

    XmlUtil *xmlUtil;
    Account *account;
    Settings *settings;

    CourseTableModel *tableModel;
    SortFilterProxyModelCourse *proxyModel;
    delegateRowHover *delegateRow;

    QMenu *contextMenu;
    QAction *actionEdit;
    QAction *actionDelete;
    QAction *actionSetAsDone;
    QAction *actionOpenFolder;
    QModelIndex indexSourceSelected;
};

#endif // MAIN_COURSEPAGE_H
