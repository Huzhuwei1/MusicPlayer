//
// Created by Administrator on 2017/8/31.
//
#include "javaCallback.h"

javaCallback::javaCallback(JavaVM *javaVM, JNIEnv *jniEnv, jobject jobject) {
    jvm = javaVM;
    env = jniEnv;
    jobj = jobject;
    jclass jcls = env->GetObjectClass(jobject);
    if(!jcls){ return; }
    jmethodID_prepared = env->GetMethodID(jcls,"onPrepared","()V");
    jmethodID_playing = env->GetMethodID(jcls,"onPlaying","(II)V");
    jmethodID_error = env->GetMethodID(jcls,"onError","()V");
    jmethodID_complete = env->GetMethodID(jcls,"onComplete","()V");
}

javaCallback::~javaCallback() {}

void javaCallback::onPrepared(int type) {

    if(type == THREAD_TYPE_MAIN) {
        env->CallVoidMethod(jobj,jmethodID_prepared);
    } else if (type == THREAD_TYPE_CHILD){
        JNIEnv* jni_env;
        if(JNI_OK != jvm->AttachCurrentThread(&jni_env,0)) {
            LOGE("cannot get JNIEnv");
            return;
        }
        jni_env->CallVoidMethod(jobj,jmethodID_prepared);
        jvm->DetachCurrentThread();
    }
}

void javaCallback::onPlaying(int type, int current_time, int total_time) {

    if(type == THREAD_TYPE_MAIN) {
        env->CallVoidMethod(jobj,jmethodID_playing,current_time,total_time);
    } else if (type == THREAD_TYPE_CHILD){
        JNIEnv* jni_env;
        if(JNI_OK != jvm->AttachCurrentThread(&jni_env,0)) {
            LOGE("cannot get JNIEnv");
            return;
        }
        jni_env->CallVoidMethod(jobj,jmethodID_playing,current_time,total_time);
        jvm->DetachCurrentThread();
    }
}

void javaCallback::onError(int type) {

    if(type == THREAD_TYPE_MAIN) {
        env->CallVoidMethod(jobj,jmethodID_error);
    } else if (type == THREAD_TYPE_CHILD){
        JNIEnv* jni_env;
        if(JNI_OK != jvm->AttachCurrentThread(&jni_env,0)) {
            LOGE("cannot get JNIEnv");
            return;
        }
        jni_env->CallVoidMethod(jobj,jmethodID_error);
        jvm->DetachCurrentThread();
    }
}

void javaCallback::onComplete(int type) {

    if(type == THREAD_TYPE_MAIN) {
        env->CallVoidMethod(jobj,jmethodID_complete);
    } else if (type == THREAD_TYPE_CHILD){
        JNIEnv* jni_env;
        if(JNI_OK != jvm->AttachCurrentThread(&jni_env,0)) {
            LOGE("cannot get JNIEnv");
            return;
        }
        jni_env->CallVoidMethod(jobj,jmethodID_complete);
        jvm->DetachCurrentThread();
    }
}

