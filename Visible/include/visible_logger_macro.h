//
//  visible_logger_macro.h
//  Visible
//
//  Created by Arman Garakani on 12/23/18.
//

#ifndef visible_logger_macro_h
#define visible_logger_macro_h

#define VAPPLOG "VLog"
#define VAPPLOG_INFO(...) spdlog::get("VLog")->info(__VA_ARGS__)
#define VAPPLOG_TRACE(...) spdlog::get("VLog")->trace(__VA_ARGS__)
#define VAPPLOG_ERROR(...) spdlog::get("VLog")->error(__VA_ARGS__)
#define VAPPLOG_WARNING(...) spdlog::get("VLog")->warn(__VA_ARGS__)
#define VAPPLOG_NOTICE(...) spdlog::get("VLog")->notice(__VA_ARGS__)
#define VAPPLOG_SEPARATOR() VAPPLOG_INFO("-----------------------------")

#endif /* visible_logger_macro_h */
