//
// Created by Administrator on 2017/9/17.
//

#include "AudioDecoder.h"
static pthread_mutex_t  audioEngineLock = PTHREAD_MUTEX_INITIALIZER;

AudioDecoder::AudioDecoder(javaCallback *_callback, HPlayStatus *_playStatus, const char *url) {
    this->callback = _callback;
    this->playStatus = _playStatus;
    this->url = url;
    audioQueue = new AudioQueue(playStatus);
    pthread_mutex_init(&init_mutex,NULL);
    pthread_mutex_init(&seek_mutex,NULL);
}

AudioDecoder::~AudioDecoder() {
   pthread_mutex_destroy(&init_mutex);
   pthread_mutex_destroy(&seek_mutex);
}


//this callback handler is called every time a buffer finishes playing
void bqPlayerCallback(SLAndroidSimpleBufferQueueItf bq, void* context) {
    AudioDecoder *decoder = (AudioDecoder *)context;
    if(decoder != NULL) {
//        int data_size = decoder->resample();
        int data_size = decoder->getSoundTouchData();
        if (data_size <= 0) {
            if (decoder->total_time - decoder->last_time < 1) {
                decoder->actual_time = decoder->last_time = decoder->total_time;
                decoder->callback->onPlaying(THREAD_TYPE_CHILD, (int) decoder->actual_time, decoder->total_time);
            }
            pthread_mutex_unlock(&audioEngineLock);
            return;
        }
        int sample_rate = 0;
        if (decoder->pCodecParam != NULL){
            sample_rate = decoder->pCodecParam->sample_rate;
        }
        decoder->actual_time = decoder->actual_time + data_size/(sample_rate * 2 * 2);
        if(decoder->actual_time - decoder->last_time >= 1){
            decoder->last_time = decoder->actual_time;
            decoder->callback->onPlaying(THREAD_TYPE_CHILD, (int) decoder->actual_time, decoder->total_time);
        }
        if(decoder->isRecordPcm) {
            decoder->callback->onCallPcmToAAC(THREAD_TYPE_CHILD, data_size * 2 * 2, decoder->soundtouch_buffer);
        }

        LOGD("Current Volume Lp: %d",decoder->getVolumeLp((char *)decoder->soundtouch_buffer,data_size * 4));
//       enqueue another buffer
        SLresult result = (*bq)->Enqueue(bq, (char *)decoder->soundtouch_buffer, (SLuint32) data_size *2 *2);
        if (SL_RESULT_SUCCESS != result) {
            pthread_mutex_unlock(&audioEngineLock);
        }
    }
}

int interrupt_callback(void *data) {
    AudioDecoder *decoder = (AudioDecoder *)data;
    if (decoder->playStatus->exit) {
        return AVERROR_EOF;
    }
    return 0;
}

void AudioDecoder::prepare() {
    pthread_mutex_lock(&init_mutex);
    //注册解码器
    av_register_all();
    avformat_network_init();
    

    //打开文件
    pFormatCtx = avformat_alloc_context();
    pFormatCtx->interrupt_callback.callback = interrupt_callback;
    pFormatCtx->interrupt_callback.opaque = this;
    if(avformat_open_input(&pFormatCtx,url,NULL,NULL) != 0){
        LOGE("cannot open url:%s",url);
        unlockInitMutex();
        callback->onError(THREAD_TYPE_CHILD);
        return;
    }
    //获取流信息
    if (avformat_find_stream_info(pFormatCtx,NULL) < 0){
        LOGE("find stream info fail");
        unlockInitMutex();
        callback->onError(THREAD_TYPE_CHILD);
        return;
    }
    //获取音频流
    for (int i = 0; i < pFormatCtx->nb_streams ; i++) {
        if (pFormatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO){
            audioStreamIndex = i;
            pCodecParam = pFormatCtx->streams[i]->codecpar;
            out_buffer = (uint8_t *) av_malloc((size_t) (pCodecParam->sample_rate * 2 * 2));
            soundtouch_buffer = (SAMPLETYPE *) av_malloc((size_t) (pCodecParam->sample_rate * 2 * 2));
            soundTouch = new SoundTouch();
            soundTouch->setSampleRate((uint) pCodecParam->sample_rate);
            soundTouch->setChannels(2);
            total_time = (int) pFormatCtx->duration/AV_TIME_BASE;
            time_base = pFormatCtx->streams[i]->time_base;
        }
    }
    //获取解码器
    AVCodec* avCodec =avcodec_find_decoder(pCodecParam->codec_id);
    if (!avCodec) {
        LOGE("find AvCodec fail");
        unlockInitMutex();
        callback->onError(THREAD_TYPE_CHILD);
        return;
    }
    pCodeCtx = avcodec_alloc_context3(avCodec);
    if (avcodec_parameters_to_context(pCodeCtx,pCodecParam) < 0){//新API
        LOGE("get codeCtx fail");
        unlockInitMutex();
        callback->onError(THREAD_TYPE_CHILD);
        return;
    };
    //打卡解码器
    if (avcodec_open2(pCodeCtx,avCodec,0)!=0){
        LOGE("open avcodec fail");
        unlockInitMutex();
        callback->onError(THREAD_TYPE_CHILD);
        return;
    }

    callback->onPrepared(THREAD_TYPE_CHILD);
    pthread_mutex_unlock(&init_mutex);
}


