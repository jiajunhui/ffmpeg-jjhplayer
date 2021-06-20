#include <jni.h>
#include <string>
#include "JNICallbackHelper.h"
#include "JJHPlayer.h"
#include <android/native_window_jni.h>

JavaVM *vm = nullptr;
JJHPlayer *player = nullptr;
ANativeWindow *window = nullptr;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;//静态初始化锁

jint JNI_OnLoad(JavaVM *vm, void * args){
    ::vm = vm;
    return JNI_VERSION_1_6;
}

//函数指针
//实现渲染
void renderCallback(uint8_t *src_data, int width, int height, int src_lineSize){
    pthread_mutex_lock(&mutex);
    if(!window){
        pthread_mutex_unlock(&mutex);//出现问题后，必须考虑到，释放锁，避免出现死锁问题
        return;
    }

    //设置窗口的大小，各个属性
    ANativeWindow_setBuffersGeometry(window, width, height, WINDOW_FORMAT_RGBA_8888);

    //window的缓冲区buffer
    ANativeWindow_Buffer windowBuffer;

    //如果我在渲染的时候是被锁住的，那我就无法渲染，我需要释放，防止出现死锁
    if(ANativeWindow_lock(window, &windowBuffer, nullptr)){
        LOGE("window is locked!!!");
        ANativeWindow_release(window);
        window = nullptr;
        pthread_mutex_unlock(&mutex);
        return;
    }

    //开始真正渲染，window没有被锁住 把RGBA数据  ---> 字节对齐 渲染
    uint8_t *dst_data = static_cast<uint8_t *>(windowBuffer.bits);
    int dst_lineSize = windowBuffer.stride * 4;

    for(int i=0;i<windowBuffer.height;i++){
        memcpy(dst_data + i * dst_lineSize, src_data + i * src_lineSize, dst_lineSize);
    }

    //数据刷新
    //解锁后 并且刷新 window_buffer 的数据显示画面
    ANativeWindow_unlockAndPost(window);

    pthread_mutex_unlock(&mutex);
}

extern "C"
JNIEXPORT void JNICALL
Java_cn_jiajunhui_lib_jjhplayer_JJHPlayer_nativePrepare(JNIEnv* env, jobject thiz, jstring data_source) {
    const char *_data_source = env->GetStringUTFChars(data_source, 0);
    auto *helper = new JNICallbackHelper(vm, env, thiz);
    player = new JJHPlayer(_data_source, helper);
    player->setRenderCallback(renderCallback);
    player->prepare();
    env->ReleaseStringUTFChars(data_source, _data_source);
}

extern "C"
JNIEXPORT void JNICALL
Java_cn_jiajunhui_lib_jjhplayer_JJHPlayer_nativeStart(JNIEnv * env, jobject thiz) {
    if(player)
        player->start();
}

extern "C"
JNIEXPORT void JNICALL
Java_cn_jiajunhui_lib_jjhplayer_JJHPlayer_nativePause(JNIEnv
* env,
jobject thiz
) {
// TODO: implement nativePause()
}

extern "C"
JNIEXPORT void JNICALL
Java_cn_jiajunhui_lib_jjhplayer_JJHPlayer_nativeStop(JNIEnv
* env,
jobject thiz
) {
    if(player)
        player->stop();
}

extern "C"
JNIEXPORT void JNICALL
Java_cn_jiajunhui_lib_jjhplayer_JJHPlayer_nativeRelease(JNIEnv
* env,
jobject thiz
) {
    LOGD("native-lib prepare release");
    pthread_mutex_lock(&mutex);

    // 先释放之前的显示窗口
    if (window) {
        ANativeWindow_release(window);
        window = nullptr;
    }

    pthread_mutex_unlock(&mutex);

    if(player)
        player->release();
    DELETE(player);
    DELETE(vm);
    LOGD("native-lib release");
}


//实例化window
extern "C"
JNIEXPORT void JNICALL
Java_cn_jiajunhui_lib_jjhplayer_JJHPlayer_nativeSetSurface(JNIEnv *env, jobject thiz,
                                                           jobject surface) {
    pthread_mutex_lock(&mutex);

    //先释放之前的显示窗口
    if(window){
        ANativeWindow_release(window);
        window = nullptr;
    }

    //创建新的窗口用于视频显示
    window = ANativeWindow_fromSurface(env, surface);

    LOGD("set surface init window")

    pthread_mutex_unlock(&mutex);
}

extern "C"
JNIEXPORT jboolean JNICALL
Java_cn_jiajunhui_lib_jjhplayer_JJHPlayer_nativeIsPlaying(JNIEnv *env, jobject thiz) {
    if(player)
        return player->isPlayingState();
    return false;
}

extern "C"
JNIEXPORT jint JNICALL
Java_cn_jiajunhui_lib_jjhplayer_JJHPlayer_nativeGetCurrentPosition(JNIEnv *env, jobject thiz) {
    if(player)
        return player->getCurrentPosition();
    return 0;
}

extern "C"
JNIEXPORT jint JNICALL
Java_cn_jiajunhui_lib_jjhplayer_JJHPlayer_nativeGetDuration(JNIEnv *env, jobject thiz) {
    if(player)
        return player->getDuration();
    return 0;
}

extern "C"
JNIEXPORT void JNICALL
Java_cn_jiajunhui_lib_jjhplayer_JJHPlayer_nativeSeekTo(JNIEnv *env, jobject thiz, jint position) {
    if(player)
        player->seekTo(position);
}