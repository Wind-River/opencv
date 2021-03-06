#include <opencv2/dnn.hpp>
#include <opencv2/dnn/shape_utils.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/highgui.hpp>

#include <clockLib.h>

using namespace cv;
using namespace cv::dnn;

#include <fstream>
#include <iostream>
#include <cstdlib>
#include <sys/time.h>
#include <unistd.h>

#define CV_VX_SHOW
#ifdef CV_VX_SHOW
#include "fboutput/cvvxdisplay.hpp"
#endif

using namespace std;

const size_t inWidth = 300;
const size_t inHeight = 300;
const float inScaleFactor = 0.007843f;
const float meanVal = 127.5;
const char* classNames[] = {"background",
                            "aeroplane", "bicycle", "bird", "boat",
                            "bottle", "bus", "car", "cat", "chair",
                            "cow", "diningtable", "dog", "horse",
                            "motorbike", "person", "pottedplant",
                            "sheep", "sofa", "train", "tvmonitor"};

const char* params
    = "{ help           | false | print usage         }"
      "{ proto          | MobileNetSSD_deploy.prototxt | model configuration }"
      "{ model          | MobileNetSSD_deploy.caffemodel | model weights }"
      "{ camera_device  | 0     | camera device number }"
      "{ video          |       | video or image for detection}"
      "{ out            |       | path to output video file}"
      "{ min_confidence | 0.2   | min confidence      }"
      "{ opencl         | false | enable OpenCL }"
;


