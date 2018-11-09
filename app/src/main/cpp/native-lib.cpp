#include <jni.h>
#include <string>
#include "../../../../mylibrary/src/main/cpp/AndroidLogg.h"
extern "C"
JNIEXPORT jstring JNICALL
Java_com_hzw_mylibrary_Demo_testJni(JNIEnv *env, jobject instance) {

    // TODO
    std::string hello = "Hello from C++";


    return env->NewStringUTF(hello.c_str());
}