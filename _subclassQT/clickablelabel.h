#ifndef CLICKABLELABEL_H
#define CLICKABLELABEL_H

#include <QLabel>




class ClickableLabel : public QLabel
{
    Q_OBJECT

public:

    explicit ClickableLabel(QWidget *parent = 0, Qt::WindowFlags f = 0);


    void setVolumeLabel(bool isVolumeLabel) {
        this->isVolumeLabel = isVolumeLabel;
    }

//-------------------------------------------------------
signals:
    void clicked(bool stateClicked);

public slots:


protected:
    void mousePressEvent ( QMouseEvent * event ) ;

private:
    bool stateClicked;
    bool isVolumeLabel;


};

#endif // CLICKABLELABEL_H
