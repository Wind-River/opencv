#ifndef __FBOUTPUT_H
#define __FBOUTPUT_H

void startFbWrite(char* fbDevNameArg, unsigned char* pSrc, unsigned int srcStride, unsigned int srcWidth, unsigned int srcHeight) ;
struct displayInfoStruct {
    unsigned int screenWidth;
    unsigned int screenHeight;
    unsigned int screenNumChannels;
    unsigned int screenStride;
    unsigned int screenOffset;
    unsigned int screenNumBuffers;
    unsigned int drawOffsetX;
    unsigned int drawOffsetY;
    unsigned int drawWidth;
    unsigned int drawHeight;
    unsigned int imgOffsetX;
    unsigned int imgOffsetY;
    void** bufPtrs;
    void* frontBuf;
    int frontBufIndex;
    void* fbHandle;
    int fbDevFd;
};

struct displayInfoStruct* displayOpen(char* fbDevName);
STATUS displayWrite(struct displayInfoStruct* pDisplayHandle, unsigned char* pSrc, unsigned int srcStride, unsigned int srcWidth, unsigned int srcHeight);
void displayClose(struct displayInfoStruct* pDisplayInfo);
STATUS displayCenterImage(struct displayInfoStruct* pDisplayInfo, unsigned int imgWidth, unsigned int imgHeight);
STATUS displayDrawImg(struct displayInfoStruct* pDisplayInfo, unsigned char* pSrc, unsigned int srcStride);

#endif /* __FBOUTPUT_H */

