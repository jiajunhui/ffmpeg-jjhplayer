#ifndef JJHPLAYER_VIDEOCHANNEL_H
#define JJHPLAYER_VIDEOCHANNEL_H

#include "BaseChannel.h"
#include "AudioChannel.h"
#include "log.h"

extern "C"{
#include <libswscale/swscale.h>
#include <libavutil/avutil.h>
#include <libavutil/imgutils.h>
#include <libavutil/time.h>
};

typedef void(*RenderCallback)(uint8_t *, int , int , int);//函数指针声明定义

// 视频：1.解码    2.播放
// 1.把队列里面的压缩包(AVPacket *)取出来，然后解码成（AVFrame * ）原始包 ----> 保存队列
// 2.把队列里面的原始包(AVFrame *)取出来， 播放
class VideoChannel : public BaseChannel {
private:
    pthread_t pid_video_decode;
    pthread_t pid_video_play;
    RenderCallback renderCallback;

    double fps;// fps是视频通道独有的，fps（一秒钟多少帧）
    AudioChannel *audio_channel;

public:
    VideoChannel(int stream_index, AVCodecContext * avCodecContext, AVRational time_base, double fps, JNICallbackHelper * jniCallbackHelper);
    ~VideoChannel();

    void setRenderCallback(RenderCallback callback);

    void setAudioChannel(AudioChannel *audioChannel);

    void start();

    void stop();

    void video_decode();
    void video_play();

};


#endif //JJHPLAYER_VIDEOCHANNEL_H
