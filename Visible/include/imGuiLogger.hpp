//
//  app_logger.hpp
//  Visible
//
//  Created by Arman Garakani on 10/15/18.
//

#ifndef app_logger_h
#define app_logger_h

#include "cinder/CinderImGui.h"
#include <mutex>

#include "spdlog/spdlog.h"
#include "spdlog/sinks/sink.h"
#include "spdlog/sinks/base_sink.h"
#include "spdlog/sinks/ostream_sink.h"
#include "spdlog/details/os.h"
#include "spdlog/fmt/fmt.h"
#include "spdlog/formatter.h"
#include "spdlog/details/log_msg.h"
#include "spdlog/sinks/null_sink.h"


using namespace spdlog;

class imGuiLog
{
public:
    ImGuiTextBuffer     Buf;
    ImGuiTextFilter     Filter;
    ImVector<int>       LineOffsets;        // Index to lines offset
    bool                ScrollToBottom;
    
    void    Clear()     { Buf.clear(); LineOffsets.clear(); }
    
    template <typename T>
    void operator <<(T const& value) {
        //std::cout << value;
        AddLog("%s", value);
    }
    
    void    AddLog(const char* fmt, ...) IM_FMTARGS(2)
    {
        int old_size = Buf.size();
        //Print to stdout
        va_list args;
        va_start(args, fmt);
        printf("[VLog] ");
        std::vprintf(fmt, args);
        va_end(args);
        
        //Print into log
        va_list args2;
        va_start(args2, fmt);
        Buf.appendfv(fmt, args2);
        va_end(args2);
        for (int new_size = Buf.size(); old_size < new_size; old_size++)
            if (Buf[old_size] == '\n')
                LineOffsets.push_back(old_size);
        ScrollToBottom = true;
    }
    
    void    Draw(const char* title, bool* p_opened = NULL)
    {
        ImGui::SetNextWindowSize(ImVec2(500,400), ImGuiCond_FirstUseEver);
        ImGui::Begin(title, p_opened);
        if (ImGui::Button("Clear")) Clear();
        ImGui::SameLine();
        bool copy = ImGui::Button("Copy");
        ImGui::SameLine();
        Filter.Draw("Filter", -100.0f);
        ImGui::Separator();
        ImGui::BeginChild("scrolling");
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0,1));
        if (copy) ImGui::LogToClipboard();
        
        if (Filter.IsActive())
        {
            const char* buf_begin = Buf.begin();
            const char* line = buf_begin;
            for (int line_no = 0; line != NULL; line_no++)
            {
                const char* line_end = (line_no < LineOffsets.Size) ? buf_begin + LineOffsets[line_no] : NULL;
                if (Filter.PassFilter(line, line_end))
                    ImGui::TextUnformatted(line, line_end);
                line = line_end && line_end[1] ? line_end + 1 : NULL;
            }
        }
        else
        {
            ImGui::TextUnformatted(Buf.begin());
        }
        
        if (ScrollToBottom)
            ImGui::SetScrollHere(1.0f);
        ScrollToBottom = false;
        ImGui::PopStyleVar();
        ImGui::EndChild();
        ImGui::End();
    }
};

namespace spdlog{
    namespace sinks
    {
        template<typename Mutex>
        class imGuiLogSink : public sinks::base_sink <Mutex> {
        public:
            imGuiLogSink(imGuiLog& c) : m_logger(c) {};
            
            // SPDLog sink interface
            
        protected:
            void sink_it_(const spdlog::details::log_msg& msg) override
            {
                
                // log_msg is a struct containing the log entry info like level, timestamp, thread id etc.
                // msg.raw contains pre formatted log
                
                // If needed (very likely but not mandatory), the sink formats the message before sending it to its final destination:
                fmt::memory_buffer formatted ;
                sink::formatter_->format(msg, formatted);
                auto str = fmt::to_string(formatted);
                m_logger.AddLog("%s", str.c_str());
            }
            
            void flush_() override
            {
                std::cout << std::flush;
            }
            
            
            
            
        private:
            imGuiLog& m_logger;
        };
     
        
    }
}


#endif /* app_logger_h */
