//
//  ut_gui_plot.cpp
//  Visible
//
//  Created by Arman Garakani on 1/9/19.
//

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdelete-non-virtual-dtor"
#pragma GCC diagnostic ignored "-Wunused-private-field"
#pragma GCC diagnostic ignored "-Wcomma"

#include "VisibleApp.h"
#include "imgui_plot.h"
#include "imgui_visible_widgets.hpp"

using namespace std;


std::vector<float> acid = {39.1747, 39.2197, 39.126, 39.0549, 39.0818, 39.0655, 39.0342,
    38.8791, 38.8527, 39.0099, 38.8608, 38.9188, 38.8499, 38.6693,
    38.2513, 37.9095, 37.3313, 36.765, 36.3621, 35.7261, 35.0656,
    34.2602, 33.2523, 32.3183, 31.6464, 31.0073, 29.8166, 29.3423,
    28.5223, 27.5152, 26.8191, 26.3114, 25.8164, 25.0818, 24.7631,
    24.6277, 24.8184, 25.443, 26.2479, 27.8759, 29.2094, 30.7956,
    32.3586, 33.6268, 35.1586, 35.9315, 36.808, 37.3002, 37.67, 37.9986,
    38.2788, 38.465, 38.5513, 38.6818, 38.8076, 38.9388, 38.9592,
    39.058, 39.1322, 39.0803, 39.1779, 39.1531, 39.1375, 39.1978,
    39.0379, 39.1231, 39.202, 39.1581, 39.1777, 39.2971, 39.2366,
    39.1555, 39.2822, 39.243, 39.1807, 39.1488, 39.2491, 39.265, 39.198,
    39.2855, 39.2595, 39.4274, 39.3258, 39.3162, 39.4143, 39.3034,
    39.2099, 39.2775, 39.5042, 39.1446, 39.188, 39.2006, 39.1799,
    39.4077, 39.2694, 39.1967, 39.2828, 39.2438, 39.2093, 39.2167,
    39.2749, 39.4703, 39.2846};


#define TEST_APP_WIDTH 1280
#define TEST_APP_HEIGHT 960

struct MultiplotData_F {
    std::vector<float> Y;
    std::vector<float> X;
};



class VisibleTestApp : public App, public gui_base
{
public:
    
    //   VisibleTestApp();
    //  ~VisibleTestApp();
    
    virtual void SetupGUIVariables() override;
    virtual void DrawGUI() override;
    virtual void QuitApp();
    
    void prepareSettings( Settings *settings );
    void setup()override;
    void mouseDown( MouseEvent event )override;
    void keyDown( KeyEvent event )override;
    
    void update()override;
    void draw()override;
    void resize()override;
    void displayChange();
    void update_log (const std::string& msg);
    
    bool shouldQuit();
    void draw_multi_plot ();
    void run_cardiac();
private:
    void generate_data();
    fs::path getLoggingDirectory ();
    std::vector<std::string> m_args;
    vec2                mSize;
    Font                mFont;
    std::string            mLog;
    
    Rectf                        mGlobalBounds;
    map<string, boost::any> mPlist;
    
    mutable std::unique_ptr<sequencedImageContext> mContext;
    mutable lif_browser::ref mBrowser;
    
    bool showLog = false;
    bool showHelp = false;
    bool showOverlay = false;
    int convergence = 0;
    std::string         mFileName;
    std::string         mFileExtension;
    lifIO::ContentType_t mContentType; // "" denotes canonical LIF file
    bool m_isIdLabLif = false;
    void setup_ui ();
//    vWidget vw;

    contractionLocator::contraction_t ctr;
    typedef vector<float>::iterator dItr_t;
    
    std::vector<float> fder, fder2;
    
    static constexpr size_t buf_size = 512;
    std::vector<float> x_data, y_data1, y_data2, y_data3;

    
    
};



