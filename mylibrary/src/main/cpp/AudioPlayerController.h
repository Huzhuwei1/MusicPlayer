//
// Created by Administrator on 2017/9/17.
//

#ifndef MUSICPLAYER_AUDIOPLAYERCONTROLLER_H
#define MUSICPLAYER_AUDIOPLAYERCONTROLLER_H


#include <cwchar>
#include "javaCallback.h"
#include "AudioDecoder.h"
#include "HPlayStatus.h"

class AudioPlayerController {

public:javaCallback* callback = NULL;
       AudioDecoder* audioDecoder = NULL;
       HPlayStatus *playStatus = NULL;
       pthread_t thread_prepare;
       pthread_t thread_decoder;
       pthread_t thread_play;


public:AudioPlayerController(javaCallback* _callback, HPlayStatus *_playStatus);
       ~AudioPlayerController();
       void prepare(const char* url);
       void start();
       void pause();
       void resume();
       void stop();
       void seek(int seconds);
       void setVolume(int percent);
       void setSoundChannel(int channel_id);
       void setTempo(float tempo);
       void setPitch(float pitch);
};


#endif //MUSICPLAYER_AUDIOPLAYERCONTROLLER_H
