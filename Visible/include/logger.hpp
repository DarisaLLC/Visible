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
#define APPLOG "Log"
#define APPLOG_INFO(...) spdlog::get("Log")->info(__VA_ARGS__)
#define APPLOG_TRACE(...) spdlog::get("Log")->trace(__VA_ARGS__)
#define APPLOG_ERROR(...) spdlog::get("Log")->error(__VA_ARGS__)
#define APPLOG_WARNING(...) spdlog::get("Log")->warn(__VA_ARGS__)
#define APPLOG_NOTICE(...) spdlog::get("Log")->notice(__VA_ARGS__)
#define APPLOG_SEPARATOR() APPLOG_INFO("-----------------------------")
}


#endif /* logger_h */