void VisibleTestApp::run_cardiac (){
    fder.resize (acid.size());
    fder2.resize (acid.size());
    
    // Get contraction peak ( valley ) first
    auto min_iter = std::min_element(acid.begin(),acid.end());
    ctr.contraction_peak.first = std::distance(acid.begin(),min_iter);
    
    // Computer First Difference,
    adjacent_difference(acid.begin(),acid.end(), fder.begin());
    std::rotate(fder.begin(), fder.begin()+1, fder.end());
    fder.pop_back();
    std::transform(fder.begin(), fder.end(), fder2.begin(), [](double f)->double { return f * f; });
    // find first element greater than 0.1
    auto pos = find_if (fder2.begin(), fder2.end(),  [&](double const& elem){ return elem > 0.1; });
    
    ctr.contraction_start.first = std::distance(fder2.begin(),pos);
    auto max_accel = std::min_element(fder.begin()+ ctr.contraction_start.first ,fder.begin()+ctr.contraction_peak.first);
    ctr.contraction_max_acceleration.first = std::distance(fder.begin()+ ctr.contraction_start.first, max_accel);
    ctr.contraction_max_acceleration.first += ctr.contraction_start.first;
    auto max_relax = std::max_element(fder.begin()+ ctr.contraction_peak.first ,fder.end());
    ctr.relaxation_max_acceleration.first = std::distance(fder.begin()+ ctr.contraction_peak.first, max_relax);
    ctr.relaxation_max_acceleration.first += ctr.contraction_peak.first;
    
    // Initialize rpos to point to the element following the last occurance of a value greater than 0.1
    // If there is no such value, initialize rpos = to begin
    // If the last occurance is the last element, initialize this it to end
    dItr_t rpos = find_if (fder2.rbegin(), fder2.rend(),  [&](double const& elem){ return elem > 0.1; }).base();

    ctr.relaxation_end.first = std::distance (fder2.begin(), rpos);
    
    //    EXPECT_EQ(ctr.contraction_start.first,16);
    //    EXPECT_EQ(ctr.contraction_peak.first,35);
    //    EXPECT_EQ(ctr.contraction_max_acceleration.first,27);
    //    EXPECT_EQ(ctr.relaxation_max_acceleration.first,43);
    //    EXPECT_EQ(ctr.relaxation_end.first,52);
    
    
    
//    contractionProfile ca;
//    ca.run(acid);
    
}
using namespace ci;
using namespace ci::app;
using namespace std;

void VisibleTestApp::QuitApp(){
    ImGui::DestroyContext();
    // fg::ThreadsShouldStop = true;
    quit();
}

//void VisibleTestApp::prepareSettings( Settings *settings );

