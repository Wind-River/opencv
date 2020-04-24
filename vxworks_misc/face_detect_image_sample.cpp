
#include <opencv2/imgproc.hpp>  // Gaussian Blur
#include <opencv2/core.hpp>        // Basic OpenCV structures (cv::Mat, Scalar)
#include <opencv2/videoio.hpp>
#include <opencv2/highgui.hpp>  // OpenCV window I/O
#include <opencv2/features2d.hpp>
#include <opencv2/objdetect.hpp>

#include <stdio.h>
#include <string>
#include <vector>

using namespace std;
using namespace cv;
#include "fboutput/cvvxdisplay.hpp"
const string WindowName = "Face Detection example";

class CascadeDetectorAdapter: public DetectionBasedTracker::IDetector
{
    public:
        CascadeDetectorAdapter(cv::Ptr<cv::CascadeClassifier> detector):
            IDetector(),
            Detector(detector)
        {
            CV_Assert(detector);
        }

        void detect(const cv::Mat &Image, std::vector<cv::Rect> &objects)
        {
            Detector->detectMultiScale(Image, objects, scaleFactor, minNeighbours, 0, minObjSize, maxObjSize);
        }

        virtual ~CascadeDetectorAdapter()
        {}

    private:
        CascadeDetectorAdapter();
        cv::Ptr<cv::CascadeClassifier> Detector;
 };

int main(int , char** )
{
    //namedWindow(WindowName);
    cvVxInitDisplay();
    VideoCapture VideoStream(0);
    VideoWriter out_vid;

    if (!VideoStream.isOpened())
    {
        printf("Error: Cannot open video stream from camera\n");
        return 1;
    }

    std::string cascadeFrontalfilename = "lbpcascade_frontalface.xml";
    cv::Ptr<cv::CascadeClassifier> cascade = makePtr<cv::CascadeClassifier>(cascadeFrontalfilename);
    cv::Ptr<DetectionBasedTracker::IDetector> MainDetector = makePtr<CascadeDetectorAdapter>(cascade);
    if ( cascade->empty() )
    {
      printf("Error: Cannot load %s\n", cascadeFrontalfilename.c_str());
      return 2;
    }

    cascade = makePtr<cv::CascadeClassifier>(cascadeFrontalfilename);
    cv::Ptr<DetectionBasedTracker::IDetector> TrackingDetector = makePtr<CascadeDetectorAdapter>(cascade);
    if ( cascade->empty() )
    {
      printf("Error: Cannot load %s\n", cascadeFrontalfilename.c_str());
      return 2;
    }

    DetectionBasedTracker::Parameters params;
    DetectionBasedTracker Detector(MainDetector, TrackingDetector, params);

    if (!Detector.run())
    {
        printf("Error: Detector initialization failed\n");
        return 2;
    }

    Mat ReferenceFrame;
    Mat GrayFrame;
    Mat frame1;
    vector<Rect> Faces;
    VideoStream.set(CAP_PROP_FRAME_WIDTH, 640);
    VideoStream.set(CAP_PROP_FRAME_HEIGHT, 480);
    VideoStream.set(CAP_PROP_FPS, 30);
    out_vid.open("dbtface.avi", VideoWriter::fourcc('M','J','P','G'), 10,
            Size(VideoStream.get(CAP_PROP_FRAME_WIDTH),VideoStream.get(CAP_PROP_FRAME_HEIGHT)),true);
    int j = 0;
    do
    {
        VideoStream >> frame1;
        cvtColor(frame1, ReferenceFrame, COLOR_YUV2BGR_YUY2);
        cvtColor(ReferenceFrame, GrayFrame, COLOR_RGB2GRAY);
        Detector.process(GrayFrame);
        Detector.getObjects(Faces);

        for (size_t i = 0; i < Faces.size(); i++)
        {
            rectangle(ReferenceFrame, Faces[i], Scalar(0,255,0));
        }

        //imshow(WindowName, ReferenceFrame);
        cvtColor(ReferenceFrame, ReferenceFrame, COLOR_BGR2BGRA);
        cvVxShow(ReferenceFrame);
        if (out_vid.isOpened())
            out_vid.write(ReferenceFrame);
        j++;
    } while ( j < 300);
    out_vid.release();
    Detector.stop();

    return 0;
}
