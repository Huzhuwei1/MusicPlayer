//
// Created by Administrator on 2017/9/17.
//

#include "AudioPlayerController.h"

void* threadMethod_prepare(void *data) {
    AudioPlayerController* controller = (AudioPlayerController*)data;
    controller->audioDecoder->prepare();
    pthread_exit(&controller->thread_prepare);
}

void* threadMethod_start(void* data) {
    AudioPlayerController* controller = (AudioPlayerController*)data;
    controller->audioDecoder->start();
    pthread_exit(&controller->thread_decoder);
}

void* threadMethod_play(void* data) {
    AudioPlayerController* controller = (AudioPlayerController*)data;
    controller->audioDecoder->play();
    pthread_exit(&controller->thread_play);
}

AudioPlayerController::AudioPlayerController(javaCallback *_callback, HPlayStatus *_playStatus) {
    this->callback = _callback;
    this->playStatus = _playStatus;
}

AudioPlayerController::~AudioPlayerController() {

}

void AudioPlayerController::prepare(const char *url) {
    if(audioDecoder == NULL) {
        audioDecoder = new AudioDecoder(callback,playStatus,url);
    }
    pthread_create(&thread_prepare,NULL,threadMethod_prepare,this);
}


void AudioPlayerController::start() {

    if(audioDecoder == NULL) {
        LOGE("AudioDecoder is empty");
        return;
    }
    pthread_create(&thread_decoder,NULL,threadMethod_start,this);
    pthread_create(&thread_play,NULL,threadMethod_play,this);
}

void AudioPlayerController::pause() {

    if(audioDecoder == NULL) {
        LOGE("AudioDecoder is empty");
        return;
    }
    audioDecoder->pause();
}

void AudioPlayerController::resume() {

    if(audioDecoder == NULL) {
        LOGE("AudioDecoder is empty");
        return;
    }
    audioDecoder->resume();
}

void AudioPlayerController::stop() {
   if (audioDecoder != NULL) {
       audioDecoder->release();
       delete(audioDecoder);
       audioDecoder = NULL;
   }
    if (callback != NULL) {
        delete(callback);
        callback = NULL;
    }
    if (playStatus != NULL) {
        delete(playStatus);
        playStatus = NULL;
    }
}

void AudioPlayerController::seek(int seconds) {

    if (audioDecoder != NULL) {
        audioDecoder->seek(seconds);
    }
}

void AudioPlayerController::setVolume(int percent) {

    if (audioDecoder != NULL) {
        audioDecoder->setVolume(percent);
    }
}

void AudioPlayerController::setSoundChannel(int channel_id) {

    if (audioDecoder != NULL) {
        audioDecoder->setSoundChannel(channel_id);
    }
}

void AudioPlayerController::setTempo(float tempo) {

    if (audioDecoder != NULL) {
        audioDecoder->setTempo(tempo);
    }
}

void AudioPlayerController::setPitch(float pitch) {

    if (audioDecoder != NULL) {
        audioDecoder->setPitch(pitch);
    }
}

int AudioPlayerController::getSampleRate() {

    if (audioDecoder != NULL && audioDecoder->pCodecParam != NULL) {
        return audioDecoder->pCodecParam->sample_rate;
    }
    return 0;
}

void AudioPlayerController::startStopRecord(bool isStart) {

    if (audioDecoder != NULL) {
        audioDecoder->startStopRecord(isStart);
    }
}