void VisibleTestApp::setup(){
    ui::initialize(ui::Options()
                   .window(getWindow())
                   .itemSpacing(vec2(6, 6)) //Spacing between widgets/lines
                   .itemInnerSpacing(vec2(10, 4)) //Spacing between elements of a composed widget
                   .color(ImGuiCol_Button, ImVec4(0.86f, 0.93f, 0.89f, 0.39f)) //Darken the close button
                   .color(ImGuiCol_Border, ImVec4(0.86f, 0.93f, 0.89f, 0.39f))
                   //  .color(ImGuiCol_TooltipBg, ImVec4(0.27f, 0.57f, 0.63f, 0.95f))
                   );
    
    setFrameRate( 60 );
    setWindowPos(getWindowSize()/4);
    getWindow()->setAlwaysOnTop();
  
    
}
void VisibleTestApp::mouseDown( MouseEvent event ){
    cinder::app::App::mouseDown(event);
}
void VisibleTestApp::keyDown( KeyEvent event ){
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

void VisibleTestApp::draw() {
    gl::clear( Color::gray( 0.5f ) );
    DrawGUI();
}


void VisibleTestApp::generate_data() {
    constexpr float sampling_freq = 44100;
    constexpr float freq = 500;
    x_data.resize(buf_size);
    y_data1.resize(buf_size);
    y_data2.resize(buf_size);
    y_data3.resize(buf_size);
    for (size_t i = 0; i < buf_size; ++i) {
        const float t = i / sampling_freq;
        x_data[i] = t;
        const float arg = 2 * M_PI * freq * t;
        y_data1[i] = sin(arg);
        y_data2[i] = y_data1[i] * -0.6 + sin(2 * arg) * 0.4;
        y_data3[i] = y_data2[i] * -0.6 + sin(3 * arg) * 0.4;
    }
}

void VisibleTestApp::SetupGUIVariables(){
    //Set up global preferences for the GUI
    //http://anttweakbar.sourceforge.net/doc/tools:anttweakbar:twbarparamsyntax
    GetGUI().DefineGlobal("iconalign=horizontal");
}

void VisibleTestApp::DrawGUI(){
    // Draw the menu bar
    vWidget v;
    v.draw(" ", nullptr);
    
    
    
    
    generate_data();
    draw_multi_plot();
    
    
}

bool VisibleTestApp::shouldQuit()
{
    return true;
}

void VisibleTestApp::setup_ui(){
    ui::initialize(ui::Options()
                   .itemSpacing(vec2(6, 6)) //Spacing between widgets/lines
                   .itemInnerSpacing(vec2(10, 4)) //Spacing between elements of a composed widget
                   .color(ImGuiCol_Button, ImVec4(0.86f, 0.93f, 0.89f, 0.39f)) //Darken the close button
                   .color(ImGuiCol_Border, ImVec4(0.86f, 0.93f, 0.89f, 0.39f))
                   //  .color(ImGuiCol_TooltipBg, ImVec4(0.27f, 0.57f, 0.63f, 0.95f))
                   );
    float dpiScale = 1.f;
    auto& style = ImGui::GetStyle();
    style.WindowBorderSize = 1.f * dpiScale;
    style.FrameBorderSize = 1.f * dpiScale;
    style.FrameRounding = 5.f * dpiScale;
    style.ScrollbarSize *= dpiScale;
    style.Colors[ImGuiCol_ScrollbarBg] = ImVec4( 1, 1, 1, 0.03f );
    style.Colors[ImGuiCol_WindowBg] = ImVec4( 0.129f, 0.137f, 0.11f, 1.f );
    
    style.AntiAliasedFill = false;
    style.AntiAliasedLines = false;
    setWindowPos(getWindowSize()/4);
}





void VisibleTestApp::draw_multi_plot() {
    const float* y_data[] = { y_data1.data(), y_data2.data(), y_data3.data() };
    static ImU32 colors[3] = { ImColor(0, 255, 0), ImColor(255, 0, 0), ImColor(0, 0, 255) };
    static uint32_t selection_start = 0, selection_length = 0;
    
    ImGui::Begin("Example plot", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
    // Draw first plot with multiple sources
    ImGui::PlotConfig conf;
    conf.values.xs = x_data.data();
    conf.values.count = buf_size;
    conf.values.ys_list = y_data; // use ys_list to draw several lines simultaneously
    conf.values.ys_count = 3;
    conf.values.colors = colors;
    conf.scale.min = -1;
    conf.scale.max = 1;
    conf.tooltip.show = true;
    conf.grid_x.show = true;
    conf.grid_x.size = 128;
    conf.grid_x.subticks = 4;
    conf.grid_y.show = true;
    conf.grid_y.size = 0.5f;
    conf.grid_y.subticks = 5;
    conf.selection.show = true;
    conf.selection.start = &selection_start;
    conf.selection.length = &selection_length;
    conf.frame_size = ImVec2(buf_size, 200);
    ImGui::Plot("plot1", conf);
    
    // Draw second plot with the selection
    // reset previous values
    conf.values.ys_list = nullptr;
    conf.selection.show = false;
    // set new ones
    conf.values.ys = y_data3.data();
    conf.values.offset = selection_start;
    conf.values.count = selection_length;
    conf.line_thickness = 2.f;
    ImGui::Plot("plot2", conf);
    
    ImGui::End();
}

void VisibleTestApp::update()
{
}

void VisibleTestApp::resize ()
{
    mSize = vec2( getWindowWidth(), getWindowHeight() / 12);
    if (mContext && mContext->is_valid()) mContext->resize ();
    
}


void prepareSettings( App::Settings *settings )
{
    settings->setHighDensityDisplayEnabled();
    settings->setResizable( true );
    settings->setWindowSize( 640, 480 );
}



#if 0
class VisibleTestApp : public App, public gui_base
{
public:
    
    //   VisibleRunApp();
    //  ~VisibleRunApp();
    
    virtual void SetupGUIVariables() override{
        //Set up global preferences for the GUI
        //http://anttweakbar.sourceforge.net/doc/tools:anttweakbar:twbarparamsyntax
        GetGUI().DefineGlobal("iconalign=horizontal");
        
    }
    virtual void DrawGUI() override{
        
        ui::SameLine(ui::GetWindowWidth() - 60); ui::Text("%4.1f FPS", getAverageFps());
        if (ImGui::BeginTabBar("blah"))
        {
            if (ImGui::BeginTabItem("Full"))
            {
                if(! tracks.empty())
                    imGuiPlotLineTracks(" imGui Plot Test ", ImVec2(getWindowWidth()/2,getWindowHeight()/2), tracks);
                ImGui::Text(" Length ");
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("Half"))
            {
                if(! diffs_.empty())
                    imGuiPlotLineTracks(" imGui Plot Test ", ImVec2(getWindowWidth()/2,getWindowHeight()/2), diffs_);
                ImGui::Text(" First Derivative ");
                ImGui::EndTabItem();
            }
            
            ImGui::EndTabBar();
        }
        
    }
    
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
        
        setFrameRate( 60 );
        setWindowPos(getWindowSize()/4);
        getWindow()->setAlwaysOnTop();
        run_cardiac();
        
        ntrack.first = " Length ";
       timedVecOfVals_t& data = ntrack.second;
        data.resize(acid.size());
        for (int tt = 0; tt < acid.size(); tt++){
            data[tt].second = acid[tt];
            data[tt].first.first = tt;
            data[tt].first.second = time_spec_t(tt / 1000.0);
        }
        tracks.clear();
        tracks.push_back(ntrack);
        
        
        ntrack.first = " First Derivative ";
       timedVecOfVals_t& ddata = ntrack.second;
        ddata.resize(fder2.size());
        for (int tt = 0; tt < fder2.size(); tt++){
            ddata[tt].second = fder2[tt];
            ddata[tt].first.first = tt;
            ddata[tt].first.second = time_spec_t(tt / 1000.0);
        }
        diffs_.clear();
        diffs_.push_back(ntrack);
        
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
    
    void draw()override{
        gl::clear( Color::gray( 0.5f ) );
        DrawGUI();
    }
    
    bool shouldQuit();
    
private:
    
    namedTrack_t ntrack;
    vecOfNamedTrack_t tracks, diffs_;
    bool closed = false;
    bool showLog = false;
    bool showHelp = false;
    bool showOverlay = false;
    
    contraction_analyzer::contraction ctr;
    typedef vector<double>::iterator dItr_t;
    
    std::vector<double> fder, fder2;
    
    void run_cardiac (){
        fder.resize (acid.size());
        fder2.resize (acid.size());
        
        // Get contraction peak ( valley ) first
        auto min_iter = std::min_element(acid.begin(),acid.end());
        ctr.contraction_peak.first = std::distance(acid.begin(),min_iter);
        
        // Computer First Difference,
        adjacent_difference(acid.begin(),acid.end(), fder.begin());
        std::rotate(fder.begin(), fder.begin()+1, fder.end());
        fder.pop_back();
        auto medianD = stl_utils::median1D<double>(7);
        fder = medianD.filter(fder);
        std::transform(fder.begin(), fder.end(), fder2.begin(), [](double f)->double { return f * f; });
        // find first element greater than 0.1
        auto pos = find_if (fder2.begin(), fder2.end(),    // range
                            std::bind2nd(greater<double>(),0.1));  // criterion
        
        ctr.contraction_start.first = std::distance(fder2.begin(),pos);
        auto max_accel = std::min_element(fder.begin()+ ctr.contraction_start.first ,fder.begin()+ctr.contraction_peak.first);
        ctr.contraction_max_acceleration.first = std::distance(fder.begin()+ ctr.contraction_start.first, max_accel);
        ctr.contraction_max_acceleration.first += ctr.contraction_start.first;
        auto max_relax = std::max_element(fder.begin()+ ctr.contraction_peak.first ,fder.end());
        ctr.relaxation_max_acceleration.first = std::distance(fder.begin()+ ctr.contraction_peak.first, max_relax);
        ctr.relaxation_max_acceleration.first += ctr.contraction_peak.first;
        
        // Initialize rpos to point to the element following the last occurance of a value greater than 0.1
        // If there is no such value, initialize rpos = to begin
        // If the last occurance is the last element, initialize this it to end
        dItr_t rpos = find_if (fder2.rbegin(), fder2.rend(),    // range
                               std::bind2nd(greater<double>(),0.1)).base();  // criterion
        ctr.relaxation_end.first = std::distance (fder2.begin(), rpos);
        
        //    EXPECT_EQ(ctr.contraction_start.first,16);
        //    EXPECT_EQ(ctr.contraction_peak.first,35);
        //    EXPECT_EQ(ctr.contraction_max_acceleration.first,27);
        //    EXPECT_EQ(ctr.relaxation_max_acceleration.first,43);
        //    EXPECT_EQ(ctr.relaxation_end.first,52);
        
        
        
        contraction_profile_analyzer ca;
        ca.run(acid);
        
    }
    
    void imGuiPlotLineTracks (const char* label, ImVec2 canvasSize,const vecOfNamedTrack_t& tracks){
        static std::vector<ImColor> palette = {
            ImColor{ 218, 124, 48 },
            ImColor{ 62, 150, 81 },
            ImColor{ 204, 37, 41 },
            ImColor{ 83, 81, 84 },
            ImColor{ 107, 76, 154 },
            ImColor{ 146, 36, 40 },
            ImColor{ 148, 139, 61 }
        };
        
        
        std::vector<void const*> metricsToDisplay;
        std::vector<const char*> deviceNames;
        std::vector<MultiplotData_F> userData;
        std::vector<ImColor> colors;
        size_t metricSize = 0;
        float maxValue = std::numeric_limits<float>::lowest();
        float minValue = std::numeric_limits<float>::max();
        size_t maxDomain = std::numeric_limits<size_t>::lowest();
        size_t minDomain = std::numeric_limits<size_t>::max();
        
        for(auto track : tracks){
            deviceNames.push_back(track.first.c_str());
            colors.push_back(palette[0]);
            std::vector<float> domain;
            std::vector<float> values;
            domainFromPairedTracks_D(track,domain,values);
            MultiplotData_F data;
            data.X = domain;
            data.Y = values;
            metricSize = domain.size();
            auto minItr = std::min_element(domain.begin(), domain.end());
            auto maxItr = std::max_element(domain.begin(), domain.end());
            auto minValItr = std::min_element(values.begin(), values.end());
            auto maxValItr = std::max_element(values.begin(), values.end());
            minValue = std::min(minValue, *minValItr);
            maxValue = std::max(maxValue, *maxValItr);
            minDomain = std::min(minDomain, (size_t)(*minItr));
            maxDomain = std::max(maxDomain, (size_t)(*maxItr));
            userData.emplace_back(data);
            //  userData.emplace_back(data); causes a crash why ?
        }
        for (size_t ui = 0; ui < userData.size(); ++ui) {
            metricsToDisplay.push_back(&(userData[ui]));
        }
        auto getterY = [](const void* hData, int idx) -> float {
            auto pData = reinterpret_cast<const MultiplotData_F*>(hData);
            float val = static_cast<const float*>(pData->Y.data())[idx];
            return val;
        };
        
        
        
        //        void PlotMultiLines(
        //                                     const char* label,
        //                                     int num_datas,
        //                                     const char** names,
        //                                     const ImColor* colors,
        //                                     float(*getter)(const void* data, int idx),
        //                                     const void * const * datas,
        //                                     int values_count,
        //                                     float scale_min,
        //                                     float scale_max,
        //                                     ImVec2 graph_size);
        ImGui::PlotMultiLines(label,
                              (int)userData.size(),
                              deviceNames.data(),
                              colors.data(),
                              getterY,
                              metricsToDisplay.data(),
                              (int)metricSize,
                              minValue,
                              maxValue * 1.05,
                              canvasSize);
    }
    
    
};
#endif


CINDER_APP( VisibleTestApp, RendererGl( RendererGl::Options().msaa( 4 ) ), []( App::Settings *settings ) {
    settings->setWindowSize( TEST_APP_WIDTH, TEST_APP_HEIGHT );
} )


