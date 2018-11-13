package com.hzw.mylibrary;

import android.media.AudioFormat;
import android.media.AudioTrack;
import android.media.MediaCodec;
import android.media.MediaCodecInfo;
import android.media.MediaFormat;
import android.os.AsyncTask;
import android.text.TextUtils;
import android.util.Log;

import java.io.BufferedInputStream;
import java.io.DataInputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.nio.ByteBuffer;

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
    private boolean initmediacodec;

    private File mAudioFile = null;
    volatile private boolean mIsPlaying;
    private int mSampleRate = 44100;
    private int mChannelConfig = AudioFormat.CHANNEL_OUT_STEREO;
    private int mSampleFormat = AudioFormat.ENCODING_PCM_16BIT;

    private PlayTask mPlayer;
    private HuAudioTrack mHuAudioTrack;

    public void PlayPcm(String url){
        mAudioFile = new File(url);
        if (!mAudioFile.exists())return;

        mPlayer = new PlayTask();
        mPlayer.execute();

    }

    public void stopPlayPcm() {
        mIsPlaying = false;
    }

    class PlayTask extends AsyncTask<Void,Void,Void> {
        @Override
        protected Void doInBackground(Void... arg0) {
            mIsPlaying = true;
            int bufferSize = AudioTrack.getMinBufferSize(mSampleRate, mChannelConfig, mSampleFormat);
            byte[] buffer = new byte[bufferSize];
            mHuAudioTrack = new HuAudioTrack(mSampleRate,mChannelConfig,mSampleFormat);
            try {
                // 定义输入流，将音频写入到AudioTrack类中，实现播放
                DataInputStream dis = new DataInputStream(
                        new BufferedInputStream(new FileInputStream(mAudioFile)));
                // 开始播放
                mHuAudioTrack.play();
                // 由于AudioTrack播放的是流，所以，我们需要一边播放一边读取
                while (mIsPlaying && dis.available() > 0) {
                    int i = 0;
                    while (dis.available() > 0 && i < buffer.length) {
                        buffer[i] = dis.readByte();
                        i++;
                    }
                    // 然后将数据写入到AudioTrack中
                    mHuAudioTrack.write(buffer,buffer.length);
                }

                // 播放结束
                mHuAudioTrack.stop();
                mHuAudioTrack.release();
                dis.close();
            } catch (Exception e) {
                // TODO: handle exception
                Log.e(TAG,"error:" + e.getMessage());
            }
            return null;
        }
        @Override protected void onPostExecute(Void result) {}
        @Override protected void onPreExecute() {}
    }

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


    public void startRecord(File outfile) {
        if(!initmediacodec) {
            if(nativeGetSamplerate() > 0) {
                initmediacodec = true;
                initMediaCodec(nativeGetSamplerate(), outfile);
                n_startstoprecord(true);
            }
        }
    }

    public void stopRecord() {
        if(initmediacodec) {
            n_startstoprecord(false);
            releaseMediaCodec();
        }
    }

    public void pauseRecord() {
        n_startstoprecord(false);
    }

    public void resumeRcord() {
        n_startstoprecord(true);
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

    private native int nativeGetSamplerate();

    public native void nativeCreateRecorderEngine();

    public native void nativeCreateAudioRecorder(String url);

    public native void nativeStopRecord();

    public native void nativeReleaseRecorder();

    private native void n_startstoprecord(boolean isStart);

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

    private MediaFormat encoderFormat;
    private MediaCodec encoder;
    private FileOutputStream fos;
    private MediaCodec.BufferInfo info;
    private int perpcmsize;
    private byte[] outByteBuffer;
    private int aacsamplerate = 4;

    private void initMediaCodec(int samperate, File outfile) {
        try {
            aacsamplerate = getADTSsamplerate(samperate);
            encoderFormat = MediaFormat.createAudioFormat(MediaFormat.MIMETYPE_AUDIO_AAC,samperate,2);
            encoderFormat.setInteger(MediaFormat.KEY_BIT_RATE,96000);
            encoderFormat.setInteger(MediaFormat.KEY_AAC_PROFILE, MediaCodecInfo.CodecProfileLevel.AACObjectLC);
            encoderFormat.setInteger(MediaFormat.KEY_MAX_INPUT_SIZE, 100 * 1024);
            encoder = MediaCodec.createEncoderByType(MediaFormat.MIMETYPE_AUDIO_AAC);
            info = new MediaCodec.BufferInfo();
            if (encoder == null) {
                return;
            }
            encoder.configure(encoderFormat,null,null,MediaCodec.CONFIGURE_FLAG_ENCODE);
            fos = new FileOutputStream(outfile);
            encoder.start();
        }catch (IOException e) {
            e.printStackTrace();
        }
    }

    private void encodecPcmToAAC(int size, byte[] buffer) {
        if (buffer != null && encoder !=null) {
            int inputBufferIndex = encoder.dequeueInputBuffer(0);
            if (inputBufferIndex >= 0) {
                ByteBuffer byteBuffer = encoder.getInputBuffers()[inputBufferIndex];
                Log.e("byteBuffer","byteBuffer:" +byteBuffer.capacity()+",buffer:" +buffer.length);
                byteBuffer.clear();
                byteBuffer.put(buffer);
                encoder.queueInputBuffer(inputBufferIndex,0,size,0,0);
            }
            int index = encoder.dequeueOutputBuffer(info,0);
            while (index >= 0) {
                try {
                    perpcmsize = info.size + 7;
                    outByteBuffer = new byte[perpcmsize];

                    ByteBuffer byteBuffer = encoder.getOutputBuffers()[index];
                    byteBuffer.position(info.offset);
                    byteBuffer.limit(info.offset + info.size);

                    addADTSHeader(outByteBuffer,perpcmsize,aacsamplerate);

                    byteBuffer.get(outByteBuffer,7,info.size);
                    byteBuffer.position(info.offset);

                    fos.write(outByteBuffer,0,perpcmsize);

                    encoder.releaseOutputBuffer(index,false);
                    index = encoder.dequeueOutputBuffer(info,0);
                    outByteBuffer = null;
                    Log.d("Player","编码中。。。");
                } catch (IOException e) {
                    e.printStackTrace();
                }
            }
        }
    }

    private void addADTSHeader(byte[] packet, int packetLen,int samplerate){
        int profile = 2;//AAC LC
        int freqIdx = samplerate; //samplerate
        int chanCfg = 2; //CPE
        packet[0] = (byte) 0xFF; // 0xFFF(12bit) 这里只取了8位，所以还差4位放到下一个里面
        packet[1] = (byte) 0xF9; // 第一个t位放F
        packet[2] = (byte) (((profile - 1) << 6) + (freqIdx << 2) + (chanCfg >> 2));
        packet[3] = (byte) (((chanCfg & 3) << 6) + (packetLen >> 11));
        packet[4] = (byte) ((packetLen & 0x7FF) >> 3);
        packet[5] = (byte) (((packetLen & 7) << 5) + 0x1F);
        packet[6] = (byte) 0xFC;
    }

    private void releaseMediaCodec() {
        if (encoder == null) {return;}
        try {
            fos.close();
            encoder.stop();
            encoder.release();
            encoderFormat = null;
            info = null;
            initmediacodec = false;
        } catch (IOException e) {
            e.printStackTrace();
        }finally {
            if (fos != null) {
                try {
                    fos.close();
                } catch (IOException e) {
                    e.printStackTrace();
                }
            }
        }
    }

    private int getADTSsamplerate(int samplerate) {
        int rate = 4;
        switch (samplerate) {
            case 96000:
                rate = 0;
                break;
            case 88200:
                rate = 1;
                break;
            case 64000:
                rate = 2;
                break;
            case 48000:
                rate = 3;
                break;
            case 44100:
                rate = 4;
                break;
            case 32000:
                rate = 5;
                break;
            case 24000:
                rate = 6;
                break;
            case 22050:
                rate = 7;
                break;
            case 16000:
                rate = 8;
                break;
            case 12000:
                rate = 9;
                break;
            case 11025:
                rate = 10;
                break;
            case 8000:
                rate = 11;
                break;
            case 7350:
                rate = 12;
                break;
            default:break;
        }
        return rate;
    }
}
