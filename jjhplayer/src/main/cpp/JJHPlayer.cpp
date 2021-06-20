#include "JJHPlayer.h"

JJHPlayer::JJHPlayer(const char *data_source, JNICallbackHelper *jniCallbackHelper) {
    this->data_source = new char[strlen(data_source) + 1];
    this->jniCallbackHelper = jniCallbackHelper;
    strcpy(this->data_source, data_source);
}

JJHPlayer::~JJHPlayer() {
    if(data_source){
        delete data_source;
        data_source = nullptr;
    }
    if(jniCallbackHelper){
        delete jniCallbackHelper;
        jniCallbackHelper = nullptr;
    }
}

void JJHPlayer::setRenderCallback(RenderCallback callback) {
    this->renderCallback = callback;
}

void *taskPrepare(void * args){

    LOGD("taskPrepare...");

    auto *player = static_cast<JJHPlayer *>(args);

    player->_prepare();

    return nullptr;
}

void JJHPlayer::_prepare() {
    LOGI("_prepare...");
    /**
     * TODO 第一步：打开媒体地址（文件路径， 直播地址rtmp）
     */
    formatContext = avformat_alloc_context();

    AVDictionary *dictionary = nullptr;
    av_dict_set(&dictionary, "timeout", "5000000", 0);

    /**
     * 1，AVFormatContext *
     * 2，路径
     * 3，AVInputFormat *fmt  Mac、Windows 摄像头、麦克风， 我们目前安卓用不到
     * 4，各种设置：例如：Http 连接超时， 打开rtmp的超时  AVDictionary **options
     */
    int res = avformat_open_input(&formatContext, data_source, nullptr, &dictionary);

    av_dict_free(&dictionary);

    if(res){
        callBackErrorEvent(THREAD_CHILD, FFMPEG_CAN_NOT_OPEN_URL, 0, av_err2str(res));
        return;
    }

    LOGI("avformat_open_input...");

    /**
     * TODO 第二步：查找媒体中的音视频流的信息
     */
    res = avformat_find_stream_info(formatContext, nullptr);

    if(res < 0){
        callBackErrorEvent(THREAD_CHILD, FFMPEG_CAN_NOT_FIND_STREAMS, 0, av_err2str(res));
        return;
    }

    this->duration = formatContext->duration / (AV_TIME_BASE/1000);

    LOGI("avformat_find_stream_info : duration = %d ", duration);

    //formatContext->nb_streams; 流的个数
    /**
     * TODO 第三部 根据流信息 流个数 用循环查找
     */
    for(int i=0;i<formatContext->nb_streams;i++){
        /**
         * TODO 第四部 获取媒体流
         */
        AVStream *stream = formatContext->streams[i];
        /**
         * TODO 第五步 从上面的流中 获取编解码参数
         * 由于 后面的编码器 解码器都需要参数（宽高等等）
         */
        AVCodecParameters *parameters = stream->codecpar;
        /**
         * TODO 第六步 根据上面的参数 获取编解码器
         */
        AVCodec *codec = avcodec_find_decoder(parameters->codec_id);
        if(!codec){
            callBackErrorEvent(THREAD_CHILD, FFMPEG_FIND_DECODER_FAIL, 0, "can't find decoder!");
            return;
        }
        /**
         * TODO 第七部 编解码器上下文（真正干活的）
         */
        AVCodecContext *codecContext = avcodec_alloc_context3(codec);
        if(!codecContext){
            callBackErrorEvent(THREAD_CHILD, FFMPEG_ALLOC_CODEC_CONTEXT_FAIL, 0, "init AVCodecContext error!");
            return;
        }

        /**
         * TODO 第八步 parameters copy codecContext
         */
        res = avcodec_parameters_to_context(codecContext, parameters);
        if(res < 0){
            callBackErrorEvent(THREAD_CHILD, FFMPEG_CODEC_CONTEXT_PARAMETERS_FAIL, 0, av_err2str(res));
            return;
        }

        /**
         * TODO 第九步 打开解码器
         */
        res = avcodec_open2(codecContext, codec, nullptr);
        if(res){
            callBackErrorEvent(THREAD_CHILD, FFMPEG_OPEN_DECODER_FAIL, 0, av_err2str(res));
            return;
        }

        if(parameters->codec_type == AVMediaType::AVMEDIA_TYPE_AUDIO){
            audioChannel = new AudioChannel(i, codecContext, stream->time_base, jniCallbackHelper);
        }else if(parameters->codec_type == AVMediaType::AVMEDIA_TYPE_VIDEO){
            //虽然是视频类型，但是只是一帧封面
            if(stream->disposition & AV_DISPOSITION_ATTACHED_PIC){
                continue;
            }
            AVRational fps_rational = stream->avg_frame_rate;
            double fps = av_q2d(fps_rational);
            videoChannel = new VideoChannel(i, codecContext, stream->time_base, fps, jniCallbackHelper);
            videoChannel->setRenderCallback(renderCallback);
        }

    }

    if(!audioChannel && !videoChannel){
        callBackErrorEvent(THREAD_CHILD, FFMPEG_NO_MEDIA, 0, "no audio or video!");
        return;
    }

    LOGI("prepared finish...");

    if(jniCallbackHelper){
        jniCallbackHelper->onPrepared(THREAD_CHILD);
    }

}

