include(FetchContent)


FetchContent_Declare(
  imgui
  GIT_REPOSITORY https://github.com/ocornut/imgui
  GIT_TAG origin/docking
)

FetchContent_MakeAvailable(imgui)


FetchContent_Declare(
  imgui_filebrowser
  GIT_REPOSITORY https://github.com/aiekick/ImGuiFileDialog
  GIT_TAG Lib_Only
)

FetchContent_MakeAvailable(imgui_filebrowser)
