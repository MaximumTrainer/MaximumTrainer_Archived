#ifndef SOUNDPLAYER_H
#define SOUNDPLAYER_H

#include <QResource>
#include <QtCore>

#include "SFML/Audio.hpp"
//#include <iomanip>
//#include <iostream>




class SoundPlayer : public QObject
{
    Q_OBJECT

public:
    ~SoundPlayer();
    SoundPlayer(QObject *parent = 0);



    /// 0 to 100
    void setVolume(double volume);

    void playSoundEffectTest();

    void playSoundAchievement();
    void playSoundLastBeepInterval();
    void playSoundFirstBeepInterval();
    void playSoundEndWorkout();
    void playSoundStartWorkout();
    void playSoundPauseWorkout();
    void playSoundCadenceTooLow();
    void playSoundCadenceTooHigh();
    void playSoundPowerTooLow();
    void playSoundPowerTooHigh();




private :
    //    QResource *qrcAchievement;
    //    QResource *qrcLastBeepInterval;
    //    QResource *qrcFirstBeepInterval;
    //    QResource *qrcEndWorkout;
    //    QResource *qrcStartWorkout;
    //    QResource *qrcCadenceTooLow;
    //    QResource *qrcCadenceTooHigh;
    //    QResource *qrcPowerTooLow;
    //    QResource *qrcPowerTooHigh;


    /// Achievement
    sf::SoundBuffer bufferSoundAchievement;
    sf::Sound soundAchievement;

    /// LastBeepInterval
    sf::SoundBuffer bufferSoundLastBeepInterval;
    sf::Sound soundLastBeepInterval;

    //    /// FirstBeepInterval
    sf::SoundBuffer bufferSoundFirstBeepInterval;
    sf::Sound soundFirstBeepInterval;

    //    /// EndWorkout
    sf::SoundBuffer bufferSoundEndWorkout;
    sf::Sound soundEndWorkout;

    //    /// StartWorkout = PauseWorkout
    sf::SoundBuffer bufferSoundStartWorkout;
    sf::Sound soundStartWorkout;

    //    /// CadenceTooLow
    sf::SoundBuffer bufferSoundCadenceTooLow;
    sf::Sound soundCadenceTooLow;

    //    /// CadenceTooHigh
    sf::SoundBuffer bufferSoundCadenceTooHigh;
    sf::Sound soundCadenceTooHigh;

    //    /// PowerTooLow
    sf::SoundBuffer bufferSoundPowerTooLow;
    sf::Sound soundPowerTooLow;

    //    /// PowerTooHigh
    sf::SoundBuffer bufferSoundPowerTooHigh;
    sf::Sound soundPowerTooHigh;

};
Q_DECLARE_METATYPE(SoundPlayer*)

#endif // SOUNDPLAYER_H
