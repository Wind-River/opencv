/*
  * Copyright (c) 2017, Wind River Systems, Inc.
*
* Redistribution and use in source and binary forms, with or without modification, are
* permitted provided that the following conditions are met:
*
* 1) Redistributions of source code must retain the above copyright notice,
* this list of conditions and the following disclaimer.
*
* 2) Redistributions in binary form must reproduce the above copyright notice,
* this list of conditions and the following disclaimer in the documentation and/or
* other materials provided with the distribution.
*
* 3) Neither the name of Wind River Systems nor the names of its contributors may be
* used to endorse or promote products derived from this software without specific
* prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
* AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
* IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
* ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
* LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
* DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
* SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
* CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
* OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
* USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
#ifdef __VXWORKS__
#include "precomp.hpp"
#include <stdio.h>
#include <vxWorks.h>
#include <iosLib.h>
#include <errno.h>
#include <errnoLib.h>
#include <selectLib.h>
#include <clockLib.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <fcntl.h>
/* Not technically required, but needed on some UNIX distributions */
#include <sys/types.h>
#include <sys/stat.h>

#include "cap_uvc_vx.hpp"
#include "cap_interface.hpp"


#include "precomp.hpp"


namespace cv {
class CvCaptureCAM_VX : public IVideoCapture
{
public:
    void init()
    {
        m_index = -1;
        m_bsize = 0;
        m_vbuf = NULL;
    }

    CvCaptureCAM_VX(int index)
    {
        init();
        open(index);
    }

    virtual ~CvCaptureCAM_VX()
    {
        close();
    }

    virtual double getProperty(int propIdx) const  CV_OVERRIDE;
    virtual bool setProperty(int propIdx, double propVal) CV_OVERRIDE;

    virtual bool grabFrame() CV_OVERRIDE;
    virtual bool retrieveFrame(int outputType, OutputArray frame) CV_OVERRIDE ;
    virtual int getCaptureDomain() CV_OVERRIDE;

    virtual bool isOpened() const CV_OVERRIDE;
    virtual bool open(int cameraId);
    virtual void close();

protected:
    int m_fd;
    int m_index;
    uint32_t m_frameIntervalUs;
    uint32_t m_bufferSize;
    uint16_t m_width;
    uint16_t m_height;
    uint32_t m_bsize;
    uint32_t m_format;
    Mat  m_frame;
    char *m_vbuf;

    bool grabbedInOpen;