void AudioDecoder::start() {

    AVPacket* avPacket = av_packet_alloc();
    //读取音频帧
    int count = 0;
    while (playStatus != NULL && !playStatus->exit) {

        if (audioQueue != NULL && audioQueue->getQueueSize() > 50) {
            continue;
        }

        pthread_mutex_lock(&seek_mutex);
        int ret = av_read_frame(pFormatCtx,avPacket);
        pthread_mutex_unlock(&seek_mutex);

        if (ret == 0){
            if(avPacket->stream_index == audioStreamIndex) {
                count++;
                //解码压缩数据
                int ret = avcodec_send_packet(pCodeCtx, avPacket);
                if(ret != 0) {
                    LOGE("Error avcodec_send_packet fail");
                    continue;
                }

                while (1) {
                    AVFrame *avFrame = av_frame_alloc();
                    ret = avcodec_receive_frame(pCodeCtx, avFrame);
                    if(ret != 0) {
                        LOGE("Error avcodec_receive_frame fail");
                        av_frame_free(&avFrame);
                        av_free(avFrame);
                        avFrame = NULL;
                        break;
                    }
                    audioQueue->putAVFrame(avFrame);
                    LOGD("解码到第%d帧",count);
                    continue;
                }

            }
        } else {
            //flush decoder
            while (1) {
                avPacket->data = NULL;
                avPacket->size = 0;
                int ret = avcodec_send_packet(pCodeCtx, avPacket);
                if(ret != 0) {
                    LOGE("Error avcodec_send_packet fail");
                    break;
                }

                AVFrame *avFrame = av_frame_alloc();
                ret = avcodec_receive_frame(pCodeCtx, avFrame);
                if (ret == AVERROR_EOF){
                    LOGE("the decoder has been fully flushed, and there will be no more output frames");
                }
                if(ret != 0) {
                    LOGE("Error avcodec_receive_frame fail");
                    av_frame_free(&avFrame);
                    av_free(avFrame);
                    avFrame = NULL;
                    break;
                }
                audioQueue->putAVFrame(avFrame);
                break;
            }
            LOGD("decode finished");
            av_packet_free(&avPacket);
            av_free(avPacket);
            avPacket = NULL;
            break;
        }
    }

    while (playStatus != NULL && !playStatus->exit) {
        if(audioQueue->getQueueSize() > 0) {
            //等待播放完成
            continue;
        } else{
            playStatus->exit = true;
            break;
        }
    }

    callback->onComplete(THREAD_TYPE_CHILD);
    exit = true;
}


