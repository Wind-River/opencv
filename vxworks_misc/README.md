OPENCV FOR VXWORKS
-------------------

There are two ways to build OpenCV

 I. With the VxWorks SDK for non-commercial https://labs.windriver.com/downloads/wrsdk.html

II. With  Wind River Workbench and Wind River VxWorks products


------------
I. Using The VxWorks SDK

NOTE: assuming vxsdk environment is set up

1. Clone the opencv customized for vxworks

2. Create a sysroot directory (this will be a directory for the opencv installation)

3. Create a directory for you opencv build

4. In the newly created opencv build directory  you have two options:

 a. Comand line:

     Note: All paths in the following command will have to be edited to match your environment


 $ cmake -DCMAKE_TOOLCHAIN_FILE=\<path-to-opencv-vx-sources-dir\>/platforms/vxworks/vxworks.cmake -DCMAKE_INSTALL_PREFIX=\<path-to-sysroot-directory\>/usr \<path-to-opencv-vx-sources-dir\>

 Enable python3 module build:
 cmake -DCMAKE_TOOLCHAIN_FILE=\<path-to-opencv-vx-sources-dir\>/platforms/vxworks/vxworks.cmake -DCMAKE_INSTALL_PREFIX=\<path-to-sysroot-directory\>/usr -DBUILD_opencv_python3=ON \<path-to-opencv-vx-sources-dir\>


 b. Run cmake-gui and make sure to set the following options as such:


    -DCMAKE_CROSSCOMPILING=TRUE
    -DCMAKE_SYSTEM_NAME=vxworks
    CMAKE_FIND_ROOT_PATH_MODE_INCLUDE=ONLY
    CMAKE_FIND_ROOT_PATH_MODE_LIBRARY=ONLY
    CMAKE_FIND_ROOT_PATH_MODE_PROGRAM=ONLY
    CMAKE_FIND_ROOT_PATH=\<path-to-sdk>
    CMAKE_INSTALL_PREFIX=\<path-of-your-choosing\>/sysroot/usr
    CMAKE_C_COMPILER=\<path-to-sdk\>/toolkit/bin/wr-cc
    CMAKE_CXX_COMPILER=\<path-to-sdk\>/toolkit/bin/wr-c++
    CMAKE_LINKER=\<path-to-sdk\>/toolkit/bin/wr-ld
    CMAKE_MAKE_PROGRAM=/usr/bin/make



Or instead of setting the above options, when clicking configure for the first time, select "Unix Makefiles" and go to Specify toolchain file for cross-compiling and load  <path-to-opencv-vx-sources-dir\>/platforms/vxworks/vxworks.cmake


Click Configure then Generate


5.Compile everything:

    $ make
    $ make install



The resulting binaries  should be installed in the sysroot directory.


NOTE: The vxworks toolchain (platforms/vxworks/vxworks.cmake) file disables many opencv modules,
if you want to experiment with them their respective entry from the toolchain file.

RTP build setup
---------------

1. VxWorks  SDK:

-  In opencv-vx/vxworks-misc there are a few code samples to try out
-  Go to the sdk directory and run:

    $ wr-c++ canny_image_sample.cpp -o canny.vxe -I<path-to-your-sysroot\>/sysroot/usr/include/opencv4 -L<path-to-your-sysroot\>/sysroot/usr/lib -lopencv_imgproc -lopencv_imgcodecs -lopencv_core

2. Workbench:

- Create a rtp project
- Link the resulted opencv lib with this rtp
- Add the fboutput directory located in vxworks_misc to the project
- Add the desired opencv code to the project

**Displaying the result on screen**
Note: Will not work in quemu, this will only work on machines that have support for graphics support on VxWorks

     cd opencv-vx/vxworks-misc/fboutput
     Note: depending on what on the hardware used and the initialized framebuffer the value of DEFAULT_FBDEV (inc cvvxdisplay.hpp) may need to change
    $ cmake -DCMAKE_TOOLCHAIN_FILE=\<path-to-toolchain-file\>/vxsdk.toolchain.cmake   -DCMAKE_CXX_FLAGS=-I<path-to-opencv-install-dirt\>/usr/include/opencv4  .
    $ make
    $ cd opencv-vx/vxworks-misc/
    $ wr-c++ canny_video_sample.cpp -o canny.vxe -I<path-to-your-sysroo>/sysroot/usr/include/opencv4  -L<path-to-your-sysroot>/sysroot/usr/lib  -lopencv_imgcodecs -lopencv_imgproc  -lopencv_core -lopencv_videoio -lopencv_highgui -lopencv_photo -lopencv_video -lopencv_imgcodecs -L<path-to-opencv-vx>/opencv-vx/vxworks_misc/fboutput/ -lcvvxdisplay

 **IMPORTANT:** There are a few differences in the vxWorks port:

 - Writing video files and capturing from video files:
    - only single channel(no sound) mjpeg encoded videos are supported
 - Writing and reading image files:
     - only jpeg and png images are supported
 - Displaying an image:
    - in vxWorks the image will be displayed on a framebuffer
    - the display will need to be initialized (cvVxInitDisplay()) before the display loop
    - the required display format is RGBA
    - before displaying an image with cvVxShow() convert it to RGBA format
    - the framebuffer solution is intel only
 - The above differences from standard opencv examples can be observed in the provided samples


**Multicore using TBB**

Using vxworks integrated TBB

- Build the VSB with UTILS_TBB

- The following options apply if TBB is present in the SDK
    - Add the following settings to the opencv cmake command line, and rebuild opencv:

        -DCMAKE_CXX_FLAGS="-ltbb"

        -DTBB_ENV_INCLUDE=\<path-to-sdk\>/toolkit/include/usr/h/public

        -DTBB_VER_FILE=\<path-to-sdk\>/toolkit/include/usr/h/publictbb/tbb_stddef.h

        -DTBB_ENV_LIB=\<path-to-sdk\>/toolkit/include/usr/lib/common/libtbb.so

