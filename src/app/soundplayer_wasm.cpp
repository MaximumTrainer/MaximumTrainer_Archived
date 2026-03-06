// WebAssembly SoundPlayer using the Web Audio API via Emscripten EM_JS.
// Generates synthesised beep tones – each workout event uses a distinct
// pitch and duration so they are audibly distinguishable.
// AudioContext creation is deferred to the first call to avoid browsers
// blocking audio contexts created before a user gesture.
#include "soundplayer.h"
#include <emscripten.h>

EM_JS(void, js_play_beep, (double freq, double durationMs, double volume), {
    try {
        if (!window._wasmAudioCtx) {
            window._wasmAudioCtx =
                new (window.AudioContext || window.webkitAudioContext)();
        }
        const ctx  = window._wasmAudioCtx;
        const osc  = ctx.createOscillator();
        const gain = ctx.createGain();
        osc.connect(gain);
        gain.connect(ctx.destination);
        osc.type = 'sine';
        osc.frequency.value = freq;
        gain.gain.setValueAtTime(volume, ctx.currentTime);
        gain.gain.exponentialRampToValueAtTime(
            0.001, ctx.currentTime + durationMs / 1000.0);
        osc.start(ctx.currentTime);
        osc.stop(ctx.currentTime + durationMs / 1000.0);
    } catch (e) { /* audio not available */ }
});

SoundPlayer::SoundPlayer(QObject *parent) : QObject(parent) {}
SoundPlayer::~SoundPlayer() {}

// volume is 0–100; scale to 0–1 for Web Audio
void SoundPlayer::setVolume(double volume) { m_volume = volume / 100.0; }

// Each event uses a distinct frequency (Hz) and duration (ms).
//   High pitch  = high/fast feedback  (cadence/power too high)
//   Low pitch   = low/slow feedback   (cadence/power too low)
//   Mid-rising  = start / achievement
//   Mid-falling = end / pause
void SoundPlayer::playSoundEffectTest()        { js_play_beep( 880, 200, m_volume); }
void SoundPlayer::playSoundAchievement()       { js_play_beep(1047, 500, m_volume); }
void SoundPlayer::playSoundLastBeepInterval()  { js_play_beep( 880, 300, m_volume); }
void SoundPlayer::playSoundFirstBeepInterval() { js_play_beep( 660, 150, m_volume); }
void SoundPlayer::playSoundEndWorkout()        { js_play_beep( 523, 600, m_volume); }
void SoundPlayer::playSoundStartWorkout()      { js_play_beep( 660, 400, m_volume); }
void SoundPlayer::playSoundPauseWorkout()      { js_play_beep( 440, 300, m_volume); }
void SoundPlayer::playSoundCadenceTooLow()     { js_play_beep( 330, 200, m_volume); }
void SoundPlayer::playSoundCadenceTooHigh()    { js_play_beep( 990, 200, m_volume); }
void SoundPlayer::playSoundPowerTooLow()       { js_play_beep( 330, 200, m_volume); }
void SoundPlayer::playSoundPowerTooHigh()      { js_play_beep( 990, 200, m_volume); }

