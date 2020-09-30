#ifndef OXYGENWIDGET_H
#define OXYGENWIDGET_H

#include <QWidget>

namespace Ui {
class OxygenWidget;
}

class OxygenWidget : public QWidget
{
    Q_OBJECT

public:
    explicit OxygenWidget(QWidget *parent = 0);
    ~OxygenWidget();


public slots:
     void oxygenValueChanged(double percentageSaturatedHemoglobin, double totalHemoglobinConcentration);  // %, g/d;

private:
    Ui::OxygenWidget *ui;
};

#endif // OXYGENWIDGET_H
