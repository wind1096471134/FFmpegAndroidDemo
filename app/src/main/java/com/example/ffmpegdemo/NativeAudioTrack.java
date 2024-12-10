package com.example.ffmpegdemo;

import android.media.AudioFormat;
import android.media.AudioManager;
import android.media.AudioTrack;
import android.util.Log;

public class NativeAudioTrack {
    private volatile AudioTrack audioTrack = null;

    public void playStart(int sampleRate, int channelConfig, int audioFormat) {
        synchronized (this) {
            if (audioTrack != null) {
                audioTrack.release();
            }
            int channelConfigForAT;
            switch (channelConfig) {
                case 1:
                    channelConfigForAT = AudioFormat.CHANNEL_IN_MONO;
                    break;
                case 2:
                    channelConfigForAT = AudioFormat.CHANNEL_IN_STEREO;
                    break;
                default:
                    channelConfigForAT = AudioFormat.CHANNEL_IN_DEFAULT;
                    break;
            }

            int audioFormatForAT;
            switch (audioFormat) {
                case 1:
                    audioFormatForAT = AudioFormat.ENCODING_PCM_8BIT;
                    break;
                case 2:
                    audioFormatForAT = AudioFormat.ENCODING_PCM_16BIT;
                    break;
                case 3:
                    audioFormatForAT = AudioFormat.ENCODING_PCM_FLOAT;
                    break;
                default:
                    audioFormatForAT = AudioFormat.ENCODING_DEFAULT;
                    break;
            }

            audioTrack = new AudioTrack(AudioManager.STREAM_MUSIC, sampleRate, channelConfigForAT, audioFormatForAT,
                    AudioTrack.getMinBufferSize(sampleRate, channelConfigForAT, audioFormatForAT), AudioTrack.MODE_STREAM);
            audioTrack.play();
        }
    }

    public void playFrame(byte[] data, int size) {
        synchronized (this) {
            if (audioTrack != null) {
                int ret = audioTrack.write(data, 0, size);
                //Log.i("AudioTrack", "write " + ret);
            }
        }
    }

    public void playEnd() {
        synchronized (this) {
            if(audioTrack != null) {
                audioTrack.release();
                audioTrack = null;
            }
        }
    }
}