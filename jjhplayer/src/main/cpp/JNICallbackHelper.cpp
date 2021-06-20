#include "JNICallbackHelper.h"

JNICallbackHelper::JNICallbackHelper(JavaVM *vm, JNIEnv *env, jobject obj) {
    this->vm = vm;
    this->env = env;
    this->job = env->NewGlobalRef(obj);
    jclass clazz = env->GetObjectClass(obj);
    jmd_onPrepared = env->GetMethodID(clazz, "onNativeCallPrepared", "()V");
    jmd_onErrorEvent = env->GetMethodID(clazz, "onNativeCallErrorEvent", "(IILjava/lang/String;)V");
    jmd_onPlayerEvent = env->GetMethodID(clazz, "onNativeCallPlayerEvent", "(IILjava/lang/String;)V");
}

JNICallbackHelper::~JNICallbackHelper() {
    vm = nullptr;
    env->DeleteGlobalRef(job);
    job = nullptr;
    env = nullptr;
}

void JNICallbackHelper::onPrepared(int threadMode) {
    if(threadMode == THREAD_MAIN){
        env->CallVoidMethod(job, jmd_onPrepared);
    }else if(threadMode == THREAD_CHILD){
        JNIEnv *child_env;
        vm->AttachCurrentThread(&child_env, nullptr);
        child_env->CallVoidMethod(job, jmd_onPrepared);
        vm->DetachCurrentThread();
    }
}

void JNICallbackHelper::onErrorEvent(int threadMode, int eventCode, int extra, const char *desc) {
    LOGI("callBackErrorEvent threadMode : %d, eventCode : %d, extra : %d, desc : %s", threadMode, eventCode, extra, desc);
    if(threadMode == THREAD_MAIN){
        env->CallVoidMethod(job, jmd_onErrorEvent, eventCode, extra, desc);
    }else if(threadMode == THREAD_CHILD){
        JNIEnv *child_env;
        vm->AttachCurrentThread(&child_env, nullptr);
        jstring message = child_env->NewStringUTF(desc);
        child_env->CallVoidMethod(job, jmd_onErrorEvent, eventCode, extra, message);
//        child_env->ReleaseStringUTFChars(message, desc);//Release 会崩溃 ？
        vm->DetachCurrentThread();
    }
}

void JNICallbackHelper::onPlayerEvent(int threadMode, int eventCode, int extra, const char *desc) {
    LOGI("callBackPlayerEvent threadMode : %d, eventCode : %d, extra : %d, desc : %s", threadMode, eventCode, extra, desc);
    if(threadMode == THREAD_MAIN){
        env->CallVoidMethod(job, jmd_onPlayerEvent, eventCode, extra, desc);
    }else if(threadMode == THREAD_CHILD){
        JNIEnv *child_env;
        vm->AttachCurrentThread(&child_env, nullptr);
        jstring message = child_env->NewStringUTF(desc);
        child_env->CallVoidMethod(job, jmd_onPlayerEvent, eventCode, extra, message);
//        child_env->ReleaseStringUTFChars(message, desc);//Release 会崩溃 ？
        vm->DetachCurrentThread();
    }
}