void AudioDecoder::initOpenSLES() {
    //创建接口对象，即引擎对象
    //Create engine
    slCreateEngine(&engineObject,0,NULL,0,NULL,NULL);

    //realize the engine
    (*engineObject)->Realize(engineObject,SL_BOOLEAN_FALSE);

    //get the engine interface ,which is needed in order to create other objects
    (*engineObject)->GetInterface(engineObject,SL_IID_ENGINE,&engineEngine);

    //c创建混音器
    //create output mix, with environmental reverb specified as a non-required interface
    const SLInterfaceID  ids[1] = {SL_IID_ENVIRONMENTALREVERB};
    const SLboolean req[1] = {SL_BOOLEAN_FALSE};
    (*engineEngine)->CreateOutputMix(engineEngine,&outputMixObject,1,ids,req);

    //realize the output mix
    (*outputMixObject)->Realize(outputMixObject,SL_BOOLEAN_FALSE);
    //get the environmental reverb interface
    // this could fail if the environmental reverb effect is not available,
    // either because the feature is not present, excessive CPU load, or
    // the required MODIFY_AUDIO_SETTINGS permission was not requested and granted
    SLresult lresult =(*outputMixObject)->GetInterface(outputMixObject,SL_IID_ENVIRONMENTALREVERB,&outputMixEnvironmentalReverb);
    if(SL_RESULT_SUCCESS == lresult) {
        (*outputMixEnvironmentalReverb)->SetEnvironmentalReverbProperties(
                outputMixEnvironmentalReverb,&reverbSettings);
    }


    //创建播放器(录音器)
    //create buffer queue audio player

    //configure audio source
    SLDataLocator_AndroidSimpleBufferQueue loc_bufq = {SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE,2};
    SLDataFormat_PCM format_pcm = {SL_DATAFORMAT_PCM, 2,
                                   (SLuint32) getCurSampleRateForOpenSLES(pCodecParam != NULL ? pCodecParam->sample_rate : 0),
                                   SL_PCMSAMPLEFORMAT_FIXED_16, SL_PCMSAMPLEFORMAT_FIXED_16,
                                   SL_SPEAKER_FRONT_LEFT | SL_SPEAKER_FRONT_RIGHT, SL_BYTEORDER_LITTLEENDIAN};

    SLDataSource audioSrc = {&loc_bufq,&format_pcm};
    //configure audio sink
    SLDataLocator_OutputMix loc_outmix = {SL_DATALOCATOR_OUTPUTMIX,outputMixObject};
    SLDataSink audioSnk = {&loc_outmix,NULL};

    const SLInterfaceID ids1[4] = {SL_IID_BUFFERQUEUE,SL_IID_VOLUME,SL_IID_PLAYBACKRATE,SL_IID_MUTESOLO};
    const SLboolean  req1[4] = {SL_BOOLEAN_TRUE,SL_BOOLEAN_TRUE,SL_BOOLEAN_TRUE,SL_BOOLEAN_TRUE};
    (*engineEngine)->CreateAudioPlayer(engineEngine,&bqPlayerObject,&audioSrc,&audioSnk,
                                       4,ids1,req1);

    //realize the player
    (*bqPlayerObject)->Realize(bqPlayerObject,SL_BOOLEAN_FALSE);

    //get the play interface
    (*bqPlayerObject)->GetInterface(bqPlayerObject,SL_IID_PLAY,&bqPlayerPlay);

    //创建缓冲队列和回调函数
    //get the buffer queque interface
    (*bqPlayerObject)->GetInterface(bqPlayerObject,SL_IID_BUFFERQUEUE,&bqPlayerBufferQueue);

    //register callback on the buffer queue
    (*bqPlayerBufferQueue)->RegisterCallback(bqPlayerBufferQueue,bqPlayerCallback,this);

    //get the volume interface
    (*bqPlayerObject)->GetInterface(bqPlayerObject,SL_IID_VOLUME,&bqPlayerVolume);

    //获取声道接口
    (*bqPlayerObject)->GetInterface(bqPlayerObject,SL_IID_MUTESOLO,&bqPlayerMuteSolo);

    //设置播放状态
    //set the player's state to playing
    (*bqPlayerPlay)->SetPlayState(bqPlayerPlay,SL_PLAYSTATE_PLAYING);

    //StartPlay
    bqPlayerCallback(bqPlayerBufferQueue,this);

}

void AudioDecoder::play() {

    initOpenSLES();
}

int AudioDecoder::getCurSampleRateForOpenSLES(int samplerate) {
    int rate = 0;
    switch (samplerate) {
        case 8000:
            rate = SL_SAMPLINGRATE_8;
            break;
        case 11025:
            rate = SL_SAMPLINGRATE_11_025;
            break;
        case 12000:
            rate = SL_SAMPLINGRATE_12;
            break;
        case 16000:
            rate = SL_SAMPLINGRATE_16;
            break;
        case 22050:
            rate = SL_SAMPLINGRATE_22_05;
            break;
        case 24000:
            rate = SL_SAMPLINGRATE_24;
            break;
        case 32000:
            rate = SL_SAMPLINGRATE_32;
            break;
        case 44100:
            rate = SL_SAMPLINGRATE_44_1;
            break;
        case 48000:
            rate = SL_SAMPLINGRATE_48;
            break;
        case 64000:
            rate = SL_SAMPLINGRATE_64;
            break;
        case 88200:
            rate = SL_SAMPLINGRATE_88_2;
            break;
        case 96000:
            rate = SL_SAMPLINGRATE_96;
            break;
        case 192000:
            rate = SL_SAMPLINGRATE_192;
            break;
        default:
            rate =  SL_SAMPLINGRATE_44_1;
    }
    return rate;
}

