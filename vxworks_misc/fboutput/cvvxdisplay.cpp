#include "opencv2/highgui.hpp"

#include <iostream>
#include <stdio.h>
#include <vxWorks.h>
#include <iosLib.h>

extern "C"
{
#include "fboutput.h"
}

#include "cvvxdisplay.hpp"

struct displayInfoStruct* displayHandle = NULL;

int cvVxInitDisplay() {
    if (displayHandle == NULL)
		displayHandle = displayOpen((char *)DEFAULT_FBDEV);
    if (displayHandle == NULL) {
    	printf("Unable to open display %s.\n", DEFAULT_FBDEV, 0, 0, 0, 0, 0);
      	return -1;
	}
    return 0;
}

int cvVxShow(Mat &frame) {
	if (displayHandle == NULL)
		return -1;
	if (frame.data){
		if (displayHandle->drawHeight != frame.rows ||
		    displayHandle->drawWidth != frame.cols ) {
			displayCenterImage(displayHandle, frame.cols, frame.rows);
     	}
    	displayDrawImg(displayHandle, frame.data, frame.cols * 4);
        return 0;
   } else {
   		printf("frame is empty\n");
   		return -1;
   }
}

int cvGetDisplaySize(int *width, int *height) {
	if ((width == NULL) || (height == NULL) || (displayHandle == NULL))
		return -1;
	*width = displayHandle->screenWidth;
	*height = displayHandle->screenHeight;
	printf("%s:%d dw = %d dh = %d (%d, %d)\n", __FUNCTION__, __LINE__, *width, *height, displayHandle->drawWidth, displayHandle->drawHeight);
	return 0;
}

