#ifndef MYVLCPLAYER_H
#define MYVLCPLAYER_H

#include <QGridLayout>
#include <QHBoxLayout>
#include <QWidget>
#include <QSlider>
#include <QMenu>
#include <QEvent>
#include <QHostInfo>



#include <VLCQtCore/Common.h>
#include <VLCQtCore/Instance.h>
#include <VLCQtCore/Media.h>
#include <VLCQtCore/MediaPlayer.h>
#include <VLCQtCore/MediaListPlayer.h>
#include <VLCQtCore/Video.h>
#include <VLCQtCore/Audio.h>
#include <VLCQtWidgets/ControlVideo.h>
#include <VLCQtWidgets/ControlAudio.h>
#include <VLCQtWidgets/WidgetSeek.h>
#include <VLCQtWidgets/WidgetVideo.h>






#include "clickablelabel.h"



class VlcInstance;
class VlcMedia;
class VlcMediaPlayer;
class VlcVideo;
class VlcAudio;
class VlcMediaListPlayer;
class VlcControlVideo;
class VlcControlAudio;




class MyVlcPlayer : public QWidget
{
    Q_OBJECT
public:
    explicit MyVlcPlayer(QWidget *parent = 0);
    ~MyVlcPlayer();
    bool eventFilter(QObject *watched, QEvent *event);

    void setMovieTime(int msec);

    void setRadio(bool isRadio) {
        this->isRadio = isRadio;
    }



signals:
    void playing();
    void stopped();
    void paused();




public slots:
    void openUrlRadio(QString url);
    void openUrlRadioFromIp(QHostInfo hostInfo);
    void stop();
    void videoRightClick();

    void openLocal();
    void openUrl();
    void playOrPause();
    void pause();
    void resume();

    void resetActionMenu();

    void updateSubtitle();
    void subtitleChanged();


    void changeVolume(int);
    void muteVolume(bool);

    void hideWidgets();
    void pushTimerHideWidgets();

    void audioTracksChanged(QList<QAction*>);
//    void subtitlesTrackChanged(QList<QAction*>);

    void checkForSubtitle(QList<QAction*> lstAction);





private :
    void initUI();
    void constructSubtitleMenu();
    void setPlayInterface();
    void setPauseInterface();
    void interfaceVideoLoaded(bool loaded);
    QString loadPath();
    void savePath(QString path);

    void saveSoundVolume(int vol);
    int loadSoundVolume();





private :
    QMenu *menu;
    QMenu *subMenuOpen;
    bool isRadio;

    //Subtitles
    QMenu *subMenuSubtitleTrack;
    QActionGroup *subtitleTrackActionGroup;
    QList<QAction*> lstSubAction;

    QAction *action_playOrPause;
    QAction *action_stop;
    QAction *action_openLocalMedia;
    QAction *action_openURLMedia;

    QTimer *timer_hideWidgets;

    VlcInstance *_instance;
    VlcMedia *_media;
    VlcMediaPlayer *_player;
    VlcVideo *_video;
    VlcAudio *_audio;
    VlcControlVideo *_controlVideo;
    VlcControlAudio *_controlAudio;

    bool widgetControlhidden;
    bool isStarted;
    bool videoLoaded;
    bool isMuted;

    int audioVol;

    bool videoAlreadySet;
    bool audioAlreadySet;
    bool subtitlesAlreadySet;


    // UI
    QGridLayout *gridLayout;
    QWidget *widget_volume;
    QHBoxLayout *horizontalLayout;
    QSpacerItem *horizontalSpacer;
    ClickableLabel *label_volume;
    QSlider *horizontalSlider_volume;
    QLabel *label_currentVolume;
    QLabel *label;
    VlcWidgetVideo *video;
    VlcWidgetSeek *seek;
};

#endif // MYVLCPLAYER_H
