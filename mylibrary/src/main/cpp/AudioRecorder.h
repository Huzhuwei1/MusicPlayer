//
// Created by Administrator on 2017/11/13.
//

#ifndef MUSICPLAYER_AUDIORECORDER_H
#define MUSICPLAYER_AUDIORECORDER_H
#include <stdio.h>
#include <assert.h>
#include "AndroidLogg.h"

#include <SLES/OpenSLES.h>
#include <SLES/OpenSLES_Android.h>


#define NB_BUFFERS_IN_QUEUE 1

/* Explicitly requesting SL_IID_ANDROIDSIMPLEBUFFERQUEUE and SL_IID_ANDROIDCONFIGURATION
 * on the AudioRecorder object */
#define NUM_EXPLICIT_INTERFACES_FOR_RECORDER 2

/* Size of the recording buffer queue */
#define NB_BUFFERS_IN_QUEUE 1
/* Size of each buffer in the queue */
#define BUFFER_SIZE_IN_SAMPLES 8192
#define BUFFER_SIZE_IN_BYTES   (2 * BUFFER_SIZE_IN_SAMPLES)

class AudioRecorder {

public:AudioRecorder();
       ~AudioRecorder();
       void createEngine();
       void createAudioRecord(const char *url);
       void stop();
       void shutdown();

};


#endif //MUSICPLAYER_AUDIORECORDER_H
