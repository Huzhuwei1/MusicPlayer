package com.hzw.mylibrary;

import android.text.TextUtils;
import android.util.Log;

/**
 * Created by huzw on 2017/9/17.
 */

public class Player {

    static {
        System.loadLibrary("native-lib");
    }

    private final String TAG = "Player";
    private String url;

    private MyPlayerListener mListener;

    public void setListener(MyPlayerListener listener) {
        mListener = listener;
    }

    /**
     * 设置播放源
     * @param url
     */
    public void setUrl(String url) {
        this.url = url;
    }

    public void prepare() {

        if (TextUtils.isEmpty(url)) {
            Log.e(TAG, "url cannot be empty");
            return;
        }
        
        nativePrepare(url);
    }

    public void start() {
        if (TextUtils.isEmpty(url)) {
            Log.e(TAG, "url cannot be empty");
            return;
        }

        nativeStart();
    }

    public void pause() {
        nativePause();
    }

    public void resume() {
        nativeResume();
    }

    public void stop() {
        nativeStop();
    }

    public void seek(int seconds) {
        nativeSeek(seconds);
    }

    public void setVolume(int percent) {
        nativeSetVolume(percent);
    }

    public void setSoundChannel(SoundChannel channel){
        nativeSetSoundChannel(channel.getId());
    }

    public void setTempo(float tempo) {
        nativeSetTempo(tempo);
    }

    public void setPitch(float pitch) {
        nativeSetPitch(pitch);
    }

    public void onPrepared() {
        if (mListener != null) {
            mListener.onPrepared();
        }
    }

    public void onPlaying(int currentTime, int totalTime) {
        if (mListener != null) {
            mListener.onPlaying(currentTime, totalTime);
        }
    }

    public void onError() {
        if (mListener != null) {
            stop();
            mListener.onError();
        }
    }

    public void onComplete() {
        stop();
        if (mListener != null) {
            mListener.onComplete();
        }
    }

    public interface MyPlayerListener {
        void onPrepared();
        void onPlaying(int currentTime, int totalTime);
        void onError();
        void onComplete();
    }


    private native void nativePrepare(String url);

    private native void nativeStart();

    private native void nativePause();

    private native void nativeResume();

    private native void nativeStop();

    private native void nativeSeek(int seconds);

    private native void nativeSetVolume(int percent);

    private native void nativeSetSoundChannel(int channel);

    private native void nativeSetTempo(float percent);

    private native void nativeSetPitch(float percent);

    public enum SoundChannel{
        CHANNEL_LEFT(0),CHANNEL_RIGHT(1),CHANNEL_STEREO(2);
        private int id;
        SoundChannel(int id) {
            this.id = id;
        }

        public int getId() {
            return id;
        }
    }


}
