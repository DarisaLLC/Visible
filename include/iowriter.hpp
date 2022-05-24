//
//  iowriter.hpp
//  Visible
//
//  Created by Arman Garakani on 5/30/19.
//

#ifndef iowriter_h
#define iowriter_h

#include <iostream>
#include <string>
#include "timed_types.h"
#include "core/core.hpp"
#include "vision/histo.h"
#include "vision/pixel_traits.h"
#include "vision/opencv_utils.hpp"
#include "core/stl_utils.hpp"
#include "core/stats.hpp"
#include "vision/localvariance.h"
#include "core/lineseg.hpp"
#include <boost/range/irange.hpp>

// @todo Move this
// For base classing
class ioWriterBase : public base_signaler
{
    virtual std::string getName () const { return "OCVImageWriterBase"; }
};


class ioImageWriter : public ioWriterBase
{
public:
    typedef std::function<std::string(const uint32_t& index, uint32_t field_width, char fill_char)> file_naming_fn_t;
    
    typedef void (writer_info_delegate) (int,bool);
    
    ioImageWriter(const std::string& image_name = "image",
                  const file_naming_fn_t namer = ioImageWriter::default_namer,
                  const std::string& file_sep = "/"):
    m_name(std::move(image_name)), m_sep(std::move(file_sep)),m_namer(namer)
    {
        // Signals we provide
        signal_write_info = createSignal<ioImageWriter::writer_info_delegate>();
    }
    
    typedef std::vector<roiWindow<P8U>> channel_images_t;
    typedef std::vector<cv::Mat> channel_mats_t;
    
    void operator()(const std::string& dir_fqfn, channel_images_t& channel)
    {
        m_fqfn = dir_fqfn;
        m_valid = boost::filesystem::exists( m_fqfn );
        m_valid = m_valid && boost::filesystem::is_directory( m_fqfn );
        if(!m_valid){
            if (signal_write_info && signal_write_info->num_slots() > 0)
                signal_write_info->operator()(0,true);
            return;
        }
        m_name = m_fqfn + m_sep +m_name;
        
        if (channel.empty()) return;
        int fw = 1 + std::log10(channel.size());
        std::lock_guard<std::mutex> guard (m_mutex);
        
        uint32_t count = 0;
        uint32_t total = static_cast<uint32_t>(channel.size());
        vector<roiWindow<P8U> >::const_iterator vitr = channel.begin();
        do
        {
            cv::Mat mat(vitr->height(), vitr->width(), CV_8UC(1), vitr->rowPointer(0), 512);
            std::string fn = m_name +  m_namer(count++, fw, '0') + ".png";
            cv::imwrite(fn, mat);
            // Signal to listeners
            if (signal_write_info && signal_write_info->num_slots() > 0)
                signal_write_info->operator()(count-1,count == total);
        }
        while (++vitr != channel.end());
    }
    
    void operator()(const std::string& dir_fqfn, channel_mats_t& channel)
    {
        m_fqfn = dir_fqfn;
        m_valid = boost::filesystem::exists( m_fqfn );
        m_valid = m_valid && boost::filesystem::is_directory( m_fqfn );
        if(!m_valid){
            if (signal_write_info && signal_write_info->num_slots() > 0)
                signal_write_info->operator()(0,true);
            return;
        }
        m_name = m_fqfn + m_sep +m_name;
        
        if (channel.empty()) return;
        int fw = 1 + std::log10(channel.size());
        std::lock_guard<std::mutex> guard (m_mutex);
        
        uint32_t count = 0;
        uint32_t total = static_cast<uint32_t>(channel.size());
        vector<cv::Mat>::const_iterator vitr = channel.begin();
        do
        {
            std::string fn = m_name +  m_namer(count++, fw, '0') + ".png";
            cv::imwrite(fn, *vitr);
            // Signal to listeners
            if (signal_write_info && signal_write_info->num_slots() > 0)
                signal_write_info->operator()(count-1,count == total);
        }
        while (++vitr != channel.end());
    }
    
    void operator()(const std::string& dir_fqfn, std::vector<std::vector<float>>& signals)
    {
        m_fqfn = dir_fqfn;
        m_valid = boost::filesystem::exists( m_fqfn );
        m_valid = m_valid && boost::filesystem::is_directory( m_fqfn );
        if(!m_valid){
            if (signal_write_info && signal_write_info->num_slots() > 0)
                signal_write_info->operator()(0,true);
            return;
        }
        m_name = m_fqfn + m_sep +m_name;
        
        if (signals.empty()) return;
        int fw = 1 + std::log10(signals.size());
        std::lock_guard<std::mutex> guard (m_mutex);
        
        uint32_t count = 0;
        uint32_t total = static_cast<uint32_t>(signals.size());
        vector<std::vector<float>>::const_iterator vitr = signals.begin();
        do
        {
            std::string fn = m_name +  m_namer(count++, fw, '0') + ".csv";
            stl_utils::save_csv(*vitr, fn);
            // Signal to listeners
            if (signal_write_info && signal_write_info->num_slots() > 0)
                signal_write_info->operator()(count-1,count == total);
        }
        while (++vitr != signals.end());
    }
    static std::string default_namer (const uint32_t& image_index, uint32_t field_width, char fill_char)
    {
        std::stringstream ss;
        std::string s;
        ss << std::setfill(fill_char) << std::setw(field_width) << image_index;
        s = ss.str();
        return s;
    }
    
    
protected:
    boost::signals2::signal<ioImageWriter::writer_info_delegate>* signal_write_info;
    
private:
    
    mutable std::mutex m_mutex;
    bool m_valid;
    std::string m_fqfn;
    std::string m_name;
    std::string m_sep;
    file_naming_fn_t  m_namer;
    
};


#endif /* iowriter_h */
