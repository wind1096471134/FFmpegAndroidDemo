package com.example.ffmpegdemo

import android.content.Intent
import android.graphics.Bitmap
import android.graphics.BitmapFactory
import android.graphics.Color
import android.os.Bundle
import android.view.Gravity
import android.view.View
import android.view.ViewGroup
import android.widget.FrameLayout
import android.widget.TextView
import android.widget.Toast
import androidx.appcompat.app.AppCompatActivity
import androidx.recyclerview.widget.LinearLayoutManager
import androidx.recyclerview.widget.RecyclerView.Adapter
import androidx.recyclerview.widget.RecyclerView.ViewHolder
import com.example.ffmpegdemo.databinding.ActivityMainBinding
import java.io.File
import java.io.FileOutputStream
import java.io.IOException
import java.util.concurrent.ExecutorService
import java.util.concurrent.Executors


class MainActivity : AppCompatActivity() {

    private lateinit var binding: ActivityMainBinding
    private lateinit var threadPoolExecutor: ExecutorService
    private val actionList = arrayOf(
        1 to "图片 -> 视频",
        2 to "图片+音频 -> 视频",
        3 to "视频 -> 视频",
        4 to "播放本地视频"
    )

    class CustomViewHolder(
        root: View,
        val titleView: TextView
    ) : ViewHolder(root)

    private val actionAdapter = object : Adapter<CustomViewHolder>() {
        override fun onCreateViewHolder(parent: ViewGroup, viewType: Int): CustomViewHolder {
            val textView = TextView(parent.context).apply {
                textSize = 16f
                gravity = Gravity.CENTER
                layoutParams = ViewGroup.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.WRAP_CONTENT)
                val padding = (resources.displayMetrics.density * 20).toInt()
                setPadding(padding, padding, padding, padding)
                setBackgroundResource(R.color.item_bg)
            }
            val root = FrameLayout(parent.context).apply {
                setPadding(0,0,0, (resources.displayMetrics.density * 1).toInt())
                setBackgroundColor(Color.GRAY)
                layoutParams = ViewGroup.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.WRAP_CONTENT)
            }
            root.addView(textView)
            return CustomViewHolder(root, textView)
        }

        override fun getItemCount(): Int {
            return actionList.size
        }

        override fun onBindViewHolder(holder: CustomViewHolder, position: Int) {
            val data = actionList[position]
            holder.titleView.text = data.second
            holder.itemView.setOnClickListener {
                when(data.first) {
                    1 -> {
                        encodeImgToVideo()
                    }
                    2 -> {
                        encodeImgAndAudioToVideo()
                    }
                    3 -> {
                        encodeVideoToVideo()
                    }
                    4 -> {
                        playLocalVideo()
                    }
                }
            }
        }
    }
    private val nativeMediaCallback = object : NativeMediaCallback {
        override fun onEncodeFinish(ret: Int, filePath: String?) {
            resultInUiThread(if(ret == 0) "编码成功，文件保存到：$filePath" else "操作失败")
        }
    }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        initView()
        ffmpegSetNativeCallback(nativeMediaCallback)
        threadPoolExecutor = Executors.newFixedThreadPool(4)
    }

    private fun initView() {
        binding = ActivityMainBinding.inflate(layoutInflater)
        setContentView(binding.root)
        binding.actionList.adapter = actionAdapter
        binding.actionList.layoutManager = LinearLayoutManager(baseContext)
    }

    override fun onDestroy() {
        super.onDestroy()
        threadPoolExecutor.shutdown()
        ffmpegEncodeDestroy()
    }

    private fun encodeImgToVideo() {
        binding.loading.visibility = View.VISIBLE
        val imgFile = baseContext.filesDir.absolutePath + "/img.png"
        appendLogText("输入文件：\n$imgFile")
        threadPoolExecutor.execute {
            saveAssetFileToLocalIfNeed("img.PNG", imgFile)
            val outputFile = baseContext.filesDir.absolutePath + "/imgToVideo.mp4"
            val ret = ffmpegEncodeImgToVideo(imgFile, outputFile)
        }
    }

    private fun encodeImgAndAudioToVideo() {
        binding.loading.visibility = View.VISIBLE
        val imgFile = baseContext.filesDir.absolutePath + "/img.png"
        val audioFile = baseContext.filesDir.absolutePath + "/audio.aac"
        appendLogText("输入文件：\n$imgFile\n$audioFile")
        threadPoolExecutor.execute {
            saveAssetFileToLocalIfNeed("img.PNG", imgFile)
            saveAssetFileToLocalIfNeed("audio.aac", audioFile)
            val outputFile = baseContext.filesDir.absolutePath + "/imgAndAudioToVideo.mp4"
            val ret = ffmpegEncodeImgAndAudioToVideo(imgFile, audioFile, outputFile)
        }
    }

    private fun encodeVideoToVideo() {
        binding.loading.visibility = View.VISIBLE
        val videoFile = baseContext.filesDir.absolutePath + "/video.mp4"
        appendLogText("输入文件：\n$videoFile")
        threadPoolExecutor.execute {
            saveAssetFileToLocalIfNeed("video.mp4", videoFile)
            val outputFile = baseContext.filesDir.absolutePath + "/videoToVideo.mp4"
            val ret = ffmpegEncodeVideoToVideo(videoFile, outputFile)
        }
    }

    private fun playLocalVideo() {
        binding.loading.visibility = View.VISIBLE
        threadPoolExecutor.execute {
            val videoFile = baseContext.filesDir.absolutePath + "/video.mp4"
            saveAssetFileToLocalIfNeed("video.mp4", videoFile)
            binding.loading.post {
                binding.loading.visibility = View.GONE
                val intent = Intent(baseContext, PlayerActivity::class.java).apply {
                    putExtra(PlayerActivity.FILE_URL, videoFile)
                }
                startActivity(intent)
            }
        }
    }

    private fun saveAssetFileToLocalIfNeed(name: String, localFile: String): Boolean {
        try {
            val inputStream = assets.open(name)
            val file = File(localFile)
            if(!file.exists()) {
                val outputStream = FileOutputStream(file)
                val buffer = ByteArray(1024)
                var length: Int
                while ((inputStream.read(buffer).also { length = it }) > 0) {
                    outputStream.write(buffer, 0, length)
                }
                outputStream.flush()
                outputStream.close()
                inputStream.close()
            }
            return true
        } catch (e : IOException) {
            e.printStackTrace()
        }
        return false
    }

    private fun resultInUiThread(msg: String) {
        binding.actionList.post {
            appendLogText(msg, false)
            binding.loading.visibility = View.GONE
        }
    }

    private fun appendLogText(msg: String, clear: Boolean = true) {
        val showMsg = if(clear) msg else "${binding.msgText.text}\n$msg"
        binding.msgText.text = showMsg
    }

    external fun ffmpegSetNativeCallback(callback: NativeMediaCallback)

    external fun ffmpegEncodeImgToVideo(imgInputPath: String, outputPath: String): Boolean

    external fun ffmpegEncodeImgAndAudioToVideo(imgInputPath: String, audioInputPath: String?, outputPath: String): Boolean

    external fun ffmpegEncodeVideoToVideo(videoInputPath: String, outputPath: String): Boolean

    external fun ffmpegEncodeDestroy();

    companion object {
        // Used to load the 'ffmpegdemo' library on application startup.
        init {
            System.loadLibrary("ffmpegdemo")
            System.loadLibrary("ffmpeg")
        }
    }

    interface NativeMediaCallback {
        fun onEncodeFinish(ret: Int, filePath: String?)
    }
}