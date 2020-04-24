/* includes */

#include <vxWorks.h>
#include <stdio.h>
#include <stdlib.h>
#include <ioLib.h>
#include <fbdev.h>
#include <math.h>
#include "fboutput.h"

LOCAL STATUS displayBlankScreen(struct displayInfoStruct* pDisplayInfo);
LOCAL STATUS displayBlankBuffer(struct displayInfoStruct* pDisplayInfo, int bufIndex);

struct displayInfoStruct* displayOpen(char* fbDevName) {
    STATUS status;
    FB_IOCTL_ARG arg;
    struct displayInfoStruct* pDisplayInfo;
    
    int fbDevFd = 0; 
    fbDevFd = open (fbDevName, O_RDWR, 0666);

    if (fbDevFd <= 0 ) {
        printf("Error opening %s.\n", fbDevName);
        return NULL;
    }
    
    pDisplayInfo = (struct displayInfoStruct*) malloc(sizeof( struct displayInfoStruct));
    pDisplayInfo->fbHandle = (FB_IOCTL_ARG*)malloc(sizeof(FB_IOCTL_ARG));
    status = ioctl (fbDevFd, FB_IOCTL_GET_FB_INFO, pDisplayInfo->fbHandle);
    if (status != OK) {
        printf("Problem geting FB info.\n");
        free (pDisplayInfo);
        return NULL;
    }
    
    FB_IOCTL_ARG* pFbIoctlArg = (FB_IOCTL_ARG*)(pDisplayInfo->fbHandle);
    
    pDisplayInfo->screenWidth = pFbIoctlArg->getFbInfo.width;
    pDisplayInfo->screenStride = pFbIoctlArg->getFbInfo.stride;
    pDisplayInfo->screenHeight = pFbIoctlArg->getFbInfo.height;
    pDisplayInfo->screenNumChannels = pFbIoctlArg->getFbInfo.bpp/8;
    pDisplayInfo->screenOffset = pDisplayInfo->screenStride * pDisplayInfo->screenHeight;
    pDisplayInfo->screenNumBuffers = pFbIoctlArg->getFbInfo.buffers;
    
    pDisplayInfo->bufPtrs = (void**)malloc(pDisplayInfo->screenNumBuffers * sizeof(void*));
    
    int i;
    for (i=0; i< pDisplayInfo->screenNumBuffers; i++) {
        pDisplayInfo->bufPtrs[i] = pFbIoctlArg->getFbInfo.pFirstFb + i*pDisplayInfo->screenOffset;
    }
    
    pDisplayInfo->frontBufIndex = 0;
    pDisplayInfo->drawOffsetX = 0;
    pDisplayInfo->drawOffsetY = 0;
    pDisplayInfo->drawWidth = 0;
    pDisplayInfo->drawHeight = 0;
    pDisplayInfo->imgOffsetX = 0;
    pDisplayInfo->imgOffsetY = 0;
    

    printf("Screen information.\n");
    printf("   Width:  %d\n",  pDisplayInfo->screenWidth);
    printf("   Height:  %d\n",  pDisplayInfo->screenHeight);
    printf("   Stride:  %d\n",  pDisplayInfo->screenStride);
    printf("   Channels:  %d\n",  pDisplayInfo->screenNumChannels);
    printf("   Buffers:  %d\n",  pDisplayInfo->screenNumBuffers);
    printf("   Pixel format = 0x%x\n", pFbIoctlArg->getFbInfo.pixelFormat.flags);
    
    displayBlankScreen(pDisplayInfo);
    
    return pDisplayInfo;
}

/* Prepare the parameters for centering an image.
 * This method can be called once for a series of same-size images that are written to the display.
 */
STATUS displayCenterImage(struct displayInfoStruct* pDisplayInfo, unsigned int imgWidth, unsigned int imgHeight) {
    pDisplayInfo->drawOffsetX = 0;
    pDisplayInfo->drawOffsetY = 0;
    pDisplayInfo->drawWidth = 0;
    pDisplayInfo->drawHeight = 0;
    pDisplayInfo->imgOffsetX = 0;
    pDisplayInfo->imgOffsetY = 0;
    if (imgWidth < pDisplayInfo->screenWidth) {
        pDisplayInfo->drawOffsetX = (unsigned int)((pDisplayInfo->screenWidth - imgWidth)/2);
        pDisplayInfo->drawWidth = imgWidth;
    }
    else {
        pDisplayInfo->imgOffsetX = (unsigned int)((imgWidth- pDisplayInfo->screenWidth)/2);
        pDisplayInfo->drawWidth = pDisplayInfo->screenWidth;
    }
    
    if (imgHeight < pDisplayInfo->screenHeight) {
        pDisplayInfo->drawOffsetY = (unsigned int)((pDisplayInfo->screenHeight - imgHeight)/2);
        pDisplayInfo->drawHeight = imgHeight;
    }
    else {
        pDisplayInfo->imgOffsetY = (unsigned int)((imgHeight- pDisplayInfo->screenHeight)/2);
        pDisplayInfo->drawHeight = pDisplayInfo->screenHeight;
    }
    
    int bufferIndex;
    for (bufferIndex = 0; bufferIndex < pDisplayInfo->screenNumBuffers; bufferIndex++) {
        displayBlankBuffer(pDisplayInfo, bufferIndex);
    }
    return OK;
}

