package com.hzw.humusic;

import android.os.Bundle;
import android.os.Handler;
import android.support.v7.app.AppCompatActivity;
import android.util.Log;
import android.view.View;
import android.widget.SeekBar;
import android.widget.TextView;

import com.hzw.mylibrary.Player;

import java.io.File;
import java.util.concurrent.LinkedBlockingQueue;

public class MainActivity extends AppCompatActivity {

    Player player;
    private SeekBar mVolumeSeekBar;
    private SeekBar mTempoSeekBar;
    private SeekBar mPitchSeekBar;
    private TextView mTvTempo;
    private TextView mTvPitch;
    private float mPitchV;
    private float mTempoV;

    // Used to load the 'native-lib' library on application startup.
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        mTvPitch = findViewById(R.id.tv_pitch);
        mTvTempo = findViewById(R.id.tv_tempo);
        player = new Player();
        player.nativeCreateRecorderEngine();
        player.setListener(new Player.MyPlayerListener() {
            @Override
            public void onPrepared() {
                Log.e("MainActivity","准备好了");
                player.start();
            }

            @Override
            public void onPlaying(int currentTime, int totalTime) {
                Log.e("MainActivity","currentTime:" +currentTime+",totalTime:"+totalTime);
            }

            @Override
            public void onError() {
                Log.e("MainActivity","出错了");
            }

            @Override
            public void onComplete() {
                Log.e("MainActivity","播放完了了");
            }
        });
        mVolumeSeekBar = findViewById(R.id.volume);
        mVolumeSeekBar.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener() {
            @Override
            public void onProgressChanged(SeekBar seekBar, int progress, boolean fromUser) {
                player.setVolume(progress);
            }

            @Override
            public void onStartTrackingTouch(SeekBar seekBar) {

            }

            @Override
            public void onStopTrackingTouch(SeekBar seekBar) {

            }
        });

        mTempoSeekBar = findViewById(R.id.tempo);
        mTempoSeekBar.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener() {
            @Override
            public void onProgressChanged(SeekBar seekBar, int progress, boolean fromUser) {
                mTempoV = progress / 100f + 0.50f;
                int v = (int) (mTempoV *100);
                mTvTempo.setText("音速:X"+ v/100f);
            }

            @Override
            public void onStartTrackingTouch(SeekBar seekBar) {
            }

            @Override
            public void onStopTrackingTouch(SeekBar seekBar) {
                player.setTempo(mTempoV);
            }
        });
        mPitchSeekBar = findViewById(R.id.pitch);
        mPitchSeekBar.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener() {
            @Override
            public void onProgressChanged(SeekBar seekBar, int progress, boolean fromUser) {
                mPitchV = progress / 100f + 0.5f;
                int v = (int) (mPitchV * 100);
                mTvPitch.setText("音调:X"+ v/100f);
            }

            @Override
            public void onStartTrackingTouch(SeekBar seekBar) {
            }

            @Override
            public void onStopTrackingTouch(SeekBar seekBar) {
                player.setPitch(mPitchV);
            }
        });
//        for (int i = 0; i < 100; i++) {
//            mQueue.offer(i+"");
//        }
//        new Thread(){
//            @Override
//            public void run() {
//                while (flag) {
//                    try {
//                        String take = mQueue.poll(5, TimeUnit.SECONDS);
//
//                        Log.e("MainActivity",take+"");
//                    } catch (InterruptedException e) {
//                        e.printStackTrace();
//                    }
//                }
//                Log.e("MainActivity","Thread exit");
//            }
//        }.start();
//
//        new Thread(){
//            @Override
//            public void run() {
//                while (true){
//                    if(mQueue.size() == 0){
//                        try {
//                            sleep(2000);
//                        } catch (InterruptedException e) {
//                            e.printStackTrace();
//                        }
//                        flag = false;
//                    }
//                }
//            }
//        }.start();
    }

    volatile boolean flag = true;
    LinkedBlockingQueue<String> mQueue = new LinkedBlockingQueue<>();
    /**
     * A native method that is implemented by the 'native-lib' native library,
     * which is packaged with this application.
     */
    Handler mHandler = new Handler();


    public void start(View view) {
        player.setUrl("http://mpge.5nd.com/2015/2015-11-26/69708/1.mp3");
        player.setUrl("/storage/emulated/0/99.mp3");
        player.prepare();

    }

    public void pause(View view) {

        player.pause();
    }

    public void resume(View view) {

        player.resume();
    }

    public void stop(View view) {

        player.stop();
    }

    public void seek(View view) {

        player.seek(100);
    }

    public void leftChannel(View view) {

        player.setSoundChannel(Player.SoundChannel.CHANNEL_LEFT);
    }

    public void rightChannel(View view) {

        player.setSoundChannel(Player.SoundChannel.CHANNEL_RIGHT);
    }

    public void stereo(View view) {

        player.setSoundChannel(Player.SoundChannel.CHANNEL_STEREO);
    }



    public void startAudioRecord(View view) {
        player.nativeCreateAudioRecorder("/storage/emulated/0/huAudioRecord.pcm");
    }

    public void stopAudioRecord(View view) {
        player.nativeStopRecord();
    }

    public void playAudioRecord(View view) {
        player.PlayPcm("/storage/emulated/0/humusic.pcm");
    }

    public void stopPlayAudioRecord(View view) {
        player.stopPlayPcm();
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        player.nativeReleaseRecorder();
    }

}
