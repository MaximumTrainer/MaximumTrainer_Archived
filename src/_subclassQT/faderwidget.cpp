#include "faderwidget.h"

#include <QDebug>



FaderWidget::FaderWidget(QWidget *parent) :
    QWidget(parent)
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
void FaderWidget::fadeOutAfterTimer() {

    fadeOut(this->tmpDurationFadeOut);
}

//---------------------------------------------------------------------------------
void FaderWidget::fadeIn(int durationMs) {

//    qDebug() << "animateFadeIn FaderWidget" << durationMs;
    anim->stop();
    anim->setDuration(durationMs);
    anim->setStartValue(0);
    anim->setEndValue(1);
    anim->setEasingCurve(QEasingCurve::OutQuad);
    anim->start();
}

//---------------------------------------------------------------------------------
void FaderWidget::fadeOut(int durationMs) {

//    qDebug() << "animateFadeout FaderWidget" << durationMs;
    anim->stop();
    anim->setDuration(durationMs);
    anim->setStartValue(1);
    anim->setEndValue(0);
    anim->setEasingCurve(QEasingCurve::OutQuad);
    anim->start();
}


//--------------------------------------------------------------------------------
void FaderWidget::fadeOutAfterPause(int fadeDuration, int pause) {

    this->tmpDurationFadeOut = fadeDuration;
    timerAnimatorFadeOut->start(pause);

}

//-------------------------------------------------------------------------------------------------
void FaderWidget::fadeInAndFadeOutAfterPause(int fadeInDuration, int fadeOutDuration, int pause) {

    fadeIn(fadeInDuration);

    this->tmpDurationFadeOut = fadeOutDuration;
    timerAnimatorFadeOut->start(pause);

}