int AudioDecoder::resample(uint8_t ** _buffer) {
    int data_size = 0;
    while(playStatus != NULL && !playStatus->exit) {

        if (playStatus->seek) {
            continue;
        }
        AVFrame *avFrame;
        if(audioQueue->getAVFrame(&avFrame) != 0) {
            av_frame_free(&avFrame);
            av_free(avFrame);
            avFrame = NULL;
            LOGE("Error getAVFrame fail");
            continue;
        }

        if(avFrame->channels && avFrame->channel_layout == 0) {
            avFrame->channel_layout = (uint64_t) av_get_default_channel_layout(avFrame->channels);
        } else if (avFrame->channels == 0 && avFrame->channel_layout > 0) {
            avFrame->channels = av_get_channel_layout_nb_channels(avFrame->channel_layout);
        }
        SwrContext *swr_ctx;

        swr_ctx = swr_alloc_set_opts(NULL,
                                     AV_CH_LAYOUT_STEREO,
                                     AV_SAMPLE_FMT_S16,
                                     avFrame->sample_rate,
                                     avFrame->channel_layout,
                                     (AVSampleFormat) avFrame->format,
                                     avFrame->sample_rate,
                                     NULL, NULL);
        if(!swr_ctx || swr_init(swr_ctx) <0) {
            av_frame_free(&avFrame);
            av_free(avFrame);
            avFrame = NULL;
            swr_free(&swr_ctx);
            LOGE("Error swr_init fail");
            continue;
        }

        samples = swr_convert(swr_ctx,
                              &out_buffer,
                              avFrame->nb_samples,
                              (const uint8_t **) avFrame->data,
                              avFrame->nb_samples);

        int out_channels = av_get_channel_layout_nb_channels(AV_CH_LAYOUT_STEREO);
        data_size = samples * out_channels * av_get_bytes_per_sample(AV_SAMPLE_FMT_S16);

        *_buffer = out_buffer;
        now_time = avFrame->pts * av_q2d(time_base);
        if (now_time > actual_time) {
            actual_time = now_time;
        }
        av_frame_free(&avFrame);
        av_free(avFrame);
        avFrame = NULL;
        swr_free(&swr_ctx);
        break;

    }
    return data_size;
}

void AudioDecoder::pause() {

    if (bqPlayerPlay != NULL){
        (*bqPlayerPlay)->SetPlayState(bqPlayerPlay,SL_PLAYSTATE_PAUSED);
    }

}

void AudioDecoder::resume() {

    if (bqPlayerPlay != NULL){
        (*bqPlayerPlay)->SetPlayState(bqPlayerPlay,SL_PLAYSTATE_PLAYING);
    }

}

void AudioDecoder::stop() {
    if (bqPlayerPlay != NULL) {
        (*bqPlayerPlay)->SetPlayState(bqPlayerPlay,SL_PLAYSTATE_STOPPED);
    }
}

void AudioDecoder::release() {
    LOGE("内存开始释放了");
    if(playStatus != NULL && playStatus->exit) {
        //中止解码，并释放
        playStatus->exit = true;

        pthread_mutex_lock(&init_mutex);
        int wait_count = 0;
        while (!exit){
            //还未退出
            if (wait_count > 1000) {
                exit = true;
            }
            wait_count++;
            av_usleep(10000);
        }
        pthread_mutex_unlock(&init_mutex);
    }
    stop();
    if (audioQueue != NULL) {
        delete(audioQueue);
        audioQueue = NULL;
    }
    if(bqPlayerObject != NULL) {
        (*bqPlayerObject)->Destroy(bqPlayerObject);
        bqPlayerObject = NULL;
        bqPlayerPlay = NULL;
        bqPlayerBufferQueue = NULL;
        bqPlayerMuteSolo = NULL;
        bqPlayerVolume = NULL;
    }
    if (outputMixObject != NULL) {
        (*outputMixObject)->Destroy(outputMixObject);
        outputMixObject = NULL;
        outputMixEnvironmentalReverb = NULL;
    }
    if (engineObject != NULL) {
        (*engineObject)->Destroy(engineObject);
        engineObject = NULL;
        engineEngine = NULL;
    }
    pthread_mutex_destroy(&audioEngineLock);
    if (out_buffer != NULL) {
        free(out_buffer);
        out_buffer = NULL;
    }
    if (buffer != NULL) {
        buffer = NULL;
    }
    if (soundTouch != NULL) {
        delete soundTouch;
        soundTouch = NULL;
    }
    if (soundtouch_buffer != NULL) {
        free(soundtouch_buffer);
        soundtouch_buffer = NULL;
    }
    if (pCodeCtx != NULL) {
        avcodec_close(pCodeCtx);
        avcodec_free_context(&pCodeCtx);
        pCodeCtx = NULL;
    }
    if (pFormatCtx != NULL) {
        avformat_close_input(&pFormatCtx);
        avformat_free_context(pFormatCtx);
        pFormatCtx = NULL;
    }
    if (playStatus != NULL) {
        playStatus = NULL;
    }
    if(callback != NULL) {
        callback = NULL;
    }
    LOGE("内存释放了");
}

