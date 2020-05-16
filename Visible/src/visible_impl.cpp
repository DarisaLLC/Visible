

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdelete-non-virtual-dtor"
#pragma GCC diagnostic ignored "-Wunused-private-field"
#pragma GCC diagnostic ignored "-Wcomma"

#include "opencv2/stitching.hpp"
#include "core/stl_utils.hpp"
#include "VisibleApp.h"

using namespace boost;


std::ostream& operator<<(std::ostream& std_stream, const tiny_media_info& t)
{
    if (! t.isImageFolder ())
        std_stream  << " -- General Movie Info -- " << std::endl;
    else
        std_stream  << " -- Image Folder Info -- " << std::endl;
    
    std_stream << "Dimensions:" << t.getWidth() << " x " << t.getHeight() << std::endl;
    std_stream << "Duration:  " << t.getDuration() << " seconds" << std::endl;
    std_stream << "Frames:    " << t.getNumFrames() << std::endl;
    std_stream << "Framerate: " << t.getFramerate() << std::endl;
    return std_stream;
}


// Utils for both apps
namespace {
    
    fs::path get_app_directory_exists (const fs::path&& user_app_support){
        auto platform = ci::app::Platform::get();
        auto home_path = platform->getHomeDirectory();
        fs::path app_support = home_path / user_app_support;
        
        bool success = exists (app_support);
        if (! success )
            success = fs::create_directories (app_support);
        if (success) return app_support;
        return fs::path ();
    }
    
}
bool VisibleAppControl::check_input (const string &filename){
    
    auto check_file_and_size = svl::io::check_file(filename);
    if(! check_file_and_size.first) return check_file_and_size.first;
    
    boost::filesystem::path bpath(filename);
    auto boost_file_and_size = svl::io::existsFile(bpath);
    if(! boost_file_and_size) return boost_file_and_size;
    
    auto bsize = boost::filesystem::file_size(bpath);
    return check_file_and_size.second == bsize;
}


fs::path  VisibleAppControl::get_visible_app_support_directory () { return get_app_directory_exists (fs::path(c_visible_app_support));}
fs::path  VisibleAppControl::get_runner_app_support_directory () { return get_app_directory_exists (fs::path(c_visible_runner_app_support));}
fs::path VisibleAppControl::get_visible_cache_directory () {
    // Create an invisible folder for storage
    auto visiblePath = getHomeDirectory()/c_visible_cache_folder_name;
    if (!fs::exists( visiblePath)) fs::create_directories(visiblePath);
    return visiblePath;
}

fs::path VisibleAppControl::make_result_cache_entry_for_content_file (const boost::filesystem::path& path){

    fs::path ret_path;
    if(! exists(path)) return ret_path;
    
    try{
        auto visiblePath = get_visible_cache_directory();
        
        // Create a directory for this lif file if it does not exist
        std::string filestem = path.stem().string();
        auto cachePath = visiblePath/filestem;
        if (!fs::exists( cachePath)) fs::create_directories(cachePath);
        ret_path = cachePath;
    }
    catch (const std::exception & ex)
    {
        fs::path null_path;
        std::cout << "Creating cache directories failed: " << ex.what() << std::endl;
        return null_path;
    }
    return ret_path;
}

bool  VisibleAppControl::make_result_cache_directory_for_lif (const boost::filesystem::path& path, const lif_browser::ref& lif_ref){
    
    if(! exists(path) || ! lif_ref) return false;
    
    fs::path cache_path = make_result_cache_entry_for_content_file(path);

    if (cache_path.empty()) return false;
    
    try{
        if(lif_ref){
        auto series = lif_ref->get_all_series ();
        
            for( vector<lif_serie_data>::const_iterator serieIt = series.begin(); serieIt != series.end(); ++serieIt ){
                auto serie_cache_path = cache_path/serieIt->name();
                if (!fs::exists( serie_cache_path)) fs::create_directories(serie_cache_path);
            }
        }
    }
    catch (const std::exception & ex)
    {
        std::cout << "Creating cache directories failed: " << ex.what() << std::endl;
        return false;
    }
    return true;
}

