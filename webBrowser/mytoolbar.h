#ifndef MYTOOLBAR_H
#define MYTOOLBAR_H

#include <QToolBar>
#include <QWidget>

class myToolBar : public QToolBar
{
    Q_OBJECT

public:
    myToolBar(QWidget *parent = 0);
};

#endif // MYTOOLBAR_H
