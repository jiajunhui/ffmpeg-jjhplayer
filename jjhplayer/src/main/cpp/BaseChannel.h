#ifndef JJHPLAYER_BASECHANNEL_H
#define JJHPLAYER_BASECHANNEL_H

extern "C"{
#include <libavcodec/avcodec.h>
};
#include "safe_queue.h"
#include "log.h"
#include "JNICallbackHelper.h"
#include "player_event.h"

class BaseChannel{
public:
    int stream_index;
    SafeQueue<AVPacket *> packets;
    SafeQueue<AVFrame *> frames;
    bool isPlaying;
    AVCodecContext *codecContext = 0;

    AVRational time_base;

    JNICallbackHelper *callbackHelper = 0;

    BaseChannel(int stream_index, AVCodecContext * codecCtx, AVRational time_base, JNICallbackHelper *helper)
    : stream_index(stream_index), codecContext(codecCtx), time_base(time_base), callbackHelper(helper){
        packets.setMax(100);
        frames.setMax(100);
        packets.setReleaseCallback(releaseAVPacket);
        frames.setReleaseCallback(releaseAVFrame);
    }

    virtual ~BaseChannel(){
        packets.clear();
        frames.clear();
    }

    void prepareSeek(){
        packets.setWork(0);
        frames.setWork(0);
        packets.clear();
        frames.clear();
        packets.setWork(1);
        frames.setWork(1);
    }

    void callBackErrorEvent(int threadMode, int eventCode, int extra, const char *desc){
        if(callbackHelper){
            callbackHelper->onErrorEvent(threadMode, eventCode, extra, desc);
        }
    }

    void callBackPlayerEvent(int threadMode, int eventCode, int extra, const char *desc) {
        if(callbackHelper){
            callbackHelper->onPlayerEvent(threadMode, eventCode, extra, desc);
        }
    }

    static void releaseAVPacket(AVPacket ** p){
        if(p){
            av_packet_unref(*p);
            av_packet_free(p);
            *p = nullptr;
        }
    }

    static void releaseAVFrame(AVFrame ** f){
        if(f){
            av_frame_unref(*f);
            av_frame_free(f);
            *f = nullptr;
        }
    }
};


#endif //JJHPLAYER_BASECHANNEL_H
