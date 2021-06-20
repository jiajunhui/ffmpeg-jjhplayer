#ifndef JJHPLAYER_SAFE_QUEUE_H
#define JJHPLAYER_SAFE_QUEUE_H

#include <queue>
#include <pthread.h>

using namespace std;

template<typename T>
class SafeQueue{
private:
    typedef void (*ReleaseCallback)(T *);

    typedef void (*SyncCallback)(queue<T> &);

private:
    queue<T> queue;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    int work;
    ReleaseCallback releaseCallback;
    SyncCallback syncCallback;
    int maxSize = 0;
public:
    SafeQueue(){
        pthread_mutex_init(&mutex, nullptr);
        pthread_cond_init(&cond, nullptr);
    }
    ~SafeQueue(){
        pthread_mutex_destroy(&mutex);
        pthread_cond_destroy(&cond);
    }

    /**
     * 入队 [ AVPacket *  压缩包]  [ AVFrame * 原始包]
     */
    void insert(T value){
        pthread_mutex_lock(&mutex);

        if(work){
            while(maxSize && size() >= maxSize){
                pthread_cond_wait(&cond, &mutex);
            }
            if(work)
                queue.push(value);
            pthread_cond_signal(&cond);
        }else{
            if(releaseCallback)
                releaseCallback(&value);
        }

        pthread_mutex_unlock(&mutex);
    }

    /**
     *  出队 [ AVPacket *  压缩包]  [ AVFrame * 原始包]
     */
    int getQueueAndDel(T &value){
        int res = 0;
        pthread_mutex_lock(&mutex);

        while(work && queue.empty()){
            pthread_cond_wait(&cond, &mutex);
        }

        if(!queue.empty() && work){
            value = queue.front();
            queue.pop();
            res = 1;
        }

        if(maxSize && res){
            pthread_cond_signal(&cond);
        }

        pthread_mutex_unlock(&mutex);

        return res;
    }

    // Activity onDestroy  ----> setWork(0);
    /**
    * 设置工作状态，设置队列是否工作
    * @param work
    */
    void setWork(int work){
        pthread_mutex_lock(&mutex);
        this->work = work;
        pthread_cond_signal(&cond);
        pthread_mutex_unlock(&mutex);
    }

    int empty(){
        return queue.empty();
    }

    int size(){
        return queue.size();
    }

    /**
     * 清空队列中所有的数据，循环一个一个的删除
     */
    void clear(){
        pthread_mutex_lock(&mutex);

        unsigned int size = queue.size();
        for(int i = 0; i<size;i++){
            T value = queue.front();
            if(releaseCallback)
                releaseCallback(&value);
            queue.pop();
        }

        pthread_mutex_unlock(&mutex);
    }

    /**
     * 设置此函数指针的回调，让外界去释放
     * @param callback
     */
    void setReleaseCallback(ReleaseCallback callback){
        this->releaseCallback = callback;
    }

    void setSyncCallback(SyncCallback callback){
        this->syncCallback = callback;
    }

    void setMax(int max_size){
        this->maxSize = max_size;
    }

    /**
     * 同步操作 丢包
     */
    void sync(){
        pthread_mutex_lock(&mutex);
        syncCallback(queue);// 函数指针 具体丢包动作，让外界完成
        pthread_mutex_unlock(&mutex);
    }
};

#endif //JJHPLAYER_SAFE_QUEUE_H
