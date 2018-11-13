//
// Created by Administrator on 2017/11/13.
//

#include "AudioRecorder.h"

/* Local storage for Audio data */
int8_t pcmData[NB_BUFFERS_IN_QUEUE * BUFFER_SIZE_IN_BYTES];
// engine interfaces
static SLObjectItf engineObject = NULL;
static SLEngineItf engineEngine = NULL;

//audio record interfaces
static SLObjectItf recorderObject = NULL;
static SLRecordItf recordItf = NULL;
static SLAndroidSimpleBufferQueueItf recBuffQueueItf = NULL;
static SLAndroidConfigurationItf configItf = NULL;

static FILE * gFile = NULL;

//-----------------------------------------------------------------
/* Structure for passing information to callback function */
typedef struct CallbackCntxt_ {
    SLPlayItf  playItf;
    SLuint32   size;
    SLint8*   pDataBase;    // Base address of local audio data storage
    SLint8*   pData;        // Current address of local audio data storage
} CallbackCntxt;

static CallbackCntxt cntxt;

/* Callback for recording buffer queue events */
void recCallback(SLRecordItf caller, void *pContext, SLuint32 event) {
    if (SL_RECORDEVENT_HEADATNEWPOS & event) {
        SLmillisecond pMsec = 0;
        (*caller)->GetPosition(caller, &pMsec);
    }

    if (SL_RECORDEVENT_HEADATMARKER & event) {
        SLmillisecond pMsec = 0;
        (*caller)->GetPosition(caller, &pMsec);
    }
}

/* Callback for recording buffer queue events */
void recBufferQueueCallback(SLAndroidSimpleBufferQueueItf queueItf, void *pContext) {

    CallbackCntxt *pCntxt = (CallbackCntxt*) pContext;

    /* Save the recorded data  */
    fwrite(pCntxt->pDataBase, BUFFER_SIZE_IN_BYTES, 1, gFile);

    /* Increase data pointer by buffer size */
    pCntxt->pData += BUFFER_SIZE_IN_BYTES;

    if (pCntxt->pData >= pCntxt->pDataBase + (NB_BUFFERS_IN_QUEUE * BUFFER_SIZE_IN_BYTES)) {
        pCntxt->pData = pCntxt->pDataBase;
    }

    (*queueItf)->Enqueue(queueItf, pCntxt->pDataBase, BUFFER_SIZE_IN_BYTES);

    SLAndroidSimpleBufferQueueState recQueueState;
    (*queueItf)->GetState(queueItf, &recQueueState);

}

AudioRecorder::AudioRecorder() {

}

AudioRecorder::~AudioRecorder() {

}

void AudioRecorder::createEngine() {

    SLEngineOption EngineOption[] = {
            {(SLuint32) SL_ENGINEOPTION_THREADSAFE, (SLuint32) SL_BOOLEAN_TRUE}
    };
    SLresult result;
    result = slCreateEngine(&engineObject, 1, EngineOption, 0, NULL, NULL);
    assert(SL_RESULT_SUCCESS == result);

    /* Realizing the SL Engine in synchronous mode. */
    result = (*engineObject)->Realize(engineObject, SL_BOOLEAN_FALSE);
    assert(SL_RESULT_SUCCESS == result);

    // get the engine interface, which is needed in order to create other objects
    result = (*engineObject)->GetInterface(engineObject, SL_IID_ENGINE, &engineEngine);
    assert(SL_RESULT_SUCCESS == result);
}

