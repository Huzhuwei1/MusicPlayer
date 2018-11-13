//
// Created by Administrator on 2017/9/28.
//

#include "AudioQueue.h"

AudioQueue::AudioQueue(HPlayStatus *_playStatus) {
    this->playStatus = _playStatus;
    pthread_mutex_init(&pthread_mutex,NULL);
    pthread_cond_init(&pthread_cond,NULL);
}

AudioQueue::~AudioQueue() {
    clearQueue();
    pthread_mutex_destroy(&pthread_mutex);
    pthread_cond_destroy(&pthread_cond);
}

int AudioQueue::getAVFrame(AVFrame **avFrame) {
    pthread_mutex_lock(&pthread_mutex);
    while (playStatus != NULL && !playStatus->exit) {
        if (avFrameQueue.size() > 0) {
//            AVFrame *frame =  avFrameQueue.front();
//            if(av_frame_ref(avFrame, frame) == 0) {
//                avFrameQueue.pop();
//            }
//            av_frame_free(&frame);
//            av_free(avFrame);
//            avFrame = NULL;
            *avFrame = avFrameQueue.front();
            avFrameQueue.pop();
            LOGD("从队列里面取出一个AVFrame，还剩下 %d 个", avFrameQueue.size());
            break;
        } else {
            pthread_cond_wait(&pthread_cond,&pthread_mutex);
        }
    }
    pthread_mutex_unlock(&pthread_mutex);
    return 0;
}

int AudioQueue::putAVFrame(AVFrame *avFrame) {
    pthread_mutex_lock(&pthread_mutex);
    avFrameQueue.push(avFrame);
    LOGD("放入一个AVFrame到队里里面， 个数为：%d", avFrameQueue.size());
    pthread_cond_signal(&pthread_cond);

    pthread_mutex_unlock(&pthread_mutex);
    return 0;
}

int AudioQueue::getQueueSize() {
    int size = 0;
    pthread_mutex_lock(&pthread_mutex);
    size = avFrameQueue.size();
    pthread_mutex_unlock(&pthread_mutex);

    return size;
}

void AudioQueue::clearQueue() {

    pthread_cond_signal(&pthread_cond);
    pthread_mutex_lock(&pthread_mutex);

    while (!avFrameQueue.empty()) {
        AVFrame *avFrame = avFrameQueue.front();
        avFrameQueue.pop();
        av_frame_free(&avFrame);
        av_free(avFrame);
        avFrame = NULL;
    }

    pthread_mutex_unlock(&pthread_mutex);
}

