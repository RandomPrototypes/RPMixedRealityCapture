#!/bin/bash

RPMixedRealityCapture_branch=$(git symbolic-ref --short HEAD)
if [ $RPMixedRealityCapture_branch = "master" ]; then
	libQuestMR_branch="master"
elif [ $RPMixedRealityCapture_branch = "dev" ]; then
	libQuestMR_branch="dev"
else
	libQuestMR_branch="v1.0.0"
fi