int mnet_ssd(int argc, char** argv)
{
    VideoCapture cap;
    int ret = -1;
#ifdef CV_VX_SHOW
    int disp_w = 1920, disp_h = 1080;
    if (cvVxInitDisplay() != 0) {
		cout << "Couldn't initialize display" << endl;
		return -1;
	}
    cvGetDisplaySize(&disp_w,  &disp_h);
#endif
    try {
    int cameraDevice = -1;
    CommandLineParser parser(argc, argv, params);
    parser.about("This sample uses MobileNet Single-Shot Detector "
                 "(https://arxiv.org/abs/1704.04861) "
                 "to detect objects on camera/video/image.\n"
                 ".caffemodel model's file is available here: "
                 "https://github.com/chuanqi305/MobileNet-SSD\n"
                 "Default network is 300x300 and 20-classes VOC.\n");

    if (parser.get<bool>("help") || argc == 1)
    {
        parser.printMessage();
        return 0;
    }

    String modelConfiguration = parser.get<string>("proto");
    String modelBinary = parser.get<string>("model");
    CV_Assert(!modelConfiguration.empty() && !modelBinary.empty());

    //! [Initialize network]
    dnn::Net net = readNetFromCaffe(modelConfiguration, modelBinary);
    //! [Initialize network]

    if (parser.get<bool>("opencl"))
    {
        net.setPreferableTarget(DNN_TARGET_OPENCL);
    }

    if (net.empty())
    {
        cerr << "Can't load network by using the following files: " << endl;
        cerr << "prototxt:   " << modelConfiguration << endl;
        cerr << "caffemodel: " << modelBinary << endl;
        cerr << "Models can be downloaded here:" << endl;
        cerr << "https://github.com/chuanqi305/MobileNet-SSD" << endl;
        return (-1);
    }
    if (!parser.has("video"))
    {
        cameraDevice = parser.get<int>("camera_device");
        cap = VideoCapture(cameraDevice);
        if(!cap.isOpened())
        {
            cout << "Couldn't find camera: " << cameraDevice << endl;
            return -1;
        }
        cap.set(CAP_PROP_FRAME_WIDTH, 640);
        //cap.set(CAP_PROP_FRAME_HEIGHT, 480);
        //cap.set(CAP_PROP_FPS, 30);
    }
    else
    {
        cap.open(parser.get<String>("video"));
        if(!cap.isOpened())
        {
            cout << "Couldn't open image or video: " << parser.get<String>("video") << endl;
            return -1;
        }
    }

    //Acquire input size
    Size inVideoSize((int) cap.get(CAP_PROP_FRAME_WIDTH),
                     (int) cap.get(CAP_PROP_FRAME_HEIGHT));

    double fps = cap.get(CAP_PROP_FPS);
    int fourcc = static_cast<int>(cap.get(CAP_PROP_FOURCC));
    VideoWriter outputVideo;
    outputVideo.open(parser.get<String>("out") ,
                     (fourcc != 0 ? fourcc : VideoWriter::fourcc('M','J','P','G')),
                     (fps != 0 ? fps : 10.0), inVideoSize, true);

    for(;;)
    {
        Mat frame;

        cap >> frame; // get a new frame from camera/video or read image

        if (frame.empty())
        {
            printf("%s:%d - frame empty\n", __FUNCTION__, __LINE__);
        	ret = -1;
            break;
        }
        if (cameraDevice != -1)
        	cvtColor(frame, frame, COLOR_YUV2BGR_YUY2);
        
        //! [Prepare blob]
        Mat inputBlob = blobFromImage(frame, inScaleFactor,
                                      Size(inWidth, inHeight),
                                      Scalar(meanVal, meanVal, meanVal),
                                      false, false); //Convert Mat to batch of images
        //! [Prepare blob]

        //! [Set input blob]
        net.setInput(inputBlob); //set the network input
        //! [Set input blob]

        //! [Make forward pass]
        Mat detection = net.forward(); //compute output
        //! [Make forward pass]

        vector<double> layersTimings;
        double freq = getTickFrequency() / 1000;
        double time = net.getPerfProfile(layersTimings) / freq;
		printf("FPS: %.2f time: %.2f ms\n", 1000.f/time, time);

        Mat detectionMat(detection.size[2], detection.size[3], CV_32F, detection.ptr<float>());

        if (!outputVideo.isOpened())
        {
            putText(frame, format("FPS: %.2f ; time: %.2f ms", 1000.f/time, time),
                    Point(20,20), 0, 0.5, Scalar(0,0,255));
        }
        else
            cout << "Inference time, ms: " << time << endl;

        float confidenceThreshold = parser.get<float>("min_confidence");
        for(int i = 0; i < detectionMat.rows; i++)
        {
            float confidence = detectionMat.at<float>(i, 2);

            if(confidence > confidenceThreshold)
            {
                size_t objectClass = (size_t)(detectionMat.at<float>(i, 1));

                int left = static_cast<int>(detectionMat.at<float>(i, 3) * frame.cols);
                int top = static_cast<int>(detectionMat.at<float>(i, 4) * frame.rows);
                int right = static_cast<int>(detectionMat.at<float>(i, 5) * frame.cols);
                int bottom = static_cast<int>(detectionMat.at<float>(i, 6) * frame.rows);

                rectangle(frame, Point(left, top), Point(right, bottom), Scalar(0, 255, 0));
                String label = format("%s: %.2f", classNames[objectClass], confidence);
                printf("%s: %.2f\n", classNames[objectClass], confidence);
                int baseLine = 0;
                Size labelSize = getTextSize(label, FONT_HERSHEY_SIMPLEX, 0.5, 1, &baseLine);
                top = max(top, labelSize.height);
                rectangle(frame, Point(left, top - labelSize.height),
                          Point(left + labelSize.width, top + baseLine),
                          Scalar(255, 255, 255), FILLED);
                putText(frame, label, Point(left, top),
                        FONT_HERSHEY_SIMPLEX, 0.5, Scalar(0,0,0));
            }
        }

        if (outputVideo.isOpened())
            outputVideo << frame;
#ifdef CV_VX_SHOW
        Mat f1, f2;
        cvtColor(frame, f1, COLOR_BGR2BGRA);
        resize(f1, f2, Size(disp_w, disp_h));
        cvVxShow(f2);
#endif
    }
} catch (...) {
    cap.release();
    ret = -1;
}

    return ret;
} // ssd



int main(int argc, char **argv) {
	mnet_ssd(argc, argv);
}
