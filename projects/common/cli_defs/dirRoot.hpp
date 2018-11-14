#ifndef __DIR_ROOT__
#define __DIR_ROOT__


#include <boost/algorithm/string.hpp>


#include <random>
#include <iterator>
#include "core/sshist.hpp"
#include <vector>
#include <list>
#include <queue>
#include <fstream>
#include <iostream>
#include <mutex>
#include <string>
#include <iterator>
#include <algorithm>
#include <tuple>
#include <type_traits>
#include <boost/operators.hpp>
#include <ctime>
#include <boost/date_time/posix_time/posix_time.hpp>
#include "svl_exception.hpp"
#include <boost/lexical_cast.hpp>

#include <memory>
#include <functional>
#include "core/singleton.hpp"
#include "stl_utils.hpp"
#include "core/csv.hpp"
#include "core/core.hpp"


using namespace std;
using namespace boost;
using namespace svl;
using namespace stl_utils;

namespace fs=boost::filesystem;

// A simple organization for
// visit #
// image filename
// Score Redness
// Score Thickness
// Score Scales
// Score Size
// Skin Color
// comment
// redundant patient id
#define INPUT_FIELDS {"PATIENT_ID","FILENAME","IMAGE_URL","MASK_URL","VISIT","SCORE_REDNESS","SCORE_THICKNESS","SCORE_SCALES","SCORE_SIZE","SKIN_COLOR","COMMENTS"}




namespace cli_defs
{
    class dirRoot
    {
    public:
        
        // map unique loc/pat to a int value
        typedef std::size_t patient_idx_t;
        typedef std::pair<patient_idx_t, std::string> id2idx_t;
        typedef std::map<id2idx_t::second_type,id2idx_t::first_type> patient_id2idx_map_t;
        typedef std::map<id2idx_t::first_type,id2idx_t::second_type> patient_idx2id_map_t;
        
        typedef float redness_t;
        typedef float thickness_t;
        typedef float scaleness_t;
        typedef float sizeeval_t;
        typedef float skincolor_t;
        
        // Visit #, FILENAME "SCORE_REDNESS","SCORE_THICKNESS","SCORE_SCALES","SCORE_SIZE","SKIN_COLOR","COMMENTS" PatientID ( redundancy )
        // Enum values correspond to tuple indicesses
        
        enum eScore {score_redness = 2, score_thickness = score_redness+1, score_scales = score_thickness+1, score_size = score_scales + 1, skin_color_info = score_size+1 };
        typedef std::tuple<int32_t, std::string,redness_t, thickness_t, scaleness_t, sizeeval_t, skincolor_t, std::string,std::string> visit_data_t;
        
        typedef std::function<bool(visit_data_t&)> tuple_check_fn_t;
        typedef std::pair<patient_idx_t, visit_data_t> idx2visit_t;
        typedef std::vector<idx2visit_t::second_type> visits_t;
        typedef std::map<idx2visit_t::first_type,visits_t> patient_idx2visits_map_t;
        
        dirRoot (const std::string& root_fqfn, const std::vector<std::string>& fields = INPUT_FIELDS,const std::string& vf = "Visit 0") :
        m_root_fqfn (root_fqfn), m_visit_format (vf), m_root_path (root_fqfn), m_fields (fields)
        {
            m_patient_idx2id.clear();
            m_patient_id2idx.clear();
        }
        
        const dirRoot::patient_id2idx_map_t&   patient2idx () const { return  m_patient_id2idx; }
        const dirRoot::patient_idx2id_map_t&   idx2patient () const { return m_patient_idx2id; }
        const dirRoot::patient_idx2visits_map_t& idx2visits () const { return m_patient_idx2visits; }
        
        bool selfCheck ()
        {
            auto diff_rows = m_csv_rows - totalVisits();
            return patient2idx().size () == idx2patient().size () && idx2patient().size () == idx2visits().size() &&
            totalVisits() == m_total_patient_visits && idx2patient().size () == m_total_patients && diff_rows > 0 && diff_rows < 3;
        }
        
        const size_t totalUniquePatients () { return patient2idx().size (); }
        const size_t totalVisits ()
        {
            size_t tv = 0;
            for (auto& patvisits : idx2visits())
                tv += patvisits.second.size();
            return tv;
        }
        
        const fs::path root () const { return m_root_path; }
        
        // LP0053-1001_US08_023_Visit 4_Close-Up_221281
        int parse_filename_string (const std::string& str, std::string& pid) const
        {
            std::vector<std::string> parts = this->split(str, '_');
            bool ok = parts.size() >= 4;
            if (! ok) return -1;
            pid = parts[1] + "_" + parts[2];
            return parse_visit_string (parts[3]);
        }
        
        visit_data_t parse_csv_row (const std::string& csv_row, const char dlim = ',')
        {
            std::vector<std::string> row = this->split (csv_row, ',');
            return parse_filename_tokens (row);
        }
        
        const visits_t & getVisitById (const std::string& pid) const
        {
            if (std::find(m_pids.begin(), m_pids.end(), pid) == m_pids.end())
                return visits_t ();
            
            auto idx = m_patient_id2idx [pid];
            return m_patient_idx2visits [idx];
        }
        
