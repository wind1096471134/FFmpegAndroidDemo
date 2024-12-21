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
    private var playState = 0
    private val playerStateCallback = object : NativePlayerStateCallback {
        override fun onStateChange(state: Int) {
            Log.i("PlayerActivity", "onStateChange $state")
            binding.playState.post {
                val resId = when(state) {
                    1 -> R.drawable.pause
                    2 -> R.drawable.play
                    else -> R.drawable.play
                }
                binding.playState.setImageResource(resId)
                playState = state
            }
        }
    }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        binding = ActivityPlayerBinding.inflate(layoutInflater)
        setContentView(binding.root)
        val fileUrl = intent.getStringExtra(FILE_URL)
        if(TextUtils.isEmpty(fileUrl)) {
            finish()
        }
        binding.playState.setOnClickListener {
            if(playState == 1) {
                ffmpegPlayPause()
            } else if(playState == 2) {
                ffmpegPlayResume()
            }
        }
        binding.playerSurface.holder.addCallback(object : SurfaceHolder.Callback {
            override fun surfaceCreated(holder: SurfaceHolder) {
                Log.i(TAG, "surfaceCreated")
                //play video
                ffmpegPlayVideo(fileUrl!!, holder.surface, nativeAudioTrack, playerStateCallback)
                ffmpegSetLoop(true)
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

    external fun ffmpegPlayVideo(fileUrl: String, surface: Surface, audioTrack: NativeAudioTrack, playerStateCallback: NativePlayerStateCallback)

    external fun ffmpegPlayPause()

    external fun ffmpegPlayResume()

    external fun ffmpegPlayRelease()

    external fun ffmpegSetLoop(loop: Boolean)

    interface NativePlayerStateCallback {
        /**
         * state: 0-init, 1-play, 2-pause, 3-destroy
         */
        fun onStateChange(state: Int)
    }
}