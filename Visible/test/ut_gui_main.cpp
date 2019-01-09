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
using namespace std;


std::vector<double> acid = {39.1747, 39.2197, 39.126, 39.0549, 39.0818, 39.0655, 39.0342,
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
        timed_double_vec_t& data = ntrack.second;
        data.resize(acid.size());
        for (int tt = 0; tt < acid.size(); tt++){
            data[tt].second = acid[tt];
            data[tt].first.first = tt;
            data[tt].first.second = time_spec_t(tt / 1000.0);
        }
        tracks.clear();
        tracks.push_back(ntrack);
        
        
        ntrack.first = " First Derivative ";
        timed_double_vec_t& ddata = ntrack.second;
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
    
    namedTrackOfdouble_t ntrack;
    vectorOfnamedTrackOfdouble_t tracks, diffs_;
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
    
    void imGuiPlotLineTracks (const char* label, ImVec2 canvasSize,const vectorOfnamedTrackOfdouble_t& tracks){
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


CINDER_APP( VisibleTestApp, RendererGl( RendererGl::Options().msaa( 4 ) ), []( App::Settings *settings ) {
    settings->setWindowSize( TEST_APP_WIDTH, TEST_APP_HEIGHT );
} )