    CvSize getSize();
    bool setSize(int, int);
    bool setFrameRate(int rate);
    bool setFormat(uint16_t width, uint16_t height, uint32_t format);
};

bool CvCaptureCAM_VX::isOpened() const
{
    return (m_fd != -1);
}

bool CvCaptureCAM_VX::open(int index) {
    char dev[7] = {0};
    uint32_t bsize = 0;
    uint32_t format = VIDEO_FMT_YUY2;

    snprintf(dev, sizeof(dev), "/uvc/%d", index);
    if ((m_fd = ::open(dev, O_RDONLY, 0)) == -1) {
        return false;
    }

    /* set initial format */
    if (setFormat(1280, 720, format) == false) {
        ::close(m_fd);
        m_fd = -1;
        return false;
    }


    if (setFrameRate(30) == false) {
        ::close(m_fd);
        m_fd = -1;
        return false;
    }


    if (ioctl(m_fd, USB2_VIDEO_IOCTL_GET_BUFFER_SIZE, &bsize) != 0) {
        ::close(m_fd);
        m_fd = -1;
        return false;
    }

    m_format = format;
    m_bsize = bsize;
    m_index = index;

    return true;
}

void CvCaptureCAM_VX::close()
{
    ::close(m_fd);
    m_fd = -1;
    if (m_vbuf) {
        free(m_vbuf);
        m_vbuf = NULL;
    }
}

bool CvCaptureCAM_VX::grabFrame()
{

    bool ret = false;
    int r = -1;
    fd_set rfdset;

    size_t size = 0;

    if (m_fd == -1)
        return false;

    if (m_vbuf == NULL){
        return false;
    }

    FD_ZERO(&rfdset);
    FD_SET(m_fd, &rfdset);

    r = select(m_fd + 1, &rfdset, NULL, NULL, NULL);
    if (r < 0)
        return false;

    if (FD_ISSET(m_fd, &rfdset)) {
        if ((size = ::read(m_fd, m_vbuf, m_bsize)) >=0) {
            Mat rawData(1, size, CV_8SC1, (void*)m_vbuf);
            Mat decodedMat = imdecode(rawData, IMREAD_ANYCOLOR);

            if (decodedMat.data == NULL) {
                cv::Mat f1(m_height, m_width, CV_8UC2, m_vbuf);
                f1.copyTo(m_frame);
            } else {
                decodedMat.copyTo(m_frame);
            }
            ret = true;
        }
    }

    return ret;
}

bool CvCaptureCAM_VX::retrieveFrame(int outputType, OutputArray out)
{
    if (m_format == (uint32_t)CV_FOURCC('Y', 'U', 'Y', '2')) {
        cvtColor(m_frame, out,  COLOR_YUV2BGRA_YUY2);
    } else {
        m_frame.copyTo(out);
    }
    return true;
}

double CvCaptureCAM_VX::getProperty(int property_id)  const
{
    switch (property_id) {
    case CV_CAP_PROP_FRAME_WIDTH:
        return m_width;
    case CV_CAP_PROP_FRAME_HEIGHT:
        return m_height;
    case CV_CAP_PROP_FPS:
        return (int)(1E6 / m_frameIntervalUs);
    case CV_CAP_PROP_FOURCC:
        return (double)m_format;
    }
    return 0;
}

bool CvCaptureCAM_VX::setFormat(uint16_t width, uint16_t height, uint32_t format)  {
    bool ret = false;

    if (m_fd == -1)
        return false;

    VS_FORMAT_USR_DATA *pData = (VS_FORMAT_USR_DATA *)calloc(1, sizeof(VS_FORMAT_USR_DATA));
    if (pData == NULL)
        return false;

    pData->width = width;
    pData->height = height;
    pData->fourcc = format;

    if (ioctl(m_fd, USB2_VIDEO_IOCTL_SET_FORMAT, pData) == 0) {
        ret = true;
        //get default frame interval for the new format
        if (m_format != format) {
            VS_FORMAT_DECS  vsFmtDesc;
            if (ioctl(m_fd, USB2_VIDEO_IOCTL_GET_FORMAT, &vsFmtDesc) == 0) {
                m_frameIntervalUs = vsFmtDesc.interval;
            }
        }
    }

    free(pData);
    uint32_t bsize = 0;

    if (ret) {
        if (0 != ioctl(m_fd, USB2_VIDEO_IOCTL_GET_BUFFER_SIZE, (void *)&bsize)) {

            return false;
        }
        if (m_vbuf == NULL) {
            if ((m_vbuf = (char *)calloc(1, bsize)) == NULL) {

                return false;
            }
        } else {
            if ((m_vbuf = (char *)realloc(m_vbuf, bsize)) == NULL) {
                return false;
            }
        }
    }

    m_width = width;
    m_height = height;
    m_format = format;
    m_bsize = bsize;

    return true;
}

bool CvCaptureCAM_VX::setFrameRate(int fps) {
    bool ret = false;

    if (fps == 0)
        return false;

    if (m_fd == -1)
        return false;

    uint32_t frameIntervalUs = 1E6 / fps - 1;

    if (ioctl(m_fd, USB2_VIDEO_IOCTL_SET_FRAME_INT, &frameIntervalUs) == 0) {
        ret = true;
        m_frameIntervalUs = frameIntervalUs;
    }
    return ret;
}


bool CvCaptureCAM_VX::setProperty(int property_id, double value)
{
    bool retval = false;
    int ival = cvRound(value);

    switch (property_id) {
        case CV_CAP_PROP_FRAME_WIDTH:
        case CV_CAP_PROP_FRAME_HEIGHT:
            {
                int width, height;
                if (property_id == CV_CAP_PROP_FRAME_WIDTH)
                {
                    width = ival;
                    height = width*3/4;
                }
                else {
                    height = ival;
                    width = height*4/3;
                }
                retval = setFormat(width, height, m_format);
            }
            break;
        case CAP_PROP_FOURCC:
            retval = setFormat(m_width, m_height, ival);
            break;
        case CV_CAP_PROP_FPS:
            retval = setFrameRate(ival);
            break;
      }

    return retval;
}

int CvCaptureCAM_VX::getCaptureDomain()
{
    return CAP_UVCVX;
}


Ptr<IVideoCapture> create_UVCVX_capture(int index)
{
     return  makePtr<CvCaptureCAM_VX>(index);
}

}
#endif /* __VXWORKS__ */