void JJHPlayer::callBackErrorEvent(int threadMode, int eventCode, int extra, const char *desc){
    if(jniCallbackHelper){
        jniCallbackHelper->onErrorEvent(threadMode, eventCode, extra, desc);
    }
}

void JJHPlayer::callBackPlayerEvent(int threadMode, int eventCode, int extra, const char *desc) {
    if(jniCallbackHelper){
        jniCallbackHelper->onPlayerEvent(threadMode, eventCode, extra, desc);
    }
}

void JJHPlayer::prepare() {
    LOGD("prepare...");
    pthread_create(&pid_prepare, nullptr, taskPrepare, this);
}

void * task_start(void * args){

    auto *player = static_cast<JJHPlayer *>(args);
    player->_start();

    return nullptr;
}

void JJHPlayer::start() {
    isPlaying = true;

    // 视频：1.解码    2.播放
    // 1.把队列里面的压缩包(AVPacket *)取出来，然后解码成（AVFrame * ）原始包 ----> 保存队列
    // 2.把队列里面的原始包(AVFrame *)取出来， 视频播放
    if(videoChannel){
        videoChannel->setAudioChannel(audioChannel);
        videoChannel->start();
    }

    // 音频：1.解码    2.播放
    // 1.把队列里面的压缩包(AVPacket *)取出来，然后解码成（AVFrame * ）原始包 ----> 保存队列
    // 2.把队列里面的原始包(AVFrame *)取出来， 音频播放
    if(audioChannel)
        audioChannel->start();

    pthread_create(&pid_start, nullptr, task_start, this);
}

// 把 视频 音频 的压缩包(AVPacket *) 循环获取出来 加入到队列里面去
void JJHPlayer::_start() {
    AVPacket *packet = nullptr;
    while (isPlaying){
        packet = av_packet_alloc();
        int res = av_read_frame(formatContext, packet);
        if(!res){
            if(videoChannel && videoChannel->stream_index == packet->stream_index){
                videoChannel->packets.insert(packet);
            }else if(audioChannel && audioChannel->stream_index == packet->stream_index){
                audioChannel->packets.insert(packet);
            }
        }else if(res == AVERROR_EOF){ //   end of file == 读到文件末尾了 == AVERROR_EOF
            // 表示读完了，要考虑释放播放完成，表示读完了 并不代表播放完毕
            if (videoChannel->packets.empty() && audioChannel->packets.empty()) {
                break; // 队列的数据被音频 视频 全部播放完毕了，我在退出
            }
        }else{
            break;//出现错误，结束当前循环
        }
    }
    BaseChannel::releaseAVPacket(&packet);
    stop();
}

void JJHPlayer::pause() {

}

void JJHPlayer::stop() {
    isPlaying = false;
    videoChannel->stop();
    audioChannel->stop();
    DELETE(audioChannel);
    DELETE(videoChannel);
    LOGD("JJHPlayer stop");
}

void * task_release(void * args){

    auto *player = static_cast<JJHPlayer *>(args);
    player->_release(player);

    return nullptr;
}

void JJHPlayer::_release(JJHPlayer *player) {

    stop();

    pthread_join(pid_prepare, nullptr);
    pthread_join(pid_start, nullptr);

    if(formatContext){
        avformat_close_input(&formatContext);
        avformat_free_context(formatContext);
        formatContext = nullptr;
    }

    DELETE(audioChannel);
    DELETE(videoChannel);
    DELETE(player);

    LOGD("JJHPlayer _release");

}

void JJHPlayer::release() {
    DELETE(jniCallbackHelper);
    if(audioChannel){
        DELETE(audioChannel->callbackHelper);
    }
    if(videoChannel){
        DELETE(videoChannel->callbackHelper);
    }

    // 如果是直接释放 我们的 prepare_ start_ 线程，不能暴力释放 ，否则会有bug

    // 让他 稳稳的停下来

    // 我们要等这两个线程 稳稳的停下来后，我再释放DerryPlayer的所以工作
    // 由于我们要等 所以会ANR异常

    // 所以我们我们在开启一个 stop_线程 来等你 稳稳的停下来
    // 创建子线程
    pthread_create(&pid_stop, nullptr, task_release, this);
}

bool JJHPlayer::isPlayingState() {
    return isPlaying;
}

int JJHPlayer::getCurrentPosition() {
    if(audioChannel && getDuration()){
        return audioChannel->audio_time*1000;
    }
    return 0;
}

int JJHPlayer::getDuration() {
    return this->duration;
}

void JJHPlayer::seekTo(int position) {
    if(position < 0 || !getDuration()){
        return;
    }
    if(position > getDuration()){
        return;
    }
    if(!formatContext){
        return;
    }
    LOGI("seekTo : position = %d", position);
    int res = av_seek_frame(formatContext, -1,
                            position * (AV_TIME_BASE/1000), AVSEEK_FLAG_BACKWARD | AVSEEK_FLAG_FRAME);

    LOGI("av_seek_frame : result = %d", res);

    if(res < 0){
        LOGE("av_seek_frame error , msg = %s", av_err2str(res));
        return;
    }

    if(audioChannel){
        audioChannel->prepareSeek();
    }
    if(videoChannel){
        videoChannel->prepareSeek();
    }
}