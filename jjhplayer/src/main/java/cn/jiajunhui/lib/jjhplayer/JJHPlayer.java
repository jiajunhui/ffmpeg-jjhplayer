package cn.jiajunhui.lib.jjhplayer;

import android.content.Context;
import android.os.Handler;
import android.os.Looper;
import android.view.Surface;

public class JJHPlayer implements IJJHPlayer {

    private Context mContext;
    private Handler mHandler = new Handler(Looper.getMainLooper());
    private String mDataSource;

    private OnPrepareListener mOnPrepareListener;
    private OnErrorEventListener mOnErrorEventListener;
    private OnPlayEventListener mOnPlayEventListener;

    public void setOnPrepareListener(OnPrepareListener onPrepareListener) {
        this.mOnPrepareListener = onPrepareListener;
    }

    public void setOnErrorEventListener(OnErrorEventListener onErrorEventListener) {
        this.mOnErrorEventListener = onErrorEventListener;
    }

    public void setOnPlayEventListener(OnPlayEventListener onPlayerListener) {
        this.mOnPlayEventListener = onPlayerListener;
    }

    static {
        System.loadLibrary("native-lib");
    }

    public JJHPlayer(Context context){
        this.mContext = context;
    }

    @Override
    public void setDataSource(String dataSource){
        this.mDataSource = dataSource;
    }

    @Override
    public void prepare(){
        nativePrepare(mDataSource);
    }


    @Override
    public void start(){
        nativeStart();
    }


    @Override
    public void pause(){
        nativePause();
    }


    @Override
    public void stop(){
        nativeStop();
    }


    @Override
    public void release(){
        nativeRelease();
    }

    @Override
    public boolean isPlaying() {
        return nativeIsPlaying();
    }

    @Override
    public int getCurrentPosition() {
        return nativeGetCurrentPosition();
    }

    @Override
    public int getDuration() {
        return nativeGetDuration();
    }

    @Override
    public void seekTo(int position) {
        nativeSeekTo(position);
    }

    private void onNativeCallErrorEvent(int eventCode, int extra, String message){
        mHandler.post(new Runnable() {
            @Override
            public void run() {
                if(mOnErrorEventListener!=null)
                    mOnErrorEventListener.onError(eventCode, extra,  message);
            }
        });
    }

    private void onNativeCallPrepared(){
        mHandler.post(new Runnable() {
            @Override
            public void run() {
                if(mOnPrepareListener!=null)
                    mOnPrepareListener.onPrepared(JJHPlayer.this);
            }
        });
    }

    //native call method
    private void onNativeCallPlayerEvent(int eventCode, int extra, String message){
        mHandler.post(new Runnable() {
            @Override
            public void run() {
                if(mOnPlayEventListener!=null)
                    mOnPlayEventListener.onPlayEvent(eventCode, extra, message);
            }
        });
    }

    public void setSurface(Surface surface){
        nativeSetSurface(surface);
    }

    private native void nativePrepare(String dataSource);
    private native void nativeSetSurface(Surface surface);
    private native void nativeStart();
    private native void nativePause();
    private native void nativeStop();
    private native void nativeRelease();

    private native boolean nativeIsPlaying();
    private native int nativeGetCurrentPosition();
    private native int nativeGetDuration();
    private native void nativeSeekTo(int position);

}
