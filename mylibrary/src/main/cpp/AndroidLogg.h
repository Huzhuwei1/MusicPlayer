//
// Created by Administrator on 2017/8/28.
//
#include <android/log.h>

#ifndef MUSICPLAYER_ANDROIDLOG_H
#define MUSICPLAYER_ANDROIDLOG_H
#endif //MUSICPLAYER_ANDROIDLOG_H


#define LOG_DEBUG true

#define LOGI(format, ...)  __android_log_print(ANDROID_LOG_INFO,  "(^_^)-----------", format, ##__VA_ARGS__)
#define LOGW(format, ...)  __android_log_print(ANDROID_LOG_WARN, "(>_<)-----------", format, ##__VA_ARGS__)
#define LOGD(format, ...)  __android_log_print(ANDROID_LOG_DEBUG, "(>_<)-----------", format, ##__VA_ARGS__)
#define LOGE(format, ...)  __android_log_print(ANDROID_LOG_ERROR, "(>_<)-----------", format, ##__VA_ARGS__)
