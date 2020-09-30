#ifndef UPDATEDIALOG_H
#define UPDATEDIALOG_H

#include <QDialog>

namespace Ui {
class UpdateDialog;
}

class UpdateDialog : public QDialog
{
    Q_OBJECT

public:
    explicit UpdateDialog(double versionNumber, QWidget *parent = 0);
    ~UpdateDialog();
//    void reject();
    void accept();



private:
    Ui::UpdateDialog *ui;


};

#endif // UPDATEDIALOG_H
