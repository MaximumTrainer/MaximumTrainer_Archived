#ifndef BOTTOMBAR_H
#define BOTTOMBAR_H

#include <QWidget>

namespace Ui {
class BottomBar;
}

class BottomBar : public QWidget
{
    Q_OBJECT

public:
    explicit BottomBar(QWidget *parent = 0);
    ~BottomBar();




public slots:
    void setGeneralMessage(const QString& text);
    void setGeneralMessage(const QString& text, int timeToDisplay);
    void removeGeneralMessage();

    void aboutANT_Stick_Not_Found();


    void updateHubStatus(int currentHubStarting);
    void hubStickFound(int numberFound, QString descriptionSticks);



private:
    Ui::BottomBar *ui;
};

#endif // BOTTOMBAR_H