void AudioRecorder::createAudioRecord(const char *url) {

    if (recorderObject != NULL) {

        return ;
    }

    gFile = fopen(url, "w");

    if (gFile == NULL) {
    LOGE(" open file fail ");
        return ;
    }

    SLresult result;

    /* setup the data source*/
    SLDataLocator_IODevice ioDevice = {
            SL_DATALOCATOR_IODEVICE,
            SL_IODEVICE_AUDIOINPUT,
            SL_DEFAULTDEVICEID_AUDIOINPUT,
            NULL
    };

    SLDataSource recSource = {&ioDevice, NULL};

    SLDataLocator_AndroidSimpleBufferQueue recBufferQueue = {
            SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE,
            NB_BUFFERS_IN_QUEUE
    };

    SLDataFormat_PCM pcm = {
            SL_DATAFORMAT_PCM,
            2,
            SL_SAMPLINGRATE_44_1,
            SL_PCMSAMPLEFORMAT_FIXED_16,
            16,
            SL_SPEAKER_FRONT_LEFT| SL_SPEAKER_FRONT_RIGHT,
            SL_BYTEORDER_LITTLEENDIAN
    };

    SLDataSink dataSink = { &recBufferQueue, &pcm };
    SLInterfaceID iids[NUM_EXPLICIT_INTERFACES_FOR_RECORDER] = {SL_IID_ANDROIDSIMPLEBUFFERQUEUE, SL_IID_ANDROIDCONFIGURATION};
    SLboolean required[NUM_EXPLICIT_INTERFACES_FOR_RECORDER] = {SL_BOOLEAN_TRUE, SL_BOOLEAN_TRUE};

    /* Create the audio recorder */
    result = (*engineEngine)->CreateAudioRecorder(engineEngine, &recorderObject , &recSource, &dataSink,
                                                  NUM_EXPLICIT_INTERFACES_FOR_RECORDER, iids, required);
    assert(SL_RESULT_SUCCESS == result);


    /* get the android configuration interface*/
    result = (*recorderObject)->GetInterface(recorderObject, SL_IID_ANDROIDCONFIGURATION, &configItf);
    assert(SL_RESULT_SUCCESS == result);

    /* Realize the recorder in synchronous mode. */
    result = (*recorderObject)->Realize(recorderObject, SL_BOOLEAN_FALSE);
    assert(SL_RESULT_SUCCESS == result);

    /* Get the buffer queue interface which was explicitly requested */
    result = (*recorderObject)->GetInterface(recorderObject, SL_IID_ANDROIDSIMPLEBUFFERQUEUE, (void*) &recBuffQueueItf);
    assert(SL_RESULT_SUCCESS == result);


    /* get the record interface */
    result = (*recorderObject)->GetInterface(recorderObject, SL_IID_RECORD, &recordItf);
    assert(SL_RESULT_SUCCESS == result);


    /* Set up the recorder callback to get events during the recording */
    result = (*recordItf)->SetMarkerPosition(recordItf, 2000);
    assert(SL_RESULT_SUCCESS == result);

    result = (*recordItf)->SetPositionUpdatePeriod(recordItf, 500);
    assert(SL_RESULT_SUCCESS == result);

    result = (*recordItf)->SetCallbackEventsMask(recordItf, SL_RECORDEVENT_HEADATMARKER | SL_RECORDEVENT_HEADATNEWPOS);
    assert(SL_RESULT_SUCCESS == result);

    result = (*recordItf)->RegisterCallback(recordItf, recCallback, NULL);
    assert(SL_RESULT_SUCCESS == result);

    /* Initialize the callback and its context for the recording buffer queue */

    cntxt.pDataBase = (int8_t*) &pcmData;
    cntxt.pData = cntxt.pDataBase;
    cntxt.size = sizeof(pcmData);
    result = (*recBuffQueueItf)->RegisterCallback(recBuffQueueItf, recBufferQueueCallback, &cntxt);
    assert(SL_RESULT_SUCCESS == result);

    /* Enqueue buffers to map the region of memory allocated to store the recorded data */
    LOGD("Enqueueing buffer ");
    for (int i = 0; i < NB_BUFFERS_IN_QUEUE; i++) {
        LOGD("%d ", i);
        result = (*recBuffQueueItf)->Enqueue(recBuffQueueItf, cntxt.pData, BUFFER_SIZE_IN_BYTES);
        assert(SL_RESULT_SUCCESS == result);
        cntxt.pData += BUFFER_SIZE_IN_BYTES;
    }
    cntxt.pData = cntxt.pDataBase;

    /* Start recording */
    result = (*recordItf)->SetRecordState(recordItf, SL_RECORDSTATE_RECORDING);
    assert(SL_RESULT_SUCCESS == result);
    LOGD("Starting to record");
}

void AudioRecorder::stop() {

    if (recordItf != NULL) {
        SLresult result = (*recordItf)->SetRecordState(recordItf, SL_RECORDSTATE_STOPPED);
        assert(SL_RESULT_SUCCESS == result);
    }
}

void AudioRecorder::shutdown() {

    //destroy recorder object , and invlidate all associated interfaces
    if (recorderObject != NULL) {
        (*recorderObject)->Destroy(recorderObject);
        recorderObject = NULL;
        recordItf = NULL;
        recBuffQueueItf = NULL;
        configItf = NULL;
    }

    // destroy engine object, and invalidate all associated interfaces
    if (engineObject != NULL) {
        (*engineObject)->Destroy(engineObject);
        engineObject = NULL;
        engineEngine = NULL;
    }

    //colse the file
    if (gFile != NULL) {
        fclose(gFile);
        gFile = NULL;
    }
}
