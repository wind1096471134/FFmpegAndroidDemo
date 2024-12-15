
## Android ffmpeg视频编解码代码例子。

### Example
1. 一张图片(.png)编码成视频文件(.mp4) ： MainActivity.encodeImgToVideo
2. 一张图片(.png)+一个音频文件(.aac)编码成视频文件(.mp4) ：MainActivity.encodeImgAndAudioToVideo
3. 一个视频文件(.mp4)编码成视频文件(.mp4) ： MainActivity.encodeVideoToVideo
4. 播放本地视频（.mp4) : MainActivity.playLocalVideo -> PlayerActivity

### 关键组件
1. VideoDecoder : 解码器，支持解码图片，音频，视频等。
2. VideoEncoder : 编码器，输入视频帧和音频帧，编码成mp4等。
3. Decoder和Encoder由各自工作线程进行编解码，采用简易的生产者-消费者模型。
4. MediaController : 媒体控制器，负责组合VideoDecoder和VideoEncoder进行视频编码。
5. MediaPlayer ： 简易媒体播放器，decoder解码 + SurfaceView + AudioTrack进行音视频播放。
6. MediaAVSync : 音画同步策略-视频跟随音频，且各自需要结合实际渲染/播放的耗时进行调整对齐。
7. VideoSurfaceSink : 渲染视频帧到Surface上。
8. AudioTrackSink : 播放音频帧到AudioTrack上。