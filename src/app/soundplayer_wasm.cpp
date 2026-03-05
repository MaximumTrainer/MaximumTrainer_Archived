// No-op SoundPlayer implementation for WebAssembly builds.
// Audio playback via SFML is not available in the Wasm sandbox.
// All methods are intentional no-ops; the interface is preserved so the
// rest of the application compiles and runs without modification.
#include "soundplayer.h"

SoundPlayer::~SoundPlayer() {}

SoundPlayer::SoundPlayer(QObject *parent) : QObject(parent) {}

void SoundPlayer::setVolume(double) {}
void SoundPlayer::playSoundEffectTest() {}
void SoundPlayer::playSoundAchievement() {}
void SoundPlayer::playSoundLastBeepInterval() {}
void SoundPlayer::playSoundFirstBeepInterval() {}
void SoundPlayer::playSoundEndWorkout() {}
void SoundPlayer::playSoundStartWorkout() {}
void SoundPlayer::playSoundPauseWorkout() {}
void SoundPlayer::playSoundCadenceTooLow() {}
void SoundPlayer::playSoundCadenceTooHigh() {}
void SoundPlayer::playSoundPowerTooLow() {}
void SoundPlayer::playSoundPowerTooHigh() {}
