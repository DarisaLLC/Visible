//
//  logger.hpp
//  Visible
//
//  Created by Arman Garakani on 9/8/18.
//

#ifndef logger_h
#define logger_h


#include "core/singleton.hpp"
#include "spdlog/spdlog.h"
#include "spdlog/sinks/syslog_sink.h"


#include "spdlog/sinks/basic_file_sink.h"
#include "spdlog/sinks/daily_file_sink.h"
#include "spdlog/sinks/rotating_file_sink.h"
#include "spdlog/sinks/stdout_sinks.h"
#include "spdlog/sinks/stdout_color_sinks.h"

class vlogger : public svl::SingletonLight<vlogger>
{
public:
    vlogger () : m_sys_ident ("Visible-SysLog")
    {
        auto sink = std::make_shared<spdlog::sinks::stdout_sink_mt>();
        m_std_out = std::make_shared<spdlog::logger>("console",sink);
        console()->set_level(spdlog::level::trace); // Set specific logger's log level
        spdlog::set_pattern("[%H:%M:%S:%e:%f %z] [%n] [%^---%L---%$] [thread %t] %v");
        
    }
    
    const  std::shared_ptr<spdlog::logger> console () {
        return m_std_out;
    }
              
    
private:
    std::shared_ptr<spdlog::logger> m_std_out;

    std::string m_sys_ident;
};

#include "spdlog/sinks/dist_sink.h"

namespace spdlog
{
    namespace sinks
    {
        using platform_sink_mt = stdout_sink_mt;
        using platform_sink_st = stdout_sink_st;
    }
}

namespace logging
{
    using namespace spdlog;
    // return a distributed sink.
    inline std::shared_ptr<sinks::dist_sink_mt> get_mutable_logging_container()
    {
        static auto sink = std::make_shared<sinks::dist_sink_mt>();
        
        return sink;
    }
    
    static constexpr const char c_sync_directory[] = "realm-object-server";
    static constexpr const char c_utility_directory[] = "io.realm.object-server-utility";
    
   
/// Given a file path and a path component, return a new path created by appending the component to the path.
    std::string file_path_by_appending_component(const std::string& path, const std::string& component, bool is_directory);
 /// Given a file path and an extension, append the extension to the path.
    std::string file_path_by_appending_extension(const std::string& path, const std::string& extension);

  /// Create a timestamped `mktemp`-compatible template string using the current local time.
    std::string create_timestamped_template(const std::string& prefix, int wildcard_count = 8);
    /// Reserve a unique file name based on a base directory path and a `mktemp`-compatible template string.
    /// Returns the path of the file.
    std::string reserve_unique_file_name(const std::string& path, const std::string& template_string);
}


#endif /* logger_h */
