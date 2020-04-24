#ifndef __CVVXDISPLAY_H__
#define __CVVXDISPLAY_H__

using namespace std;
using namespace cv;
int cvVxShow(Mat &frame);
int cvVxInitDisplay();
int cvGetDisplaySize(int *width, int *height);

#ifndef DEFAULT_FBDEV
#define DEFAULT_FBDEV "/dev/fb2"
#endif

#endif /* __CVVXDISPLAY_H__ */