bool VisibleAppControl::setup_text_loggers (const fs::path app_support_dir, std::string id_name){
    
    // Check app support directory
    bool app_support_ok = exists(app_support_dir);
    if (! app_support_ok ) return app_support_ok;
    
    try{
        // get a temporary file name
        std::string logname =  logging::reserve_unique_file_name(app_support_dir.string(),
                                                                 logging::create_timestamped_template(id_name));
        
        // Setup APP LOG
        auto daily_file_sink = std::make_shared<logging::sinks::daily_file_sink_mt>(logname, 23, 59);
        auto console_sink = std::make_shared<spdlog::sinks::stdout_sink_mt>();
        console_sink->set_level(spdlog::level::warn);
        console_sink->set_pattern("[%H:%M:%S:%e:%f %z] [%n] [%^---%L---%$] [thread %t] %v");
        std::vector<spdlog::sink_ptr> sinks;
        sinks.push_back(daily_file_sink);
        sinks.push_back(console_sink);
        
        auto combined_logger = std::make_shared<spdlog::logger>("VLog", sinks.begin(),sinks.end());
        combined_logger->info("Daily Log File: " + logname);
        //register it if you need to access it globally
        spdlog::register_logger(combined_logger);
    }
    catch (const spdlog::spdlog_ex& ex)
    {
        std::cout << "Log initialization failed: " << ex.what() << std::endl;
        return false;
    }
    return app_support_ok;
}


bool VisibleAppControl::setup_loggers (const fs::path app_support_dir,  imGuiLog& visualLog, std::string id_name){
    using imgui_sink_mt = spdlog::sinks::imGuiLogSink<std::mutex> ;
    
    // Check app support directory
    bool app_support_ok = exists(app_support_dir);
    if (! app_support_ok ) return app_support_ok;
    
    try{
        // get a temporary file name
        std::string logname =  logging::reserve_unique_file_name(app_support_dir.string(),
                                                                 logging::create_timestamped_template(id_name));
        
        // Setup APP LOG
        auto daily_file_sink = std::make_shared<logging::sinks::daily_file_sink_mt>(logname, 23, 59);
        auto visual_sink = std::make_shared<imgui_sink_mt>(visualLog);
        auto console_sink = std::make_shared<spdlog::sinks::stdout_sink_mt>();
        console_sink->set_level(spdlog::level::warn);
        console_sink->set_pattern("[%H:%M:%S:%e:%f %z] [%n] [%^---%L---%$] [thread %t] %v");
        std::vector<spdlog::sink_ptr> sinks;
        sinks.push_back(daily_file_sink);
        sinks.push_back(visual_sink);
        sinks.push_back(console_sink);
        
        auto combined_logger = std::make_shared<spdlog::logger>("VLog", sinks.begin(),sinks.end());
        combined_logger->info("Daily Log File: " + logname);
        //register it if you need to access it globally
        spdlog::register_logger(combined_logger);
    }
    catch (const spdlog::spdlog_ex& ex)
    {
        std::cout << "Log initialization failed: " << ex.what() << std::endl;
        return false;
    }
    return app_support_ok;
}




guiContext::guiContext (ci::app::WindowRef& ww)
: mWindow (ww), m_valid (false), m_type (null_viewer)
{
    if (ww)
        ww->setUserData (this);
}
//
//guiContext::~guiContext ()
//{
//    // std::cout << " guiContext Dtor called " << std::endl;
//}

// u implementation does nothing
void guiContext::mouseDown( MouseEvent event ) {}
void guiContext::mouseMove( MouseEvent event ) {}
void guiContext::mouseUp( MouseEvent event ) {}
void guiContext::mouseDrag( MouseEvent event ) {}
void guiContext::keyDown( KeyEvent event ) {}

void guiContext::normalize_point (vec2& pos, const ivec2& size)
{
    pos.x = pos.x / size.x;
    pos.y = pos.y / size.y;
}


const std::string & guiContext::getName() const { return mName; }
void guiContext::setName (const std::string& name) { mName = name; }
guiContext::Type guiContext::context_type () const { return m_type; }
bool guiContext::is_context_type (const Type t) const { return m_type == t; }
bool guiContext::is_valid () const { return m_valid; }


///////////////  Main Viewer ////////////////////

mainContext::mainContext(ci::app::WindowRef& ww, const boost::filesystem::path& dp)
: guiContext (ww)
{
    m_valid = false;
    m_type = Type::null_viewer;
    
}



#pragma GCC diagnostic pop

