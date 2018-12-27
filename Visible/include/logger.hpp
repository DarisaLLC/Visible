//
//  logger.hpp
//  Visible
//
//  Created by Arman Garakani on 9/8/18.
//

#ifndef logger_h
#define logger_h

#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/trim_all.hpp>

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
    vlogger ()
    {}
    
    const  std::shared_ptr<spdlog::logger> console () {
        return spdlog::get("VLog");
    }
              
};


namespace logging
{
    using namespace spdlog;

   
/// Given a file path and a path component, return a new path created by appending the component to the path.
    std::string file_path_by_appending_component(const std::string& path, const std::string& component, bool is_directory);
 /// Given a file path and an extension, append the extension to the path.
    std::string file_path_by_appending_extension(const std::string& path, const std::string& extension);

  /// Create a timestamped `mktemp`-compatible template string using the current local time.
    std::string create_timestamped_template(const std::string& prefix, int wildcard_count = 8);
    /// Reserve a unique file name based on a base directory path and a `mktemp`-compatible template string.
    /// Returns the path of the file.
    std::string reserve_unique_file_name(const std::string& path, const std::string& template_string);
    

    inline std::string sanitizeFilenamePiece(std::string input) {
        namespace alg = boost::algorithm;
        auto acceptable_chars_pred =
        alg::is_alnum() || alg::is_digit() || alg::is_any_of("-_");
        auto virtual_spaces = !acceptable_chars_pred;
        /// This removes leading and trailing bad characters (anything
        /// that's not in the good characters list above) and reduces
        /// internal clumps of them to a single character replaced with _
        alg::trim_fill_if(input, "_", virtual_spaces);
        return input;
    }


}


#endif /* logger_h */
