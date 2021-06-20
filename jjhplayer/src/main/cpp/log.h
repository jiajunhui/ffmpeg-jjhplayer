#include <android/log.h>
#ifndef JJHPLAYER_LOG_H
#define JJHPLAYER_LOG_H

#define TAG "player_native"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, TAG, __VA_ARGS__);
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, TAG, __VA_ARGS__);
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN, TAG, __VA_ARGS__);
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, TAG, __VA_ARGS__);

#endif //JJHPLAYER_LOG_H
