#include "clickablelabel.h"

#include <QDebug>



ClickableLabel::ClickableLabel(QWidget * parent, Qt::WindowFlags f):
    QLabel(parent)
{
    Q_UNUSED(f);
    stateClicked = false;

    isVolumeLabel = false;


}



void ClickableLabel::mousePressEvent ( QMouseEvent * event ) {

    Q_UNUSED(event);

    stateClicked = !stateClicked;
    emit clicked(stateClicked);

    if (isVolumeLabel) {
        if (stateClicked)
            this->setStyleSheet("image: url(:/image/icon/volume_mute);");
        else
            this->setStyleSheet("image: url(:/image/icon/volume);");
    }


}
