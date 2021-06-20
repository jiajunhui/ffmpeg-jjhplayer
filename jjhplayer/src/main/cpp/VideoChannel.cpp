#include "VideoChannel.h"

/**
 * 丢包 AVPacket * 压缩包 考虑关键帧
 * @param q
 */
void dropAVPacket(queue<AVPacket*> &q){
    while (!q.empty()){
        AVPacket *packet = q.front();
        if(packet->flags != AV_PKT_FLAG_KEY){
            BaseChannel::releaseAVPacket(&packet);
            q.pop();
        }
        break;
    }
}

/**
 * 丢包 AVFrame * 原始包 很简单，因为不需要考虑 关键帧
 * @param q
 */
void dropAVFrame(queue<AVFrame*> &q){
    if(!q.empty()){
        AVFrame *frame = q.front();
        BaseChannel::releaseAVFrame(&frame);
        q.pop();
    }
}

VideoChannel::VideoChannel(int stream_index, AVCodecContext *avCodecContext, AVRational time_base, double fps, JNICallbackHelper * jniCallbackHelper)
: BaseChannel(stream_index, avCodecContext, time_base, jniCallbackHelper) {
    this->fps = fps;
    packets.setSyncCallback(dropAVPacket);
    frames.setSyncCallback(dropAVFrame);
}

VideoChannel::~VideoChannel(){

}

void * task_video_decode(void * args){

    auto *videoChannel = static_cast<VideoChannel *>(args);
    videoChannel->video_decode();

    return nullptr;
}

void * task_video_play(void * args){

    auto *videoChannel = static_cast<VideoChannel *>(args);
    videoChannel->video_play();

    return nullptr;
}

void VideoChannel::setRenderCallback(RenderCallback callback) {
    this->renderCallback = callback;
}

void VideoChannel::setAudioChannel(AudioChannel *audioChannel) {
    this->audio_channel = audioChannel;
}

void VideoChannel::start() {
    isPlaying = true;

    packets.setWork(1);
    frames.setWork(1);

    //第一个线程：取出队列的压缩包 解码 解码后的原始包push到frame队列中去
    pthread_create(&pid_video_decode, nullptr, task_video_decode, this);

    //第二个线程：从frame队列中取出原始包 播放
    pthread_create(&pid_video_play, nullptr, task_video_play, this);

    LOGD("video channel start")
}

// 1.把队列里面的压缩包(AVPacket *)取出来，然后解码成（AVFrame * ）原始包 ----> 保存队列  【真正干活的就是他】
void VideoChannel::video_decode() {
    AVPacket *packet = nullptr;
    while (isPlaying){
        int res = packets.getQueueAndDel(packet);
        if(!isPlaying){
            releaseAVPacket(&packet);
            break;
        }
        if(!res){
            continue;
        }
        // 最新的FFmpeg，和旧版本差别很大， 新版本：1.发送pkt（压缩包）给缓冲区，  2.从缓冲区拿出来（原始包）
        res = avcodec_send_packet(codecContext, packet);
        if(res){
            LOGE("video channel decode AVPacket fail !!!")
            releaseAVPacket(&packet);
            break;// avcodec_send_packet 出现了错误，结束循环
        }
        //下面是从FFmpeg缓冲区获取原始包
        AVFrame *frame = av_frame_alloc();
        res = avcodec_receive_frame(codecContext, frame);
        if(res == AVERROR(EAGAIN)){
            // B帧  B帧参考前面成功  B帧参考后面失败   可能是P帧没有出来，再拿一次就行了
            continue;
        }else if(res != 0){
            LOGE("video channel receive AVFrame fail !!!")
            releaseAVPacket(&packet);
            break;//错误了
        }
        if(isPlaying){
            //拿到了原始包
            frames.insert(frame);
        }
        releaseAVPacket(&packet);
    }
    releaseAVPacket(&packet);
}



//2.把frames队列中的原始包AVFrame *取出来播放
//原始包YUV数据 使用libswscale 转换为RGBA数据

