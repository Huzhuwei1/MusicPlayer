//
// Created by Administrator on 2017/9/28.
//

#ifndef MUSICPLAYER_AUDIOQUEUE_H
#define MUSICPLAYER_AUDIOQUEUE_H

#include <pthread.h>
#include "queue"
#include "AndroidLogg.h"
#include "HPlayStatus.h"
extern "C" {

#include <libavcodec/avcodec.h>
};

class AudioQueue {

public:pthread_mutex_t pthread_mutex;
       pthread_cond_t pthread_cond;
       HPlayStatus *playStatus;
       std::queue<AVFrame *> avFrameQueue;


public:AudioQueue(HPlayStatus *_playStatus);
       ~AudioQueue();
       int getAVFrame(AVFrame **avFrame);
       int putAVFrame(AVFrame *avFrame);
       int getQueueSize();
       void clearQueue();
};


#endif //MUSICPLAYER_AUDIOQUEUE_H
