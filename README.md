Visible Build Notes:

1. OpenCv 3.4.3 (https://github.com/opencv/opencv/releases/tag/3.4.3) is built in /usr/local/opencv-3.4.3/
2. boost is built in /usr/local/boost ( v 67 )
2. Visible app links OpenCv and its 3rd party libs statically. 
3. /external contains 3rd party libraries used:
    boost, Cinder, spdlog and Cimg are submodules. Cimg is not currently used
    imgui and nr_c304 are commited versions of imgui and numerical recipes 
4. svl, small vision library, is internal unit tested vision library. Moving forward OpenCV is used instead of or a replacement for lower level implementations. It is a requirement that svl operates in macos and ios. 
5. When building the debug version of libcinder, debug information for the zlib library needs to be enabled. This can be done by adding the option ZLIB_DEBUG=1 ( Xcode 9.4.1 on macOS 10.13.16 )

Submodules:

 505ba78783f339c38d2072a89bf719a03c01d490 externals/CImg (v.203-1-g505ba78)
 765fb86cc89f50638f4daa4fc7fe32976d4d1755 externals/Cinder-ImGui (remotes/origin/HEAD)
 39f80117c81159878363aa603d9ea31c2f3ef2a4 externals/ImGuizmo (heads/master)
 e1b4249cde063c76cf946d915da7341bc08de7ec externals/cinder (v0.9.0-2461-ge1b4249cd)
+0896ce27b2e8dfbc4ef81e882cf438458c17c86a externals/imgui_module (v1.62-728-g0896ce27)
 f2ac7d730cb8afd386d897a3900ed145c392c1d9 externals/spdlog (v1.1.0-48-gf2ac7d7)


