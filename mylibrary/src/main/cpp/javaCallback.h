//
// Created by Administrator on 2017/8/31.
//

#ifndef MUSICPLAYER_JAVACALLBACK_H
#define MUSICPLAYER_JAVACALLBACK_H

#include <jni.h>
#include "AndroidLogg.h"

#define THREAD_TYPE_MAIN 0
#define THREAD_TYPE_CHILD 1

class javaCallback{

public:JNIEnv *env;
       JavaVM *jvm;
       jobject jobj;
       jmethodID jmethodID_prepared;
       jmethodID jmethodID_playing;
       jmethodID jmethodID_error;
       jmethodID jmethodID_complete;

public:javaCallback(JavaVM *javaVM, JNIEnv *jniEnv, jobject jobject);
       ~javaCallback();
       void onPrepared(int type);
       void onPlaying(int type, int current_time, int total_time);
       void onError(int type);
       void onComplete(int type);
};

#endif //MUSICPLAYER_JAVACALLBACK_H
