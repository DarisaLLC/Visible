Visible Build Notes:

1. OpenCv 3.4.3 (https://github.com/opencv/opencv/releases/tag/3.4.3) is built in /usr/local/opencv-3.4.3/
2. Visible app links OpenCv and its 3rd party libs statically. 
3. /external contains 3rd party libraries used:
    boost, Cinder, spdlog and Cimg are submodules. Cimg is not currently used
    imgui and nr_c304 are commited versions of imgui and numerical recipes 
4. svl, small vision library, is internal unit tested vision library. Moving forward OpenCV is used instead of or a replacement for lower level implementations. It is a requirement that svl operates in macos and ios. 
5. When building the debug version of libcinder, debug information for the zlib library needs to be enabled. This can be done by adding the option ZLIB_DEBUG=1 ( Xcode 9.4.1 on macOS 10.13.16 )






# dev
cinder/include/boost is linked to /usr/local/include/boost 1.63.0 instead of a submodule link to boost
