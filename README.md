Visible Build Notes: Currently tested on M1. 

1. Visible has the following dependencies:
    Boost (Homebrew)
    OpenCv (Homebrew)
    sodlog (Homebrew)
    OpenImageIO (Homebrew)
    ffmpeg (Homebrew)
    gsl (Homebrew)
    fmt (Homebrew)
    Cinder (git submodule github/com/gmnamra/Cinder.git branch v0.9.3_imgui_87)
    ImGui (embedded in Cinder above )
    
2. To build: at the moment this is a manual step
    brew install dependencies
    git clone --recursive Visible repo
    build Cinder
    build Visible
    
Next Steps for build:
    1. Build script
    2. cmake 
    

Visible implements "Method and apparatus for acquisition, compression, and characterization of spatiotemporal signals"

Covered by U.S. Patents 9,001,884 and 7,672,369 and Patents in EU and Japan.