        visit_data_t getVisitFromFileNameString (const std::string& str) const
        {
            
            std::string pid;
            int vn = parse_filename_string (str, pid);
            if ( vn >= 0 && ! pid.empty())
            {
                visits_t visits = getVisitById (pid);
                for (auto v : visits)
                {
                    if(std::get<0>(v) == vn)
                        return v;
                }
            }
            return visit_data_t ();
        }
        static bool visitHasRedScore (visit_data_t& v, float fval)
        {
            return svl::equal(std::get<2>(v), fval);
        }
        static bool visitHasThicknessScore (visit_data_t& v, float fval)
        {
            return svl::equal(std::get<3>(v), fval);
        }
        
        static bool visitHasScalenessScore (visit_data_t& v, float fval)
        {
            return svl::equal(std::get<4>(v), fval);
        }
        
        static bool visitHasSizeScore (visit_data_t& v, float fval)
        {
            return svl::equal(std::get<5>(v), fval);
        }
        
        
        
        // Parse file to generate a vector of < a vector of related visits >
        // Path organization encoded field names
        // filename may redundantly encode these as well.
        bool parse_build_file (const std::string& filename, const std::string& separator = "/")
        {
            size_t expected_count = m_fields.size();
            std::vector<std::vector<std::string>> rows;
            try
            {
                std::ifstream file(filename.c_str(), std::ios_base::in|std::ios_base::binary);
                spiritcsv::Parser p(file, "","", separator, false, '\r');
                rows = p.getRows ();
                m_csv_rows = rows.size();
            }
            catch (std::exception& ee)
            {
                std::cout << ee.what () << std::endl;
            }
            m_total_patients = 0;
            m_total_patient_visits = 0;
            
            bool ok = rows.size() > 1;
            if (!ok) return ok;
            std::vector<std::vector<std::string>>::const_iterator rowItr = rows.begin();
            rowItr++; // pass through header
            size_t cnt = 0;
            for (; rowItr != rows.end(); rowItr++, cnt++)
            {
                if (rowItr->size () == expected_count)
                    ok = add_filename (*rowItr);
                else if (rowItr->size () == 1)
                {
                    std::vector<std::string> row = this->split (rowItr->at(0), ',');
                    if (row.size() == expected_count)
                        ok = add_filename (row);
                }
                else
                {
                    stl_utils::Out(*rowItr);
                    std::cout << " now " << std::endl;
                }
            }
            return ok;
        }
        
        
        const size_t min_expected_count () { return m_fields.size() - 1; }
        
    private:
        size_t m_total_patient_visits;
        size_t m_total_patients;
        size_t m_csv_rows;
        
        std::string m_root_fqfn;
        std::string m_visit_format;
        mutable fs::path m_root_path;
        std::vector<std::string> m_fields;
        std::vector<std::string> m_pids;
        
        // Yellow pages
        mutable patient_id2idx_map_t    m_patient_id2idx;
        mutable patient_idx2id_map_t    m_patient_idx2id;
        mutable patient_idx2visits_map_t m_patient_idx2visits;
        
        std::vector<std::string> split(const std::string& s, char seperator) const
        {
            std::vector<std::string> output;
            
            std::string::size_type prev_pos = 0, pos = 0;
            
            while((pos = s.find(seperator, pos)) != std::string::npos)
            {
                std::string substring( s.substr(prev_pos, pos-prev_pos) );
                
                output.push_back(substring);
                
                prev_pos = ++pos;
            }
            
            output.push_back(s.substr(prev_pos, pos-prev_pos)); // Last word
            
            return output;
        }
        
        visit_data_t  parse_filename_tokens (const std::vector<std::string>& row)
        {
            size_t expected_count = min_expected_count ();
            
            assert(row.size() >= expected_count);
            std::string pid = row[0];
            int visit_number = parse_visit_string (row[4]);
            if (visit_number < 0) return visit_data_t ();
            // See note above
            float redscore = fromstr<float>(row[5]);
            float thickscore = fromstr<float>(row[6]);
            float scalescore = fromstr<float>(row[7]);
            float sizescore = fromstr<float>(row[8]);
            float skin_type = fromstr<float>(row[9]);
            return std::make_tuple(visit_number, row[1], redscore, thickscore, scalescore, sizescore, skin_type, row[10], pid);
        }
        
        bool add_filename (const std::vector<std::string>& row)
        {
            visit_data_t visitd = parse_filename_tokens (row);
            
            std::string& pid = std::get<8>(visitd);
            
            static size_t default_idx = 1234567890;
            auto ret = stl_utils::safe_get(m_patient_id2idx, pid, default_idx);
            if (ret == default_idx)
            {
                m_patient_id2idx [pid] =  m_patient_id2idx.size();
                m_patient_idx2id [ m_patient_id2idx [pid] ] = pid;
                m_pids.push_back(pid);
                m_total_patients += 1;
            }
            
            size_t idx = m_patient_id2idx [pid];
            m_patient_idx2visits[idx].push_back (visitd);
            m_total_patient_visits += 1;
            
            return true;
        }
        
        int parse_visit_string (const std::string& str) const
        {
            bool vv = str.size() == m_visit_format.size() && str.substr(0,4) == m_visit_format.substr(0,4);
            if (! vv) return -1;
            std::string vnstr = str.substr(str.size()-2, 2);
            int visit_number = -1;
            try
            {
                visit_number = std::stoi(vnstr);
            }
            catch(std::invalid_argument&)
            {
            }
            return visit_number;
            
        }
        
        
    };
    
}

#endif
