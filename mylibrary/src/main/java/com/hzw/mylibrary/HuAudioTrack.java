package com.hzw.mylibrary;

import android.media.AudioFormat;
import android.media.AudioManager;
import android.media.AudioTrack;
import android.util.Log;

import java.util.concurrent.ArrayBlockingQueue;
import java.util.concurrent.TimeUnit;

/**
 * Created by huzw on 2017/11/13.
 */

public class HuAudioTrack implements Runnable{
    private int samplerate = 44100;// 设置音频数据的采样率
    private int channel;
    private int bitRate;
    private int minBuffSize;
    private AudioTrack audioTrack;
    private int PLAYSTATE = -1;
    private AudioData cacheData; //不过一个audio min size 的数据缓存
    private ArrayBlockingQueue<AudioData> queue = new ArrayBlockingQueue<>(60);

    public HuAudioTrack(){
        this(44100, AudioFormat.CHANNEL_OUT_STEREO, AudioFormat.ENCODING_PCM_16BIT);
    }

    public HuAudioTrack(int sample,int channel,int bitRate){
        this.samplerate = sample;
        this.channel = channel;// 设置输出声道为双声道立体声 //AudioFormat.CHANNEL_CONFIGURATION_STEREO
        this.bitRate = bitRate;          // 设置音频数据块是8位还是16位
        // 声音文件一秒钟buffer的大小
        minBuffSize = AudioTrack.getMinBufferSize(samplerate, this.channel, this.bitRate);

	    audioTrack = new AudioTrack(AudioManager.STREAM_MUSIC,
                // 指定在流的类型  STREAM_MUSIC
	            // STREAM_ALARM：警告声
	            // STREAM_MUSCI：音乐声，例如music等
	            // STREAM_RING：铃声
	            // STREAM_SYSTEM：系统声音
	            // STREAM_VOCIE_CALL：电话声音
	    		samplerate,
	    		this.channel,
	            this.bitRate,
                minBuffSize, AudioTrack.MODE_STREAM);// 设置模式类型，在这里设置为流类型

        Log.i("audio","minBuffSize="+minBuffSize);
    }



    public synchronized void play(){
        if(audioTrack != null){
            audioTrack.play();
            PLAYSTATE = AudioTrack.PLAYSTATE_PLAYING;
            new Thread(this).start();
        }
    }

    @Override
    public void run() {
        while(audioTrack != null && PLAYSTATE == AudioTrack.PLAYSTATE_PLAYING){
            synchronized (HuAudioTrack.this) {
                if(audioTrack != null && audioTrack.getPlayState() == AudioTrack.PLAYSTATE_PLAYING){
                    try {
                        AudioData audioData = queue.poll(5, TimeUnit.MILLISECONDS);
                        if(audioData != null && audioData.data != null){
                            long time = System.currentTimeMillis();
                            Log.d("audio_time", "audioData.length = " + audioData.data.length);
                            audioTrack.write(audioData.data,0,audioData.data.length);
                        }else{
                            Log.i("audio", "queue is empty");
                        }
                    } catch (InterruptedException e) {
                        e.printStackTrace();
                    }
                }else{
                    Log.i("audio", "play state is not playing");
                }
            }
        }
    }

    public void write(byte[] data,final int dataLen){
        if(audioTrack != null && PLAYSTATE == AudioTrack.PLAYSTATE_PLAYING){
            AudioData audioData = new AudioData();
            audioData.data = new byte[dataLen];
            System.arraycopy(data, 0, audioData.data, 0, dataLen);
            audioData.len = dataLen;
            try {
                Log.i("audio_time", "put size = " + queue.size()+"; dataLen= " + dataLen);
                queue.put(audioData);
            } catch (InterruptedException e) {
                e.printStackTrace();
            }
        }
    }

    public void pause(){
        queue.clear();
        PLAYSTATE = AudioTrack.PLAYSTATE_PAUSED;
        if(audioTrack != null){
            synchronized (this) {
                audioTrack.pause();
            }
        }
    }

    public void stop(){
        queue.clear();
        PLAYSTATE = AudioTrack.PLAYSTATE_STOPPED;
        if(audioTrack != null){
            synchronized (this) {
                audioTrack.stop();
            }
        }
    }

    public void release(){
        queue.clear();
        if(audioTrack != null){
            synchronized (this) {
                audioTrack.release();
                PLAYSTATE = -1;
                audioTrack = null;
            }
        }
    }

    public int getMinBufferSize(){
        return minBuffSize;
    }

    public int getSamplerate(){
        return samplerate;
    }

    class AudioData{
        public byte[] data;
        public int len;
    }
}
