#!/bin/bash
source config.sh #config.sh from RPMixedRealityCapture

USE_CUDA=0
for i in "$@"; do
  case $i in
    --cuda)
      USE_CUDA=1
      shift # past argument with no value
      ;;
    -*|--*)
      echo "Unknown option $i"
      exit 1
      ;;
    *)
      ;;
  esac
done

BASE_FOLDER=$(pwd)
mkdir deps
cd deps

DEPS_FOLDER=$BASE_FOLDER/deps

#install libQuestMR
cd $DEPS_FOLDER
git clone --branch $libQuestMR_branch https://github.com/RandomPrototypes/libQuestMR.git
cd libQuestMR
git pull
if [ $USE_CUDA = 1 ]; then
	bash buildWithDeps.sh --no_demo --cuda  || exit 1
else
	bash buildWithDeps.sh --no_demo || exit 1
fi
source config.sh #config.sh from libQuestMR
cd $BASE_FOLDER

mkdir build
mkdir install
cd build

CUSTOM_CMAKE=cmake #$DEPS_FOLDER/libQuestMR/deps/cmake-3.22.3/install/bin/cmake
BUFFERED_SOCKET_CMAKE_DIR=$DEPS_FOLDER/libQuestMR/deps/BufferedSocket/install/lib/cmake/BufferedSocket
RP_CAMERA_INTERFACE_CMAKE_DIR=$DEPS_FOLDER/libQuestMR/deps/RPCameraInterface/install/lib/cmake/RPCameraInterface
LIBQUESTMR_CMAKE_DIR=$DEPS_FOLDER/libQuestMR/install/lib/cmake/libQuestMR


$CUSTOM_CMAKE -DCMAKE_INSTALL_PREFIX=../install -DCMAKE_BUILD_TYPE=$BUILD_TYPE -DBufferedSocket_DIR=$BUFFERED_SOCKET_CMAKE_DIR -DRPCameraInterface_DIR=$RP_CAMERA_INTERFACE_CMAKE_DIR -DlibQuestMR_DIR=$LIBQUESTMR_CMAKE_DIR ..  || exit 1
make -j8  || exit 1
cp RPMixedRealityCapture $BASE_FOLDER/install
cd $BASE_FOLDER/install
ln -sfn $DEPS_FOLDER/libQuestMR/deps/BufferedSocket/install/lib/libBufferedSocket.so.0.1 libBufferedSocket.so.0.1
ln -sfn $DEPS_FOLDER/libQuestMR/deps/RPCameraInterface/install/lib/libRPCameraInterface.so libRPCameraInterface.so
ln -sfn $DEPS_FOLDER/libQuestMR/install/lib/$onnxruntime_lib_name $onnxruntime_lib_name
if [ $USE_CUDA = 1 ]; then
	ln -sfn $DEPS_FOLDER/libQuestMR/install/lib/libonnxruntime_providers_shared.so libonnxruntime_providers_shared.so
	ln -sfn $DEPS_FOLDER/libQuestMR/install/lib/libonnxruntime_providers_cuda.so libonnxruntime_providers_cuda.so
fi
ln -sfn $BASE_FOLDER/resources resources
mkdir $BASE_FOLDER/resources/backgroundSub_data
cd $BASE_FOLDER/resources/backgroundSub_data
wget https://github.com/PeterL1n/RobustVideoMatting/releases/download/v1.0.0/rvm_mobilenetv3_fp32.onnx

