#include <cstring>
#include <pthread.h>
#include "AudioChannel.h"
#include "VideoChannel.h"
#include "JNICallbackHelper.h"
#include "log.h"
#include "player_event.h"
#include "error.h"
#include "safe_queue.h"

extern "C"{
#include <libavformat/avformat.h>
}

#ifndef JJHPLAYER_JJHPLAYER_H
#define JJHPLAYER_JJHPLAYER_H

class JJHPlayer {
private:
    char *data_source = 0;
    pthread_t pid_prepare;
    pthread_t pid_start;
    pthread_t pid_stop;
    AVFormatContext *formatContext;
    AudioChannel *audioChannel = 0;
    VideoChannel *videoChannel = 0;
    JNICallbackHelper *jniCallbackHelper = 0;
    bool isPlaying;
    RenderCallback renderCallback;
    int duration = 0;

    void callBackErrorEvent(int threadMode, int eventCode, int extra, const char *desc);
    void callBackPlayerEvent(int threadMode, int eventCode, int extra, const char *desc);

public:
    JJHPlayer(const char *data_source, JNICallbackHelper *jniCallbackHelper);
    ~JJHPlayer();

    void setRenderCallback(RenderCallback callback);

    void prepare();
    void _prepare();

    void start();
    void _start();

    void pause();

    void stop();

    void release();
    void _release(JJHPlayer *player);

    bool isPlayingState();

    int getCurrentPosition();

    int getDuration();

    void seekTo(int position);
};


#endif //JJHPLAYER_JJHPLAYER_H
