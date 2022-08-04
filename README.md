# RPMixedRealityCapture

Unofficial mixed reality capture software for Quest 2 without greenscreen, with support for usb webcams and android phones (soon).  
Already usable but still have some bugs to fix. Easiest way to install is to use the buildWithDeps.sh script for linux. An installer for windows will be available soon.
Based on [libQuestMR](https://github.com/RandomPrototypes/libQuestMR) for communication with the Quest and
[RPCameraInterface](https://github.com/RandomPrototypes/RPCameraInterface) for multi-platform camera interface (use the dev branch to compile with current version).

# How to install?

On linux, just run the build script :  
`bash buildWithDeps.sh --cuda`  
Remove --cuda if your computer does not have CUDA installed.  

On windows, use the installer (coming soon) or build from source.
