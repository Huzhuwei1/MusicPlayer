#include <jni.h>
#include "AudioPlayerController.h"
#include "javaCallback.h"
#include "HPlayStatus.h"

JavaVM *jvm;

javaCallback *callback;

AudioPlayerController *controller;

HPlayStatus *playStatus;

JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM* javaVM, void* reserved){
    jvm = javaVM;

    JNIEnv *env;
    if (javaVM->GetEnv((void**)env,JNI_VERSION_1_6) != JNI_OK){
        return -1;
    }
    return JNI_VERSION_1_6;
}


extern "C"
JNIEXPORT void JNICALL
Java_com_hzw_mylibrary_Player_nativePrepare(JNIEnv *env, jobject instance, jstring url_) {
    const char *url = env->GetStringUTFChars(url_, 0);
    if(controller == NULL) {
        jobject jobj = env->NewGlobalRef(instance);
        callback = new javaCallback(jvm,env,jobj);
        playStatus = new HPlayStatus();
        controller = new AudioPlayerController(callback,playStatus);
    }
    controller->prepare(url);

}extern "C"
JNIEXPORT void JNICALL
Java_com_hzw_mylibrary_Player_nativeStart(JNIEnv *env, jobject instance) {

    // TODO
    if (controller != NULL) {
        controller->start();
    }

}extern "C"
JNIEXPORT void JNICALL
Java_com_hzw_mylibrary_Player_nativePause(JNIEnv *env, jobject instance) {

    // TODO
    if (controller != NULL) {
        controller->pause();
    }

}extern "C"
JNIEXPORT void JNICALL
Java_com_hzw_mylibrary_Player_nativeResume(JNIEnv *env, jobject instance) {

    // TODO
    if (controller != NULL) {
        controller->resume();
    }
}extern "C"
JNIEXPORT void JNICALL
Java_com_hzw_mylibrary_Player_nativeStop(JNIEnv *env, jobject instance) {

    // TODO
    if (controller != NULL) {
        controller->stop();
        delete(controller);
        controller = NULL;
    }

}extern "C"
JNIEXPORT void JNICALL
Java_com_hzw_mylibrary_Player_nativeSeek(JNIEnv *env, jobject instance, jint seconds) {

    // TODO
    if (controller != NULL && seconds > 0) {
        controller->seek(seconds);
    }

}extern "C"
JNIEXPORT void JNICALL
Java_com_hzw_mylibrary_Player_nativeSetVolume(JNIEnv *env, jobject instance, jint percent) {

    // TODO
    if (controller != NULL) {
        controller->setVolume(percent);
    }

}extern "C"
JNIEXPORT void JNICALL
Java_com_hzw_mylibrary_Player_nativeSetSoundChannel(JNIEnv *env, jobject instance, jint channel) {

    // TODO
    if (controller != NULL) {
        controller->setSoundChannel(channel);
    }

}extern "C"
JNIEXPORT void JNICALL
Java_com_hzw_mylibrary_Player_nativeSetTempo(JNIEnv *env, jobject instance, jfloat percent) {

    // TODO
    if (controller != NULL) {
        controller->setTempo(percent);
    }

}extern "C"
JNIEXPORT void JNICALL
Java_com_hzw_mylibrary_Player_nativeSetPitch(JNIEnv *env, jobject instance, jfloat percent) {

    // TODO
    if (controller != NULL) {
        controller->setPitch(percent);
    }

}