void AudioDecoder::unlockInitMutex() {
    exit = true;
    pthread_mutex_unlock(&init_mutex);
}

void AudioDecoder::seek(int seconds) {

    if (total_time <= 0 || seconds > total_time)return;

    pthread_mutex_lock(&seek_mutex);
    playStatus->seek = true;
    audioQueue->clearQueue();
    actual_time = 0;
    last_time = 0;
    int64_t rel = seconds * AV_TIME_BASE;
    avformat_seek_file(pFormatCtx,-1,INT64_MIN,rel,INT64_MAX,0);
    playStatus->seek = false;
    pthread_mutex_unlock(&seek_mutex);

}

void AudioDecoder::setVolume(int percent) {

    if (bqPlayerVolume != NULL) {
        (*bqPlayerVolume)->SetVolumeLevel(bqPlayerVolume, (SLmillibel) ((100 - percent) * -50));
    }
}

void AudioDecoder::setSoundChannel(int channel_id) {

    //channel_id :  0左声道，1右声道，2立体声
    if (bqPlayerMuteSolo != NULL) {
        (*bqPlayerMuteSolo)->SetChannelMute(bqPlayerMuteSolo, 0, (SLboolean) (channel_id == 1));//右声道
        (*bqPlayerMuteSolo)->SetChannelMute(bqPlayerMuteSolo, 1, (SLboolean) (channel_id == 0));//左声道
    }
}

int AudioDecoder::getSoundTouchData() {
    int data_size = 0;
    while(playStatus != NULL && !playStatus->exit) {
        buffer = NULL;
        if(finished) {
            finished = false;
            data_size = resample(&buffer);
            if(data_size > 0) {
                for(int i = 0; i < data_size / 2 + 1; i++) {
                    soundtouch_buffer[i] = (buffer[i * 2] | ((buffer[i * 2 + 1]) << 8));
                }
                soundTouch->putSamples(soundtouch_buffer, samples);
                numsample = soundTouch->receiveSamples(soundtouch_buffer, data_size / 4);
            } else{
                soundTouch->flush();
            }
        }
        if(numsample == 0) {
            finished = true;
            continue;
        } else{
            if(buffer == NULL) {
                numsample = soundTouch->receiveSamples(soundtouch_buffer, data_size / 4);
                if(numsample == 0) {
                    finished = true;
                    continue;
                }
            }
            return numsample;
        }
    }
    return 0;
}

void AudioDecoder::setTempo(float tempo) {

    if (soundTouch != NULL) {
        soundTouch->setTempo(tempo);
        LOGE("tempo:%f",tempo);
    }
}

void AudioDecoder::setPitch(float pitch) {

    if (soundTouch != NULL) {
        soundTouch->setPitch(pitch);
        LOGE("pitch:%f",pitch);
    }
}

int AudioDecoder::getVolumeLp(char *data, size_t data_size) {
    int lp = 0;
    short int pervalue = 0;
    double  sum = 0;
    for (int i = 0; i < data_size; i+=2) {
        memcpy(&pervalue,data + i,2);
        sum += abs(pervalue);
    }
    double avg = sum/(data_size / 2);
    if (avg > 0) {
        lp = (int) (20.0 * log10(avg));
    }
    return lp;
}

void AudioDecoder::startStopRecord(bool isStart) {
    isRecordPcm = isStart;
}


