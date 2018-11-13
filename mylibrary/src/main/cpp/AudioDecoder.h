//
// Created by Administrator on 2017/9/17.
//

#ifndef MUSICPLAYER_AUDIODECODER_H
#define MUSICPLAYER_AUDIODECODER_H

#include "javaCallback.h"
#include "AndroidLogg.h"
#include <pthread.h>
#include "AudioQueue.h"
#include "HPlayStatus.h"
#include "SoundTouch.h"

using namespace soundtouch;
extern "C" {
#include "include/libavformat/avformat.h"
#include "include/libavcodec/avcodec.h"
#include "include/libavutil/avutil.h"
#include <libswresample/swresample.h>
#include <libavutil/time.h>
#include <SLES/OpenSLES.h>
#include <SLES/OpenSLES_Android.h>
};

class AudioDecoder {
public: const char *url = NULL;
        javaCallback *callback = NULL;
        HPlayStatus *playStatus = NULL;
        AVFormatContext *pFormatCtx = NULL;
        int audioStreamIndex = -1;
        AVCodecContext *pCodeCtx = NULL;
        AVCodecParameters *pCodecParam = NULL;
        AudioQueue *audioQueue = NULL;
        uint8_t *out_buffer = NULL;
        int total_time;
        double now_time;
        double actual_time;
        double last_time;
        AVRational time_base;
        pthread_mutex_t init_mutex;
        pthread_mutex_t seek_mutex;
        bool exit = false;
        int samples = 0;
        bool isRecordPcm = false;
        bool readFrameFinished = false;

        //OpenSLES
        SLObjectItf engineObject = NULL;
        SLEngineItf engineEngine = NULL;
        SLObjectItf outputMixObject = NULL;
        SLEnvironmentalReverbItf outputMixEnvironmentalReverb = NULL;
        SLObjectItf bqPlayerObject = NULL;
        SLPlayItf bqPlayerPlay = NULL;
        SLVolumeItf bqPlayerVolume = NULL;
        SLMuteSoloItf bqPlayerMuteSolo = NULL;
        SLAndroidSimpleBufferQueueItf bqPlayerBufferQueue;
        // aux effect on the output mix, used by the buffer queue player
        const SLEnvironmentalReverbSettings reverbSettings = SL_I3DL2_ENVIRONMENT_PRESET_STONECORRIDOR;


        //SoundTouch
        SoundTouch* soundTouch = NULL;
        SAMPLETYPE *soundtouch_buffer = NULL;
        bool finished = true;
        int numsample = 0;
        uint8_t *buffer = NULL;


public:AudioDecoder(javaCallback* _callback, HPlayStatus *_playStatus, const char* url);
       ~AudioDecoder();
       void prepare();
       void start();
       void pause();
       void resume();
       int resample(uint8_t **_buffer);
       void play();
       void stop();
       void initOpenSLES();
       int getCurSampleRateForOpenSLES(int samplerate);
       void unlockInitMutex();
       void release();
       void seek(int seconds);
       void setVolume(int percent);
       void setSoundChannel(int channel_id);
       int getSoundTouchData();
       void setTempo(float tempo);
       void setPitch(float pitch);
       int getVolumeLp(char *data, size_t data_size);
       void startStopRecord(bool isStart);
};


#endif //MUSICPLAYER_AUDIODECODER_H
