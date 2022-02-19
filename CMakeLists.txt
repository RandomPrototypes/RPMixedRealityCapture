cmake_minimum_required(VERSION 3.5)

project(RPMixedRealityCapture VERSION 0.1 LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(libQuestMR /home/arnaud/workspace/libQuestMR)

set(BufferedSocket /home/arnaud/workspace/BufferedSocket)

set(RPCameraInterface /home/arnaud/workspace/RPCameraInterface)

find_package(QT NAMES Qt6 Qt5 COMPONENTS Widgets Multimedia REQUIRED)
find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Widgets Multimedia REQUIRED)
find_package(OpenCV REQUIRED HINTS /home/arnaud/opencv-4.5.1/install/release/lib/cmake/opencv4)
find_package(libwebcam REQUIRED HINTS ${RPCameraInterface}/deps/libwebcam-master/install/lib/cmake/libwebcam)

find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBAV REQUIRED IMPORTED_TARGET
    libavdevice
    libavfilter
    libavformat
    libavcodec
    libswresample
    libswscale
    libavutil
)

set(PROJECT_SOURCES        
        include/mainwindow.h
        include/Util.hpp
        include/ConnectQuestPage.h
        include/CameraPreviewPage.h
        include/CameraSelectPage.h
        include/CalibrationOptionPage.h
        include/PostProcessingPage.h
        include/CalibrateWithChessboardPage.h
        include/CalibrateCameraPosePage.h
        include/RecordMixedRealityPage.h
        include/CheckCalibrationPage.h
        src/main.cpp
        src/mainwindow.cpp
        src/ConnectQuestPage.cpp
        src/PostProcessingPage.cpp
        src/CameraSelectPage.cpp
        src/CalibrationOptionPage.cpp
        src/CalibrateWithChessboardPage.cpp
        src/CalibrateCameraPosePage.cpp
        src/RecordMixedRealityPage.cpp
        src/CheckCalibrationPage.cpp
        src/CameraPreviewPage.cpp
        src/Util.cpp
        include/VideoInputMngr.h
        src/VideoInputMngr.cpp
        include/OpenCVWidget.h
        src/OpenCVWidget.cpp
        include/VideoEncoder.h
        src/VideoEncoder.cpp
        ${RPCameraInterface}/include/CameraInterface.h
        ${RPCameraInterface}/src/CameraInterface.cpp
        ${RPCameraInterface}/include/CameraInterfaceV4l2.h
        ${RPCameraInterface}/src/CameraInterfaceV4l2.cpp
        ${RPCameraInterface}/include/CameraInterfaceLibWebcam.h
        ${RPCameraInterface}/src/CameraInterfaceLibWebcam.cpp
        ${RPCameraInterface}/include/CameraEnumeratorQt.h
        ${RPCameraInterface}/src/CameraEnumeratorQt.cpp
        ${RPCameraInterface}/include/CameraInterfaceAndroid.h
        ${RPCameraInterface}/src/CameraInterfaceAndroid.cpp
        ${RPCameraInterface}/include/ImageFormat.h
        ${RPCameraInterface}/src/ImageFormat.cpp
        ${RPCameraInterface}/include/ImageData.h
        ${RPCameraInterface}/src/ImageData.cpp
        ${RPCameraInterface}/include/ImageFormatConverter.h
        ${RPCameraInterface}/src/ImageFormatConverter.cpp
        ${BufferedSocket}/BufferedSocket.h
        ${BufferedSocket}/BufferedSocket.cpp
        ${libQuestMR}/include/QuestCalibData.h
        ${libQuestMR}/src/QuestCalibData.cpp
        ${libQuestMR}/include/QuestStringUtil.h
        ${libQuestMR}/src/QuestStringUtil.cpp
        ${libQuestMR}/include/QuestFrameData.h
        ${libQuestMR}/src/QuestFrameData.cpp
        ${libQuestMR}/include/QuestCommunicator.h
        ${libQuestMR}/src/QuestCommunicator.cpp
        ${libQuestMR}/include/QuestVideoMngr.h
        ${libQuestMR}/src/QuestVideoMngr.cpp
        ${libQuestMR}/include/frame.h
        ${libQuestMR}/src/frame.cpp
        ${libQuestMR}/deps/tinyxml2/include/tinyxml2.h
        ${libQuestMR}/deps/tinyxml2/src/tinyxml2.cpp
)

include_directories(include ${libQuestMR}/include ${libQuestMR}/deps/tinyxml2/include  ${RPCameraInterface}/include ${BufferedSocket}  ${RPCameraInterface}/deps/libwebcam-master/install/include)

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(RPMixedRealityCapture
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
    )
# Define target properties for Android with Qt 6 as:
#    set_property(TARGET RPMixedRealityCapture APPEND PROPERTY QT_ANDROID_PACKAGE_SOURCE_DIR
#                 ${CMAKE_CURRENT_SOURCE_DIR}/android)
# For more information, see https://doc.qt.io/qt-6/qt-add-executable.html#target-creation
else()
    if(ANDROID)
        add_library(RPMixedRealityCapture SHARED
            ${PROJECT_SOURCES}
        )
# Define properties for Android with Qt 5 after find_package() calls as:
#    set(ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/android")
    else()
        add_executable(RPMixedRealityCapture
            ${PROJECT_SOURCES}
        )
    endif()
endif()

target_link_libraries(RPMixedRealityCapture PRIVATE Qt${QT_VERSION_MAJOR}::Widgets Qt${QT_VERSION_MAJOR}::Multimedia ${OpenCV_LIBS} PkgConfig::LIBAV webcam)

set_target_properties(RPMixedRealityCapture PROPERTIES
    MACOSX_BUNDLE_GUI_IDENTIFIER my.example.com
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(RPMixedRealityCapture)
endif()
