#include <jni.h>
#include "util.h"

#ifndef JJHPLAYER_JNICALLBACKHELPER_H
#define JJHPLAYER_JNICALLBACKHELPER_H

#include "log.h"

class JNICallbackHelper {
private:
    JavaVM *vm = 0;
    JNIEnv *env = 0;
    jobject job;
    jmethodID jmd_onPrepared;
    jmethodID jmd_onErrorEvent;
    jmethodID jmd_onPlayerEvent;
public:
    JNICallbackHelper(JavaVM *vm, JNIEnv *env, jobject obj);
    ~JNICallbackHelper();
    void onPrepared(int threadMode);
    void onErrorEvent(int threadMode, int eventCode, int extra, const char *desc);
    void onPlayerEvent(int threadMode, int eventCode, int extra, const char *desc);
};


#endif //JJHPLAYER_JNICALLBACKHELPER_H
