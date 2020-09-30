#include "faderframe.h"

#include <QDebug>

FaderFrame::FaderFrame(QWidget *parent) :
    QFrame(parent)
{

    timerAnimatorFadeOut = new QTimer(this);
    timerAnimatorFadeOut->setSingleShot(true);

    connect(timerAnimatorFadeOut, SIGNAL(timeout()), this, SLOT(fadeOutAfterTimer()) );


    effect = new QGraphicsOpacityEffect(this);
    this->setGraphicsEffect(effect);
    anim = new QPropertyAnimation(effect, "opacity", this);


    tmpDurationFadeOut = 0;
}




//---------------------------------------------------
void FaderFrame::fadeOutAfterTimer() {

    fadeOut(this->tmpDurationFadeOut);
}

//---------------------------------------------------------------------------------
void FaderFrame::fadeIn(int durationMs) {

//    qDebug() << "animateFadeIn FaderFrame" << durationMs;
    anim->stop();
    anim->setDuration(durationMs);
    anim->setStartValue(0);
    anim->setEndValue(1);
    anim->setEasingCurve(QEasingCurve::OutQuad);
    anim->start();
}

//---------------------------------------------------------------------------------
void FaderFrame::fadeOut(int durationMs) {

//    qDebug() << "animateFadeout FaderFrame" << durationMs;
    anim->stop();
    anim->setDuration(durationMs);
    anim->setStartValue(1);
    anim->setEndValue(0);
    anim->setEasingCurve(QEasingCurve::OutQuad);
    anim->start();
}


//--------------------------------------------------------------------------------
void FaderFrame::fadeOutAfterPause(int fadeDuration, int pause) {

    this->tmpDurationFadeOut = fadeDuration;
    timerAnimatorFadeOut->start(pause);

}

//-------------------------------------------------------------------------------------------------
void FaderFrame::fadeInAndFadeOutAfterPause(int fadeInDuration, int fadeOutDuration, int pause) {

    fadeIn(fadeInDuration);

    this->tmpDurationFadeOut = fadeOutDuration;
    timerAnimatorFadeOut->start(pause);

}