//转换算法
// SWS_FAST_BILINEAR == 很快 可能会模糊
// SWS_BILINEAR 适中算法
void VideoChannel::video_play() {

    AVFrame *frame = nullptr;
    uint8_t *dest_data[4];//RGBA 4字节
    int dst_lineSize[4];//RGBA

    av_image_alloc(dest_data, dst_lineSize,
                   codecContext->width, codecContext->height,
                   AV_PIX_FMT_RGBA, 1);

    //YUV -> RGBA
    SwsContext *swsContext = sws_getContext(
            //输入
            codecContext->width,
            codecContext->height,
            codecContext->pix_fmt,
            //输出
            codecContext->width,
            codecContext->height,
            AV_PIX_FMT_RGBA,
            SWS_BILINEAR,nullptr, nullptr, nullptr);
    while (isPlaying){
        int res = frames.getQueueAndDel(frame);
        if(!isPlaying){
            releaseAVFrame(&frame);
            break;//如果关闭了播放，跳出循环
        }
        if(!res){
            continue;//有可能生产者队列填充数据过慢，需要等一等
        }

        sws_scale(swsContext,
                  //输入
                  frame->data, frame->linesize,
                  0, codecContext->height,
                  //输出
                  dest_data, dst_lineSize);

        // TODO 音视频同步 3（根据fps来休眠） FPS间隔时间加入（我的视频 默默的播放，不要看起来怪怪的） == 要有延时感觉
        // 0.04是这一帧的真实时间加上延迟时间吧
        // 公式：extra_delay = repeat_pict / (2*fps)
        // 经验值 extra_delay:0.0400000
        double extra_delay = frame->repeat_pict / (2 * fps); // 在之前的编码时，加入的额外延时时间取出来（可能获取不到）
        double fps_delay = 1.0 / fps; // 根据fps得到延时时间（fps25 == 每秒25帧，计算每一帧的延时时间，0.040000）
        double real_delay = fps_delay + extra_delay; // 当前帧的延时时间  0.040000

        // fps间隔时间后的效果，任何播放器，都会有
        // 为什么不能用：根据是 视频的 fps延时在处理，和音频还没有任何关系
        // av_usleep(real_delay * 1000000);

        // >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> 下面是音视频同步
        double video_time = frame->best_effort_timestamp * av_q2d(time_base);
        double audio_time = audio_channel->audio_time;

        // 判断两个时间差值，一个快一个慢（快的等慢的，慢的快点追） == 你追我赶
        double time_diff = video_time - audio_time;
        if(time_diff > 0){
            //视频较快
            // 视频时间 > 音频时间： 要等音频，所以控制视频播放慢一点（等音频） 【睡眠】
            if (time_diff > 1)
            {   // 说明：音频预视频插件很大，TODO 拖动条 特色场景  音频 和 视频 差值很大，我不能睡眠那么久，否则是大Bug
                // av_usleep((real_delay + time_diff) * 1000000);

                // 如果 音频 和 视频 差值很大，我不会睡很久，我就是稍微睡一下
                av_usleep((real_delay * 2) * 1000000);
            }
            else
            {   // 说明：0~1之间：音频与视频差距不大，所以可以那（当前帧实际延时时间 + 音视频差值）
                av_usleep((real_delay + time_diff) * 1000000); // 单位是微妙：所以 * 1000000
            }
        }else if(time_diff < 0){
            //音频较快
            // 视频时间 < 音频时间： 要追音频，所以控制视频播放快一点（追音频） 【丢包】
            // 丢帧：不能睡意丢，I帧是绝对不能丢
            // 丢包：在frames 和 packets 中的队列

            // 经验值 0.05
            // -0.234454   fabs == 0.234454
            if(fabs(time_diff) <= 0.05){
                // 多线程（安全 同步丢包）
                frames.sync();
                continue; // 丢完取下一个包
            }
        }else{
            // 百分百同步，这个基本上很难做的
            LOGI("百分百同步了");
        }

        //ANativeWindow渲染
        if(renderCallback && isPlaying)
            renderCallback(dest_data[0], codecContext->width, codecContext->height, dst_lineSize[0]);
        releaseAVFrame(&frame);//释放原始包，因为已经被渲染过了，没用了
    }
    releaseAVFrame(&frame);
    isPlaying = false;
    av_freep(dest_data[0]);
    sws_freeContext(swsContext);
}

void VideoChannel::stop(){
    LOGD("VideoChannel prepare stop");
    isPlaying = false;
    pthread_join(pid_video_decode, nullptr);
    pthread_join(pid_video_play, nullptr);

    packets.setWork(0);
    frames.setWork(0);
    packets.clear();
    frames.clear();

    LOGD("VideoChannel stop");
}