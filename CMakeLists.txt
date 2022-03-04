cmake_minimum_required(VERSION 3.11.0)

project(RPMixedRealityCapture VERSION 0.1 LANGUAGES CXX)

include(FetchContent)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(EXTERNAL_INSTALL_LOCATION deps)

FetchContent_Populate(
    BufferedSocket
    GIT_REPOSITORY https://github.com/RandomPrototypes/BufferedSocket.git
    GIT_TAG 45c3d9e33f20aafa19ec4f03d39dcbc9c529726a
)

FetchContent_Populate(
    LibWebcam
    GIT_REPOSITORY https://github.com/rojarand/libwebcam.git
    GIT_TAG c8e81d6e898bb4dd1bd30fce5ebd6644680e0a2f
)

FetchContent_Populate(
    libQuestMR
    GIT_REPOSITORY https://github.com/RandomPrototypes/libQuestMR.git
    GIT_TAG 934077b3cd5130367b31ef537d8b35f25d951c5f
)

FetchContent_Populate(
    tinyxml2
    GIT_REPOSITORY https://github.com/leethomason/tinyxml2.git
    GIT_TAG 1dee28e51f9175a31955b9791c74c430fe13dc82
)

SET(BufferedSocket ${bufferedsocket_SOURCE_DIR})
SET(RPCameraInterface ${rpcamerainterface_SOURCE_DIR})
SET(libQuestMR ${libquestmr_SOURCE_DIR})
SET(libwebcam ${libwebcam_SOURCE_DIR})
SET(tinyxml2 ${tinyxml2_SOURCE_DIR})


find_package(QT NAMES Qt6 Qt5 COMPONENTS Widgets Multimedia REQUIRED)
find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Widgets Multimedia REQUIRED)
find_package(OpenCV REQUIRED HINTS C:/opencv-4.5.5/release/install/x64/mingw/lib)
find_package(RPCameraInterface REQUIRED HINTS C:/workspace/RPCameraInterface/install/lib/cmake/)


if(WIN32)
   SET(FFMPEG_DIR "" CACHE STRING "ffmpeg dir" HINTS C:/ffmpeg5.0)
   SET(FFMPEG_INCLUDE_DIR "" CACHE STRING "ffmpeg include dir")
   SET(FFMPEG_LIB_DIR "" CACHE STRING "ffmpeg lib dir")
   if(FFMPEG_DIR AND (NOT FFMPEG_INCLUDE_DIR))
      set(FFMPEG_INCLUDE_DIR ${FFMPEG_DIR}/include CACHE STRING "ffmpeg include dir" FORCE)
   endif()
   if(FFMPEG_DIR AND (NOT FFMPEG_LIB_DIR))
      set(FFMPEG_LIB_DIR ${FFMPEG_DIR}/lib CACHE STRING "ffmpeg lib dir" FORCE)
   endif()
   include_directories(${FFMPEG_INCLUDE_DIR})
   link_directories(${FFMPEG_LIB_DIR})
else()
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
endif()

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
        ${tinyxml2}/tinyxml2.h
        ${tinyxml2}/tinyxml2.cpp
)

if(UNIX)
set(PROJECT_SOURCES  ${PROJECT_SOURCES}
)
endif()

message("include dir: ${RPCameraInterface_DIR}")
set(RPCameraInterface_INCLUDE_DIR ${RPCameraInterface_DIR}/../../../include)

include_directories(include ${libQuestMR}/include ${tinyxml2}  ${RPCameraInterface_INCLUDE_DIR} ${BufferedSocket})

if(QT_VERSION_MAJOR GREATER_EQUAL 6)
    qt_add_executable(RPMixedRealityCapture
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
    )
# Define target properties for Android with Qt 6 as:
#    set_property(TARGET RPMixedRealityCapture APPEND PROPERTY QT_ANDROID_PACKAGE_SOURCE_DIR
#                 ${CMAKE_CURRENT_SOURCE_DIR}/android)
# For more information, see https://doc.qt.io/qt-6/qt-add-executable.html#target-creation
else()
        add_executable(RPMixedRealityCapture
            ${PROJECT_SOURCES}
        )
endif()

target_link_libraries(RPMixedRealityCapture PRIVATE Qt${QT_VERSION_MAJOR}::Widgets Qt${QT_VERSION_MAJOR}::Multimedia ${OpenCV_LIBS})

target_link_libraries(RPMixedRealityCapture PRIVATE
        libavdevice
        libavfilter
        libavformat
        libavcodec
        libswresample
        libswscale
        libavutil)

target_link_libraries(RPMixedRealityCapture PRIVATE RPCameraInterface)

set_target_properties(RPMixedRealityCapture PROPERTIES
    MACOSX_BUNDLE_GUI_IDENTIFIER my.example.com
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(RPMixedRealityCapture)
endif()
