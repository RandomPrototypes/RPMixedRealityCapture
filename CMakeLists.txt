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
    tinyxml2
    GIT_REPOSITORY https://github.com/leethomason/tinyxml2.git
    GIT_TAG 1dee28e51f9175a31955b9791c74c430fe13dc82
)

find_package(BufferedSocket REQUIRED)
message("BufferedSocket dir: ${BufferedSocket_DIR}")
set(BufferedSocket_INCLUDE_DIR ${BufferedSocket_DIR}/../../../include)

SET(tinyxml2 ${tinyxml2_SOURCE_DIR})

find_package(QT NAMES Qt6 Qt5 COMPONENTS Widgets Multimedia REQUIRED)
find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Widgets Multimedia REQUIRED)
find_package(OpenCV REQUIRED imgproc videoio calib3d)
find_package(RPCameraInterface REQUIRED)
find_package(libQuestMR REQUIRED)

if(WIN32)
   SET(FFMPEG_DIR "" CACHE STRING "ffmpeg dir")
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
   pkg_check_modules(LIBAV REQUIRED IMPORTED_TARGET libavdevice libavfilter libavformat libavcodec libswresample libswscale libavutil)
   get_cmake_property(_variableNames VARIABLES)
   list (SORT _variableNames)
   foreach (_variableName ${_variableNames})
       message(STATUS "${_variableName}=${${_variableName}}")
   endforeach()
   include_directories(${LIBAV_INCLUDE_DIRS})
endif()

set(PROJECT_SOURCES
        include/mainwindow.h
        include/Util.hpp
        include/FirstMenuPage.h
        include/ConnectQuestPage.h
        include/CameraPreviewPage.h
        include/CameraSelectPage.h
        include/CalibrationOptionPage.h
        include/CalibrateCameraQuestMenuPage.h
        include/PostProcessingOptionPage.h
        include/PostProcessingPage.h
        include/CalibrateWithChessboardPage.h
        include/CalibrateCameraPosePage.h
        include/RecordMixedRealityPage.h
        include/CheckCalibrationPage.h
        src/main.cpp
        src/mainwindow.cpp
        src/FirstMenuPage.cpp
        src/ConnectQuestPage.cpp
        src/PostProcessingOptionPage.cpp
        src/PostProcessingPage.cpp
        src/CameraSelectPage.cpp
        src/CalibrationOptionPage.cpp
        src/CalibrateCameraQuestMenuPage.cpp
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
        ${tinyxml2}/tinyxml2.h
        ${tinyxml2}/tinyxml2.cpp
)

if(UNIX)
set(PROJECT_SOURCES  ${PROJECT_SOURCES})
endif()

message("RPCameraInterface include dir: ${RPCameraInterface_DIR}")
set(RPCameraInterface_INCLUDE_DIR ${RPCameraInterface_DIR}/../../../include)
message("libQuestMR include dir: ${libQuestMR_DIR}")
set(libQuestMR_INCLUDE_DIR ${libQuestMR_DIR}/../../../include)

include_directories(include ${libQuestMR}/include ${RPCameraInterface_INCLUDE_DIR} ${libQuestMR_INCLUDE_DIR} ${BufferedSocket_INCLUDE_DIR} ${tinyxml2})

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

target_link_libraries(RPMixedRealityCapture PRIVATE BufferedSocket RPCameraInterface libQuestMR)

if(WIN32)
        if(MSVC)
                target_link_libraries(RPMixedRealityCapture PRIVATE avdevice avfilter avformat avcodec swresample swscale avutil)
        else()
                target_link_libraries(RPMixedRealityCapture PRIVATE libavdevice libavfilter libavformat libavcodec libswresample libswscale libavutil)
        endif()
else()
        target_link_libraries(RPMixedRealityCapture PRIVATE PkgConfig::LIBAV)
endif()

set_target_properties(RPMixedRealityCapture PROPERTIES
    MACOSX_BUNDLE_GUI_IDENTIFIER my.example.com
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(RPMixedRealityCapture)
endif()
