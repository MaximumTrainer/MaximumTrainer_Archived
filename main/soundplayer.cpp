#include "soundplayer.h"
#include <QDebug>


SoundPlayer::~SoundPlayer() {
    qDebug() << "Destructor SoundPlayer";
}


SoundPlayer::SoundPlayer(QObject *parent) : QObject(parent)  {


    QResource qrcAchievement(":/sound/fireTorch");
    QResource qrcLastBeepInterval(":/sound/secondBeep");
    QResource qrcFirstBeepInterval(":/sound/firstBeep");
    QResource qrcEndWorkout(":/sound/end_workout");
    QResource qrcStartWorkout(":/sound/resume");
    QResource qrcCadenceTooLow(":/sound/cadenceTooLow");
    QResource qrcCadenceTooHigh(":/sound/cadenceTooHigh");
    QResource qrcPowerTooLow(":/sound/powerTooLow");
    QResource qrcPowerTooHigh(":/sound/powerTooHigh");

    //    qrcAchievement = new QResource(":/sound/fireTorch");
    //    qrcLastBeepInterval = new QResource(":/sound/secondBeep");
    //    qrcFirstBeepInterval = new QResource(":/sound/firstBeep");
    //    qrcEndWorkout = new QResource(":/sound/end_workout");
    //    qrcStartWorkout = new QResource(":/sound/resume");
    //    qrcCadenceTooLow = new QResource(":/sound/cadenceTooLow");
    //    qrcCadenceTooHigh = new QResource(":/sound/cadenceTooHigh");
    //    qrcPowerTooLow = new QResource(":/sound/powerTooLow");
    //    qrcPowerTooHigh = new QResource(":/sound/powerTooHigh");


    bufferSoundAchievement.loadFromMemory(qrcAchievement.data(), qrcAchievement.size());
    bufferSoundLastBeepInterval.loadFromMemory(qrcLastBeepInterval.data(), qrcLastBeepInterval.size());
    bufferSoundFirstBeepInterval.loadFromMemory(qrcFirstBeepInterval.data(), qrcFirstBeepInterval.size());
    bufferSoundEndWorkout.loadFromMemory(qrcEndWorkout.data(), qrcEndWorkout.size());
    bufferSoundStartWorkout.loadFromMemory(qrcStartWorkout.data(), qrcStartWorkout.size());
    bufferSoundCadenceTooLow.loadFromMemory(qrcCadenceTooLow.data(), qrcCadenceTooLow.size());
    bufferSoundCadenceTooHigh.loadFromMemory(qrcCadenceTooHigh.data(), qrcCadenceTooHigh.size());
    bufferSoundPowerTooLow.loadFromMemory(qrcPowerTooLow.data(), qrcPowerTooLow.size());
    bufferSoundPowerTooHigh.loadFromMemory(qrcPowerTooHigh.data(), qrcPowerTooHigh.size());


    soundAchievement =   sf::Sound(bufferSoundAchievement);
    soundLastBeepInterval =  sf::Sound(bufferSoundLastBeepInterval);
    soundFirstBeepInterval =  sf::Sound(bufferSoundFirstBeepInterval);
    soundEndWorkout =  sf::Sound(bufferSoundEndWorkout);
    soundStartWorkout =  sf::Sound(bufferSoundStartWorkout);
    soundCadenceTooLow =  sf::Sound(bufferSoundCadenceTooLow);
    soundCadenceTooHigh =  sf::Sound(bufferSoundCadenceTooHigh);
    soundPowerTooLow =  sf::Sound(bufferSoundPowerTooLow);
    soundPowerTooHigh =  sf::Sound(bufferSoundPowerTooHigh);



    setVolume(100);
}


//------------------------------------------------------------------------------------------------------------------------
void SoundPlayer::setVolume(double volume) {

    qDebug() << "setVolume soundPlayer";

    soundAchievement.setVolume(volume);
    soundLastBeepInterval.setVolume(volume);
    soundFirstBeepInterval.setVolume(volume);
    soundEndWorkout.setVolume(volume);
    soundStartWorkout.setVolume(volume);
    soundCadenceTooLow.setVolume(volume);
    soundCadenceTooHigh.setVolume(volume);
    soundPowerTooLow.setVolume(volume);
    soundPowerTooHigh.setVolume(volume);
}


void SoundPlayer::playSoundEffectTest() {
        soundFirstBeepInterval.play();
}
void SoundPlayer::playSoundAchievement() {
        soundAchievement.play();
}


void SoundPlayer::playSoundLastBeepInterval() {
        soundLastBeepInterval.play();
}
void SoundPlayer::playSoundFirstBeepInterval() {
        soundFirstBeepInterval.play();
}
void SoundPlayer::playSoundEndWorkout() {
        soundEndWorkout.play();
}
void SoundPlayer::playSoundStartWorkout() {
        soundStartWorkout.play();
}
void SoundPlayer::playSoundPauseWorkout() {
        soundStartWorkout.play();
}
void SoundPlayer::playSoundCadenceTooLow() {
        soundCadenceTooLow.play();
}
void SoundPlayer::playSoundCadenceTooHigh() {
        soundCadenceTooHigh.play();
}
void SoundPlayer::playSoundPowerTooLow() {
        soundPowerTooLow.play();
}
void SoundPlayer::playSoundPowerTooHigh() {
        soundPowerTooHigh.play();
}
