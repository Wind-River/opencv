import cv2

capture = cv2.VideoCapture(0)

ret, frame = capture.read()

cv2.imwrite("capturepy.png", frame) 
capture.release()
