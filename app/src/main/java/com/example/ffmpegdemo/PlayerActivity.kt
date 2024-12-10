package com.example.ffmpegdemo

import android.os.Bundle
import android.text.TextUtils
import android.util.Log
import android.view.Surface
import android.view.SurfaceHolder
import androidx.appcompat.app.AppCompatActivity
import com.example.ffmpegdemo.databinding.ActivityPlayerBinding

class PlayerActivity : AppCompatActivity() {
    companion object {
        const val TAG = "PlayerActivity"
        const val FILE_URL = "file_url"

        init {
            System.loadLibrary("ffmpegdemo")
            System.loadLibrary("ffmpeg")
        }
    }
    private lateinit var binding: ActivityPlayerBinding
    private val nativeAudioTrack = NativeAudioTrack()

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        binding = ActivityPlayerBinding.inflate(layoutInflater)
        setContentView(binding.root)
        val fileUrl = intent.getStringExtra(FILE_URL)
        if(TextUtils.isEmpty(fileUrl)) {
            finish()
        }

        binding.playerSurface.holder.addCallback(object : SurfaceHolder.Callback {
            override fun surfaceCreated(holder: SurfaceHolder) {
                Log.i(TAG, "surfaceCreated")
                //play video
                ffmpegPlayVideo(fileUrl!!, holder.surface, nativeAudioTrack)
            }

            override fun surfaceChanged(
                holder: SurfaceHolder,
                format: Int,
                width: Int,
                height: Int
            ) {
                Log.i(TAG, "surfaceChanged:$width $height")
            }

            override fun surfaceDestroyed(holder: SurfaceHolder) {
                Log.i(TAG, "surfaceDestroyed")
                ffmpegPlayRelease()
                nativeAudioTrack.playEnd()
            }
        })
    }

    override fun onDestroy() {
        super.onDestroy()
        ffmpegPlayRelease()
    }

    external fun ffmpegPlayVideo(fileUrl: String, surface: Surface, audioTrack: NativeAudioTrack)

    external fun ffmpegPlayRelease()
}