package com.example.ffmpegdemo

import android.graphics.Bitmap
import android.graphics.BitmapFactory
import android.graphics.Color
import androidx.appcompat.app.AppCompatActivity
import android.os.Bundle
import android.view.Gravity
import android.view.View
import android.view.ViewGroup
import android.widget.FrameLayout
import android.widget.TextView
import android.widget.Toast
import androidx.recyclerview.widget.LinearLayoutManager
import androidx.recyclerview.widget.RecyclerView.Adapter
import androidx.recyclerview.widget.RecyclerView.ViewHolder
import com.example.ffmpegdemo.databinding.ActivityMainBinding
import java.io.File
import java.io.FileOutputStream
import java.util.concurrent.ExecutorService
import java.util.concurrent.Executors

class MainActivity : AppCompatActivity() {

    private lateinit var binding: ActivityMainBinding
    private lateinit var threadPoolExecutor: ExecutorService
    private val actionList = arrayOf(
        1 to "图片->视频",
        2 to "视频+音频->视频"
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
                }
            }
        }
    }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        initView()

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
    }

    private fun encodeImgToVideo() {
        binding.loading.visibility = View.VISIBLE
        threadPoolExecutor.execute {
            val inputFile = baseContext.filesDir.absolutePath + "/logo.png"
            val file = File(inputFile)
            if(!file.exists()) {
                val testImg = BitmapFactory.decodeResource(resources, R.drawable.logo)
                val outputStream = FileOutputStream(file)
                testImg.compress(Bitmap.CompressFormat.PNG, 80, outputStream)
                outputStream.flush()
                outputStream.close()
            }
            val outputFile = baseContext.filesDir.absolutePath + "/imgToVideo.mp4"
            val ret = ffmpegEncodeImgToVideo(inputFile, outputFile)
            resultInUiThread(if(ret) "编码成功，文件保存到：$outputFile" else "操作失败")
        }
    }

    private fun resultInUiThread(msg: String) {
        binding.actionList.post {
            Toast.makeText(baseContext, msg, Toast.LENGTH_SHORT).show()
            binding.loading.visibility = View.GONE
        }
    }

    external fun ffmpegEncodeImgToVideo(inputPath: String, outputPath: String): Boolean

    companion object {
        // Used to load the 'ffmpegdemo' library on application startup.
        init {
            System.loadLibrary("ffmpegdemo")
            System.loadLibrary("ffmpeg")
        }
    }
}