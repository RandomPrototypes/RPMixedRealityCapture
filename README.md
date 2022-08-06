# RPMixedRealityCapture

Unofficial mixed reality capture software for Quest 2 without greenscreen, with support for usb webcams and android phones (soon).  
Still a beta version, please report if you find bugs.  
Based on [libQuestMR](https://github.com/RandomPrototypes/libQuestMR) for communication with the Quest and
[RPCameraInterface](https://github.com/RandomPrototypes/RPCameraInterface) for multi-platform camera interface.

# How to install?

On linux, just run the build script :  
`bash buildWithDeps.sh --cuda`  
Remove --cuda if your computer does not have CUDA 11.2 installed.  

On windows, use the installer or build from source.  
Choose the CUDA version if you have CUDA installed on your computer, otherwise use DirectML version.

On MacOS, build from source (not tested because I don't have any apple device, but all the libs used are compatible with MacOS so it should theorically work).

# How to run?

On windows, run the executable RPMixedRealityCapture

On linux, go to the install folder and run `./RPMixedRealityCapture`

# OpenCV backend

OpenCV camera backend does not support camera name and resolution listing, you need to provide the number of the camera like when calling cv::VideoCapture.  
You can choose the sub-backend by setting capture_backend to "any", "vfw", "v4l", "v4l2", "firewire", "qt", "unicap", "dshow", "pvapi", "openni", "openni_asus", "android", "xiapi", "avfoundation", "giganetix", "msmf", "winrt", "intelperc", "openni2", "openni2_asus", "gphoto2", "gstreamer", "ffmpeg", "images", "aravis", "opencv_mjpeg", "intel_mfx", "xine".  
See the OpenCV documentation of cv::VideoCapture for more detail.

### Dependencies

[BufferedSocket](https://github.com/RandomPrototypes/BufferedSocket)  
[RPCameraInterface](https://github.com/RandomPrototypes/RPCameraInterface)  
[libQuestMR](https://github.com/RandomPrototypes/libQuestMR)  
[tinyxml2](https://github.com/leethomason/tinyxml2)  
[FFMPEG](https://github.com/FFmpeg/FFmpeg)  
[OpenCV](https://github.com/opencv/opencv)  
[onnx-runtime](https://github.com/microsoft/onnxruntime)  
[RobustVideoMatting](https://github.com/PeterL1n/RobustVideoMatting/)

### Credits

The quest streaming code in libQuestMR is based on the official [OBS plugin for Quest 2](https://github.com/facebookincubator/obs-plugins).  
I also got some inspiration from [RealityMixer](https://github.com/fabio914/RealityMixer)
