//
//  ut_gui_main.cpp
//  Visible
//
//  Created by Arman Garakani on 1/3/19.
//

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdelete-non-virtual-dtor"
#pragma GCC diagnostic ignored "-Wunused-private-field"
#pragma GCC diagnostic ignored "-Wcomma"

#include "VisibleApp.h"
#include "imguivariouscontrols.h"
#include "sequenceUtil.hpp"
#include "imgui_internal.h"


#include "Resources.h"

using namespace std;

#ifndef IMGUI_DEFINE_MATH_OPERATORS

//static ImVec2 operator+(const ImVec2 &a, const ImVec2 &b) {
//    return ImVec2(a.x + b.x, a.y + b.y);
//}

static ImVec2 operator-(const ImVec2 &a, const ImVec2 &b) {
    return ImVec2(a.x - b.x, a.y - b.y);
}

//static ImVec2 operator*(const ImVec2 &a, const ImVec2 &b) {
//    return ImVec2(a.x * b.x, a.y * b.y);
//}
//
//static ImVec2 operator/(const ImVec2 &a, const ImVec2 &b) {
//    return ImVec2(a.x / b.x, a.y / b.y);
//}
//
//static ImVec2 operator*(const ImVec2 &a, const float b) {
//    return ImVec2(a.x * b, a.y * b);
//}
#endif

#define TEST_APP_WIDTH 1280
#define TEST_APP_HEIGHT 960
bool gbIsPlaying = false;
bool gPlayLoop = false;

void SetStyle()
{
    ImGuiStyle &st = ImGui::GetStyle();
    st.FrameBorderSize = 1.0f;
    st.FramePadding = ImVec2(4.0f,2.0f);
    st.ItemSpacing = ImVec2(8.0f,2.0f);
    st.WindowBorderSize = 1.0f;
 //   st.TabBorderSize = 1.0f;
    st.WindowRounding = 1.0f;
    st.ChildRounding = 1.0f;
    st.FrameRounding = 1.0f;
    st.ScrollbarRounding = 1.0f;
    st.GrabRounding = 1.0f;
  //  st.TabRounding = 1.0f;
    
    // Setup style
    ImVec4* colors = ImGui::GetStyle().Colors;
    colors[ImGuiCol_Text] = ImVec4(1.00f, 1.00f, 1.00f, 0.95f);
    colors[ImGuiCol_TextDisabled] = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
    colors[ImGuiCol_WindowBg] = ImVec4(0.13f, 0.12f, 0.12f, 1.00f);
    colors[ImGuiCol_ChildBg] = ImVec4(1.00f, 1.00f, 1.00f, 0.00f);
    colors[ImGuiCol_PopupBg] = ImVec4(0.05f, 0.05f, 0.05f, 0.94f);
    colors[ImGuiCol_Border] = ImVec4(0.53f, 0.53f, 0.53f, 0.46f);
    colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_FrameBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.85f);
    colors[ImGuiCol_FrameBgHovered] = ImVec4(0.22f, 0.22f, 0.22f, 0.40f);
    colors[ImGuiCol_FrameBgActive] = ImVec4(0.16f, 0.16f, 0.16f, 0.53f);
    colors[ImGuiCol_TitleBg] = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
    colors[ImGuiCol_TitleBgActive] = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
    colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.00f, 0.00f, 0.00f, 0.51f);
    colors[ImGuiCol_MenuBarBg] = ImVec4(0.12f, 0.12f, 0.12f, 1.00f);
    colors[ImGuiCol_ScrollbarBg] = ImVec4(0.02f, 0.02f, 0.02f, 0.53f);
    colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.31f, 0.31f, 0.31f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.41f, 0.41f, 0.41f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.48f, 0.48f, 0.48f, 1.00f);
    colors[ImGuiCol_CheckMark] = ImVec4(0.79f, 0.79f, 0.79f, 1.00f);
    colors[ImGuiCol_SliderGrab] = ImVec4(0.48f, 0.47f, 0.47f, 0.91f);
    colors[ImGuiCol_SliderGrabActive] = ImVec4(0.56f, 0.55f, 0.55f, 0.62f);
    colors[ImGuiCol_Button] = ImVec4(0.50f, 0.50f, 0.50f, 0.63f);
    colors[ImGuiCol_ButtonHovered] = ImVec4(0.67f, 0.67f, 0.68f, 0.63f);
    colors[ImGuiCol_ButtonActive] = ImVec4(0.26f, 0.26f, 0.26f, 0.63f);
    colors[ImGuiCol_Header] = ImVec4(0.54f, 0.54f, 0.54f, 0.58f);
    colors[ImGuiCol_HeaderHovered] = ImVec4(0.64f, 0.65f, 0.65f, 0.80f);
    colors[ImGuiCol_HeaderActive] = ImVec4(0.25f, 0.25f, 0.25f, 0.80f);
    colors[ImGuiCol_Separator] = ImVec4(0.58f, 0.58f, 0.58f, 0.50f);
    colors[ImGuiCol_SeparatorHovered] = ImVec4(0.81f, 0.81f, 0.81f, 0.64f);
    colors[ImGuiCol_SeparatorActive] = ImVec4(0.81f, 0.81f, 0.81f, 0.64f);
    colors[ImGuiCol_ResizeGrip] = ImVec4(0.87f, 0.87f, 0.87f, 0.53f);
    colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.87f, 0.87f, 0.87f, 0.74f);
    colors[ImGuiCol_ResizeGripActive] = ImVec4(0.87f, 0.87f, 0.87f, 0.74f);
 //   colors[ImGuiCol_Tab] = ImVec4(0.01f, 0.01f, 0.01f, 0.86f);
 //   colors[ImGuiCol_TabHovered] = ImVec4(0.29f, 0.29f, 0.79f, 1.00f);
 //   colors[ImGuiCol_TabActive] = ImVec4(0.31f, 0.31f, 0.91f, 1.00f);
 //   colors[ImGuiCol_TabUnfocused] = ImVec4(0.02f, 0.02f, 0.02f, 1.00f);
 //   colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.19f, 0.19f, 0.19f, 1.00f);
    //  colors[ImGuiCol_DockingPreview] = ImVec4(0.38f, 0.48f, 0.60f, 1.00f);
    //  colors[ImGuiCol_DockingEmptyBg] = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
    colors[ImGuiCol_PlotLines] = ImVec4(0.61f, 0.61f, 0.61f, 1.00f);
    colors[ImGuiCol_PlotLinesHovered] = ImVec4(0.68f, 0.68f, 0.68f, 1.00f);
    colors[ImGuiCol_PlotHistogram] = ImVec4(0.90f, 0.77f, 0.33f, 1.00f);
    colors[ImGuiCol_PlotHistogramHovered] = ImVec4(0.87f, 0.55f, 0.08f, 1.00f);
    colors[ImGuiCol_TextSelectedBg] = ImVec4(0.47f, 0.60f, 0.76f, 0.47f);
    colors[ImGuiCol_DragDropTarget] = ImVec4(0.58f, 0.58f, 0.58f, 0.90f);
    colors[ImGuiCol_NavHighlight] = ImVec4(0.60f, 0.60f, 0.60f, 1.00f);
    colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
    colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
    colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.35f);
}

