Visible Build Notes:

1. OpenCv 3.4.3 (https://github.com/opencv/opencv/releases/tag/3.4.3) is built in /usr/local/opencv-3.4.3/
2. boost is built in /usr/local/boost ( v 67 )
2. Visible app links OpenCv and its 3rd party libs statically. 
3. /external contains 3rd party libraries used:
    boost, Cinder, spdlog and Cimg are submodules. Cimg is not currently used
    imgui and nr_c304 are commited versions of imgui and numerical recipes 
    gsl is installed using brew
4. svl, small vision library, is internal unit tested vision library. Moving forward OpenCV is used instead of or a replacement for lower level implementations. It is a requirement that svl operates in macos and ios. 
5. When building the debug version of libcinder, debug information for the zlib library needs to be enabled. This can be done by adding the option ZLIB_DEBUG=1 ( Xcode 9.4.1 on macOS 10.13.16 )

Submodules:

 [submodule "externals/spdlog"]
        path = externals/spdlog
        url = https://github.com/gabime/spdlog.git
[submodule "externals/cinder"]
        path = externals/cinder
        url = git://github.com/cinder/Cinder.git
[submodule "externals/CImg"]
        path = externals/CImg
        url = git://github.com/dtschump/CImg
[submodule "externals/Cinder-ImGui"]
        path = externals/Cinder-ImGui
        url = https://github.com/simongeilfus/Cinder-ImGui.git
[submodule "externals/imgui_module"]
        path = externals/imgui_module
        url = https://github.com/ocornut/imgui.git
[submodule "externals/ImGuizmo"]
        path = externals/ImGuizmo
        url = git@github.com:DarisaLLC/ImGuizmo.git
[submodule "externals/fmt"]
        path = externals/fmt
        url = https://github.com/fmtlib/fmt.git
        branch = master
[submodule "externals/albatross"]
        path = externals/albatross
        url = https://github.com/swift-nav/albatross.git