/* Make the buffer at bufIndex all black. */
LOCAL STATUS displayBlankBuffer(struct displayInfoStruct* pDisplayInfo, int bufIndex) {
    unsigned char* screenRowIterator = pDisplayInfo->bufPtrs[bufIndex];
    
    int row;
    for (row = 0; row < pDisplayInfo->screenHeight; row++) {
        memset(screenRowIterator, 0, pDisplayInfo->screenWidth * pDisplayInfo->screenNumChannels);
        screenRowIterator += pDisplayInfo->screenStride;
    }
    return OK;
    
}

/* Blanks out the entire screen to black. */
LOCAL STATUS displayBlankScreen(struct displayInfoStruct* pDisplayInfo) {
    
    int drawBufIndex = pDisplayInfo->frontBufIndex;
    displayBlankBuffer(pDisplayInfo, drawBufIndex);
    
    FB_IOCTL_ARG setArg;
    
    setArg.setFb.pFb = pDisplayInfo->bufPtrs[drawBufIndex];
    setArg.setFb.when = 1;
    
    STATUS status  = ioctl (pDisplayInfo->fbDevFd, FB_IOCTL_SET_FB, &setArg); 
#ifdef DEBUG
    if (status != OK) {
        printf("Trouble setting frame.\n");
    }
#endif
    pDisplayInfo->frontBufIndex = drawBufIndex;
    return status;

}

/* Draws the image centered on the screen.   Inefficient if drawing same size images
 * repeatedly.  For drawing images of the same size repeatedly, call displayCenterImage
 * once, and then call displayDrawImg repeatedly. */
STATUS displayWrite(struct displayInfoStruct* pDisplayInfo, unsigned char* pSrc, unsigned int srcStride, unsigned int srcWidth, unsigned int srcHeight) {
    STATUS status;
    
    status = displayCenterImage(pDisplayInfo, srcWidth, srcHeight);
    if (status != OK) {
        printf("Problems centering image on display.\n");
        return status;
    }
                    
    status = displayDrawImg(pDisplayInfo, pSrc, srcStride);
    if (status != OK ) {
        printf("Problems displaying image.");
    }
    return status;
}

/* Display an image. The input must be in ARGB little-endian word form.  So when reading as bytes, it is BGRA.
 * The pDisplayInfo members drawWidth and drawHeight must be set to non-zero.  */
STATUS displayDrawImg(struct displayInfoStruct* pDisplayInfo, unsigned char* pSrc, unsigned int srcStride) {
    
    int row;
    
    unsigned char* currentFrontBuf = pDisplayInfo->bufPtrs[pDisplayInfo->frontBufIndex];
    int newFrontBufIndex = pDisplayInfo->frontBufIndex + 1;
    if (newFrontBufIndex >= pDisplayInfo->screenNumBuffers) {
        newFrontBufIndex = 0;
    }
    newFrontBufIndex = 0; /* ??  Can only display first buffer.  Cannot display other buffers. ?? */
    
    unsigned char* newFrontBuf = pDisplayInfo->bufPtrs[newFrontBufIndex];
    
    unsigned char* srcRowIterator = pSrc 
                    + pDisplayInfo->imgOffsetX *pDisplayInfo->screenNumChannels
                    + pDisplayInfo->imgOffsetY * srcStride;
    
    unsigned char* screenRowIterator = (unsigned char*)newFrontBuf
                    + pDisplayInfo->drawOffsetX * pDisplayInfo->screenNumChannels
                    + pDisplayInfo->drawOffsetY * pDisplayInfo->screenStride;
    for (row = 0; row < pDisplayInfo->drawHeight; row++) {
        memcpy(screenRowIterator, srcRowIterator, pDisplayInfo->drawWidth * pDisplayInfo->screenNumChannels); 
        screenRowIterator += pDisplayInfo->screenStride;
        srcRowIterator += srcStride;
    }
    
    FB_IOCTL_ARG setArg;
    
    setArg.setFb.pFb = newFrontBuf;
    setArg.setFb.when = 1;
    
    STATUS status  = ioctl (pDisplayInfo->fbDevFd, FB_IOCTL_SET_FB, &setArg); 
#ifdef DEBUG
    if (status != OK) {
        printf("Trouble setting frame.\n");
    }
#endif
    pDisplayInfo->frontBufIndex = newFrontBufIndex;
    return status;
    
}

void displayClose(struct displayInfoStruct* pDisplayInfo) {
    if (pDisplayInfo != NULL) {
        if (pDisplayInfo->bufPtrs != NULL) {
            free(pDisplayInfo->bufPtrs);
        }
        if (pDisplayInfo->fbHandle != NULL) {
            free(pDisplayInfo->fbHandle);
        }
        
        if (pDisplayInfo->fbDevFd > 0) {
            close(pDisplayInfo->fbDevFd);
        }
        free (pDisplayInfo);
    }
}