//MySequence mySequence(gNodeDelegate);


class VisibleProtoApp : public App, public gui_base
{
public:
    
    //   VisibleRunApp();
    //  ~VisibleRunApp();
    
    
    virtual void QuitApp(){
        ImGui::DestroyContext();
        // fg::ThreadsShouldStop = true;
        quit();
    }
    
    void prepareSettings( Settings *settings );
    
    void setup()override{
        ui::initialize(ui::Options()
                       .window(getWindow())
                       .itemSpacing(vec2(6, 6)) //Spacing between widgets/lines
                       .itemInnerSpacing(vec2(10, 4)) //Spacing between elements of a composed widget
                       .color(ImGuiCol_Button, ImVec4(0.86f, 0.93f, 0.89f, 0.39f)) //Darken the close button
                       .color(ImGuiCol_Border, ImVec4(0.86f, 0.93f, 0.89f, 0.39f))
                       //  .color(ImGuiCol_TooltipBg, ImVec4(0.27f, 0.57f, 0.63f, 0.95f))
                       );
        SetStyle ();
        setFrameRate( 60 );
        setWindowPos(getWindowSize()/4);
        getWindow()->setAlwaysOnTop();
        
        
        
        
        
    }
    void mouseDown( MouseEvent event )override{
        cinder::app::App::mouseDown(event);
    }
    void keyDown( KeyEvent event )override{
        if( event.getChar() == 'f' ) {
            // Toggle full screen when the user presses the 'f' key.
            setFullScreen( ! isFullScreen() );
        }
        else if( event.getCode() == KeyEvent::KEY_ESCAPE ) {
            // Exit full screen, or quit the application, when the user presses the ESC key.
            if( isFullScreen() )
                setFullScreen( false );
            else
                quit();
        }
        else if(event.getCode() == 'd' ) {
            
        }
    }
    virtual void SetupGUIVariables() override{
        //Set up global preferences for the GUI
        //http://anttweakbar.sourceforge.net/doc/tools:anttweakbar:twbarparamsyntax
        GetGUI().DefineGlobal("iconalign=horizontal");
        mLoop = gl::Texture::create( loadImage( loadResource( IMAGE_PLOOP )));
        mNoLoop = gl::Texture::create( loadImage( loadResource( IMAGE_PNLOOP  )));
        
    }
    virtual void DrawGUI() override{
        ImGuiIO& io = ImGui::GetIO();
    //    ImGui::ScopedMainMenuBar menubar;
        ui::SameLine(ui::GetWindowWidth() - 60); ui::Text("%4.1f FPS", getAverageFps());
        
        ImVec2 rc = ImGui::GetItemRectSize();
        ImVec2 deltaHeight = ImVec2(0, rc.y);
        ImGui::SetNextWindowPos(deltaHeight);
        auto tmp = io.DisplaySize - deltaHeight;
        ImGui::SetNextWindowSize(tmp);
        //  int mSelectedNodeIndex = 0;
        int gEvaluationTime = 0;
     //   int mFrameMin = 0;
     //   int mFrameMax = 0;
        
        if (ImGui::Begin("Visible", 0, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                         ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar |ImGuiWindowFlags_NoScrollWithMouse |
                         ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoBringToFrontOnFocus))
        {
            if (ImGui::Begin("Timeline"))
            {
                int selectedEntry =  gNodeDelegate.mSelectedNodeIndex;
                static int firstFrame = 0;
                int currentTime = gEvaluationTime;
                
                ImGui::PushItemWidth(80);
                ImGui::PushID(200);
                ImGui::InputInt("", &gNodeDelegate.mFrameMin, 0, 0);
                ImGui::PopID();
                ImGui::SameLine();
                if (ImGui::Button("|<"))
                    currentTime = gNodeDelegate.mFrameMin;
                ImGui::SameLine();
                if (ImGui::Button("<"))
                    currentTime--;
                ImGui::SameLine();
                
                ImGui::PushID(201);
                if (ImGui::InputInt("", &currentTime, 0, 0, 0))
                {
                }
                ImGui::PopID();
                
                ImGui::SameLine();
                if (ImGui::Button(">"))
                    currentTime++;
                ImGui::SameLine();
                if (ImGui::Button(">|"))
                    currentTime = gNodeDelegate.mFrameMax;
                ImGui::SameLine();
                extern bool gbIsPlaying;
                if (ImGui::Button(gbIsPlaying ? "Stop" : "Play"))
                {
                    if (!gbIsPlaying)
                    {
                        currentTime = gNodeDelegate.mFrameMin;
                    }
                    gbIsPlaying = !gbIsPlaying;
                }
#if 1
            extern bool gPlayLoop;
                if(mNoLoop == nullptr){
                    mNoLoop = gl::Texture::create( loadImage( loadResource( IMAGE_PNLOOP  )));
                }
                if (mLoop == nullptr){
                      mLoop = gl::Texture::create( loadImage( loadResource( IMAGE_PLOOP )));
                }
                
            unsigned int playNoLoopTextureId = mNoLoop->getId();//  evaluation.GetTexture("Stock/PlayNoLoop.png");
            unsigned int playLoopTextureId = mLoop->getId(); //evaluation.GetTexture("Stock/PlayLoop.png");
            
            ImGui::SameLine();
            if (ImGui::ImageButton((ImTextureID)(uint64_t)(gPlayLoop ? playLoopTextureId : playNoLoopTextureId), ImVec2(16.f, 16.f)))
                gPlayLoop = !gPlayLoop;
            
            ImGui::SameLine();
            ImGui::PushID(202);
            ImGui::InputInt("", & gNodeDelegate.mFrameMax, 0, 0);
            ImGui::PopID();
            ImGui::SameLine();
            currentTime = ImMax(currentTime, 0);
            ImGui::SameLine(0, 40.f);
            if (ImGui::Button("Make Key") && selectedEntry != -1)
            {
         //       nodeGraphDelegate.MakeKey(currentTime, uint32_t(selectedEntry), 0);
            }
            
            ImGui::SameLine(0, 50.f);
            
            int setf = (mySequence.getKeyFrameOrValue.x<FLT_MAX)? int(mySequence.getKeyFrameOrValue.x):0;
            ImGui::PushID(203);
            if (ImGui::InputInt("", &setf, 0, 0))
            {
                mySequence.setKeyFrameOrValue.x = float(setf);
            }
            ImGui::SameLine();
            float setv = (mySequence.getKeyFrameOrValue.y < FLT_MAX) ? mySequence.getKeyFrameOrValue.y : 0.f;
            if (ImGui::InputFloat("Key", &setv))
            {
                mySequence.setKeyFrameOrValue.y = setv;
            }
            ImGui::PopID();
            ImGui::SameLine();
            int timeMask[2] = { 0,0 };
            if (selectedEntry != -1)
            {
                timeMask[0] = gNodeDelegate.mNodes[selectedEntry].mStartFrame;
                timeMask[1] = gNodeDelegate.mNodes[selectedEntry].mEndFrame;
            }
            ImGui::PushItemWidth(120);
            if (ImGui::InputInt2("Time Mask", timeMask) && selectedEntry != -1)
            {
              //  URChange<TileNodeEditGraphDelegate::ImogenNode> undoRedoChange(selectedEntry, [](int index) { return &gNodeDelegate.mNodes[index]; });
                timeMask[1] = ImMax(timeMask[1], timeMask[0]);
                timeMask[0] = ImMin(timeMask[1], timeMask[0]);
             //   gNodeDelegate.SetTimeSlot(selectedEntry, timeMask[0], timeMask[1]);
            }
            ImGui::PopItemWidth();
            ImGui::PopItemWidth();
            
                ImSequencer::Sequencer(&mySequence, &currentTime, NULL, &selectedEntry, &firstFrame, ImSequencer::SEQUENCER_EDIT_STARTEND | ImSequencer::SEQUENCER_CHANGE_FRAME);
            if (selectedEntry != -1)
            {
                gNodeDelegate.mSelectedNodeIndex = selectedEntry;
               // NodeGraphSelectNode(selectedEntry);
             //   auto& imoNode = gNodeDelegate.mNodes[selectedEntry];
            //    gEvaluation.SetStageLocalTime(selectedEntry, ImClamp(currentTime - imoNode.mStartFrame, 0, imoNode.mEndFrame - imoNode.mStartFrame), true);
            }
            if (currentTime != gEvaluationTime)
            {
                gEvaluationTime = currentTime;
           //     nodeGraphDelegate.SetTime(currentTime, true);
            //    nodeGraphDelegate.ApplyAnimation(currentTime);
            }
        }
            ImGui::End();

            
#endif
            if (ImGui::Begin("Nodes"))
            {
                ImGui::PushItemWidth(150);
                //    ImGui::InputText("Name", &material.mName);
                ImGui::SameLine();
                ImGui::PopItemWidth();
                ImGui::PushItemWidth(60);
                //  static int previewSize = 0;
                //ImGui::Combo("Preview size", &previewSize, "  128\0  256\0  512\0 1024\0 2048\0 4096\0");
                ImGui::SameLine();
                if (ImGui::Button("Do exports"))
                {
                    //         nodeGraphDelegate.DoForce();
                }
                ImGui::SameLine();
                if (ImGui::Button("Save Graph"))
                {
                }
                ImGui::SameLine();
                if (ImGui::Button("Delete Graph"))
                {
                    //     library.mMaterials.erase(library.mMaterials.begin() + selectedMaterial);
                    //   selectedMaterial = int(library.mMaterials.size()) - 1;
                    //    UpdateNewlySelectedGraph(nodeGraphDelegate, evaluation);
                }
                ImGui::PopItemWidth();
            }
            ImGui::End();
            
            if (ImGui::Begin("Shaders")){}
            ImGui::End();
            
            if (ImGui::Begin("Library"))
            {
                if (ImGui::Button("New Graph")){}
                ImGui::SameLine();
                if (ImGui::Button("Import")){}
            }
            ImGui::End();
            
            //   ImGui::SetWindowSize(ImVec2(300, 300));
            if (ImGui::Begin("Parameters")){}
            ImGui::End();
        }
        
        ImGui::End();
    }
    

    
    void draw()override{
        gl::clear( Color::gray( 0.5f ) );
        DrawGUI();
    }
    
    bool shouldQuit();
    
private:
    ci::gl::TextureRef    mNoLoop, mLoop;
    trackUIContainer gNodeDelegate;
    
    
};


CINDER_APP( VisibleProtoApp, RendererGl( RendererGl::Options().msaa( 4 ) ), []( App::Settings *settings ) {
    settings->setWindowSize( TEST_APP_WIDTH, TEST_APP_HEIGHT );
} )


