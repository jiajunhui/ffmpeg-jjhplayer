package cn.jiajunhui.app.jjhplayer;

import androidx.annotation.NonNull;
import androidx.appcompat.app.AppCompatActivity;

import android.os.Bundle;
import android.os.Environment;
import android.os.Handler;
import android.os.Looper;
import android.os.Message;
import android.util.Log;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.widget.SeekBar;
import android.widget.TextView;
import android.widget.Toast;

import java.io.File;

import cn.jiajunhui.lib.jjhplayer.IJJHPlayer;
import cn.jiajunhui.lib.jjhplayer.JJHPlayer;

public class MainActivity extends AppCompatActivity implements IJJHPlayer.OnPrepareListener, IJJHPlayer.OnErrorEventListener, IJJHPlayer.OnPlayEventListener, SurfaceHolder.Callback, SeekBar.OnSeekBarChangeListener {

    private JJHPlayer mPlayer;
    private SurfaceView mSurfaceView;
    private TextView mStateTv;
    private TextView mTimeTv;
    private SeekBar mSeekBar;

    private boolean isTracking;

    private final int MSG_UPDATE_TIME = 1;

    private Handler mHandler = new Handler(Looper.getMainLooper()){
        @Override
        public void handleMessage(@NonNull Message msg) {
            super.handleMessage(msg);
            switch (msg.what){
                case MSG_UPDATE_TIME:
                    int currentPosition = mPlayer.getCurrentPosition();
                    int duration = mPlayer.getDuration();
                    mTimeTv.setText(String.format("%s/%s",
                            TimeU.getTimeFormat1(currentPosition),
                            TimeU.getTimeFormat1(duration)));
                    mSeekBar.setMax(duration);
                    mSeekBar.setProgress(currentPosition);
                    delayLoopGetTime();
                    break;
            }
        }
    };

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        mSurfaceView = findViewById(R.id.surfaceView);
        mStateTv = findViewById(R.id.stateTv);
        mTimeTv = findViewById(R.id.timeTv);
        mSeekBar = findViewById(R.id.seekBar);
        mSurfaceView.getHolder().addCallback(this);

        mSeekBar.setOnSeekBarChangeListener(this);

        mPlayer = new JJHPlayer(getApplicationContext());
        mPlayer.setDataSource(new File(Environment.getExternalStorageDirectory() + File.separator + "demo.mp4").getAbsolutePath());
//        mPlayer.setDataSource("http://192.168.1.2:8080/demo.mp4");

        mPlayer.setOnPrepareListener(this);
        mPlayer.setOnErrorEventListener(this);
        mPlayer.setOnPlayEventListener(this);

        mPlayer.prepare();

    }

    private void delayLoopGetTime(){
        mHandler.removeMessages(MSG_UPDATE_TIME);
        mHandler.sendEmptyMessageDelayed(MSG_UPDATE_TIME, 1000);
    }

    @Override
    protected void onPause() {
        super.onPause();
        mPlayer.pause();
    }

    @Override
    protected void onStop() {
        super.onStop();
        mPlayer.stop();
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        mPlayer.release();
    }

    @Override
    public void onPrepared(IJJHPlayer player) {
        mStateTv.setText("prepared");
        delayLoopGetTime();
        mPlayer.start();
    }

    @Override
    public void onError(int eventCode, int extra, String message) {
        mStateTv.setText("" + message);
    }

    @Override
    public void onPlayEvent(int eventCode, int extra, String message) {

    }

    @Override
    public void surfaceCreated(@NonNull SurfaceHolder holder) {
        mPlayer.setSurface(holder.getSurface());
    }

    @Override
    public void surfaceChanged(@NonNull SurfaceHolder holder, int format, int width, int height) {

    }

    @Override
    public void surfaceDestroyed(@NonNull SurfaceHolder holder) {

    }

    @Override
    public void onProgressChanged(SeekBar seekBar, int progress, boolean fromUser) {
        if(isTracking){
            mTimeTv.setText(String.format("%s/%s",
                    TimeU.getTimeFormat1(progress),
                    TimeU.getTimeFormat1(seekBar.getMax())));
        }
    }

    @Override
    public void onStartTrackingTouch(SeekBar seekBar) {
        isTracking = true;
    }

    @Override
    public void onStopTrackingTouch(SeekBar seekBar) {
        mPlayer.seekTo(seekBar.getProgress());
        isTracking = false;
    }
}