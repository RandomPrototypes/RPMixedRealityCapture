import cv2
import numpy as np
import mediapipe as mp
import sys

mp_pose = mp.solutions.pose

def processVideo(inputVideo, outputVideo):
	cap = cv2.VideoCapture(inputVideo)
	vidWriter = None
	with mp_pose.Pose(
		min_detection_confidence=0.5,
		min_tracking_confidence=0.5,
		enable_segmentation=True) as pose:
		
		nbFrames = 0
	  
		while cap.isOpened():
			success, image = cap.read()
			if not success:
				break
			
			if vidWriter is None and outputVideo is not None:
				cols = image.shape[1]
				rows = image.shape[0]
				vidWriter = cv2.VideoWriter(outputVideo, cv2.VideoWriter_fourcc(*'mp4v'), 20.0, (cols,rows))

			# To improve performance, optionally mark the image as not writeable to
			# pass by reference.
			image.flags.writeable = False
			image = cv2.cvtColor(image, cv2.COLOR_BGR2RGB)
			results = pose.process(image)
		
			if results.segmentation_mask is not None:
				mask = np.uint8(results.segmentation_mask * 255)
				mask = np.stack((mask,) * 3, axis=-1)
			else:
				mask = np.zeros(image.shape,dtype = "uint8")
			
			if outputVideo is None:
				cv2.imshow('annotated_image', cv2.flip(mask, 1))
				if cv2.waitKey(5) & 0xFF == 27:
					break
			else:
				vidWriter.write(mask)
			
			print(nbFrames, flush=True)
			nbFrames += 1
		cap.release()
		if vidWriter is not None:
			vidWriter.release()



if __name__ == '__main__':
    if len(sys.argv) == 1:
    	processVideo(0, None)
    elif len(sys.argv) == 2:
    	processVideo(sys.argv[1], None)
    elif len(sys.argv) == 3:
    	processVideo(sys.argv[1], sys.argv[2])
    else:
    	print("usage: python humanSegmentation.py inputVideo outputVideo")
