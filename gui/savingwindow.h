#ifndef SAVINGWINDOW_H
#define SAVINGWINDOW_H

#include <QWidget>

namespace Ui {
class SavingWindow;
}

class SavingWindow : public QWidget
{
    Q_OBJECT

public:
    explicit SavingWindow(QWidget *parent = 0);
    ~SavingWindow();

    void setMessage(QString msg);

private:
    Ui::SavingWindow *ui;
};

#endif // SAVINGWINDOW_H
