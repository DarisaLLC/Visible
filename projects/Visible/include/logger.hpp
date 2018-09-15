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
#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/sinks/syslog_sink.h"
#include "spdlog/async.h"
#include "spdlog/sinks/basic_file_sink.h"

class vlogger : public svl::SingletonLight<vlogger>
{
public:
    vlogger () : m_std_out (spdlog::stdout_color_mt("console")),
    m_sys_ident ("Visible-SysLog"),
    m_syslog (spdlog::syslog_logger_mt("syslog", m_sys_ident, LOG_PID))
    {
        console()->set_level(spdlog::level::trace); // Set specific logger's log level
        spdlog::set_pattern("[%H:%M:%S:%e:%f %z] [%n] [%^---%L---%$] [thread %t] %v");
    }
    
    const  std::shared_ptr<spdlog::logger> console () {
        return m_std_out;
    }
    const  std::shared_ptr<spdlog::logger> syslog () {
          return m_syslog;
      }
              
    
private:
    std::shared_ptr<spdlog::logger> m_std_out;
    std::shared_ptr<spdlog::logger> m_syslog;
    std::string m_sys_ident;
};

#endif /* logger_h */
