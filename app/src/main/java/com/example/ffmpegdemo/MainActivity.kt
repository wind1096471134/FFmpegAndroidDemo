package com.example.ffmpegdemo

import android.graphics.Bitmap
import android.graphics.BitmapFactory
import androidx.appcompat.app.AppCompatActivity
import android.os.Bundle
import android.widget.Toast
import com.example.ffmpegdemo.databinding.ActivityMainBinding
import java.io.File
import java.io.FileOutputStream
import java.util.concurrent.ExecutorService
import java.util.concurrent.Executors
import java.util.concurrent.FutureTask

class MainActivity : AppCompatActivity() {

    private lateinit var binding: ActivityMainBinding
    private lateinit var threadPoolExecutor: ExecutorService

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)

        binding = ActivityMainBinding.inflate(layoutInflater)
        setContentView(binding.root)

        threadPoolExecutor = Executors.newFixedThreadPool(4)

        binding.button.setOnClickListener {
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
                val outputFile = baseContext.filesDir.absolutePath + "/logo.mp4"
                val ret = ffmpegEncoderTest(inputFile, outputFile)
                binding.button.post {
                    Toast.makeText(baseContext, if(ret) "success" else "fail", Toast.LENGTH_SHORT).show()
                }
            }
        }
    }

    override fun onDestroy() {
        super.onDestroy()
        threadPoolExecutor.shutdown()
    }

    external fun ffmpegEncoderTest(inputPath: String, outputPath: String): Boolean

    companion object {
        // Used to load the 'ffmpegdemo' library on application startup.
        init {
            System.loadLibrary("ffmpegdemo")
            System.loadLibrary("ffmpeg")
        }
    }
}