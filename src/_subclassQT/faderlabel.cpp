#include "faderlabel.h"

#include <QDebug>


FaderLabel::FaderLabel(QWidget *parent) :
    QLabel(parent)
{

    timerAnimatorFadeOut = new QTimer(this);
    timerAnimatorFadeOut->setSingleShot(true);

    connect(timerAnimatorFadeOut, SIGNAL(timeout()), this, SLOT(fadeOutAfterTimer()) );


    effect = new QGraphicsOpacityEffect(this);
    this->setGraphicsEffect(effect);
    anim = new QPropertyAnimation(effect, "opacity", this);


    tmpDurationFadeOut = 0;
    stateClicked = false;
}



//---------------------------------------------------
void FaderLabel::fadeOutAfterTimer() {

    fadeOut(this->tmpDurationFadeOut);
}

//---------------------------------------------------------------------------------
void FaderLabel::fadeIn(int durationMs) {

//    qDebug() << "animateFadeIn FaderLabel" << durationMs;
    anim->stop();
    anim->setDuration(durationMs);
    anim->setStartValue(0);
    anim->setEndValue(1);
    anim->setEasingCurve(QEasingCurve::OutQuad);
    anim->start();


}

//---------------------------------------------------------------------------------
void FaderLabel::fadeOut(int durationMs) {

//    qDebug() << "animateFadeout FaderLabel" << durationMs;
    anim->stop();
    anim->setDuration(durationMs);
    anim->setStartValue(1);
    anim->setEndValue(0);
    anim->setEasingCurve(QEasingCurve::OutQuad);
    anim->start();
}


//--------------------------------------------------------------------------------
void FaderLabel::fadeOutAfterPause(int fadeDuration, int pause) {

    this->tmpDurationFadeOut = fadeDuration;
    timerAnimatorFadeOut->start(pause);

}

//-------------------------------------------------------------------------------------------------
void FaderLabel::fadeInAndFadeOutAfterPause(int fadeInDuration, int fadeOutDuration, int pause) {

    fadeIn(fadeInDuration);

    this->tmpDurationFadeOut = fadeOutDuration;
    timerAnimatorFadeOut->start(pause);

}

//-----------------------------------------------------------------------------------------------------
void FaderLabel::mousePressEvent ( QMouseEvent * event ) {

    Q_UNUSED(event);

    stateClicked = !stateClicked;
    emit clicked(stateClicked);

}
