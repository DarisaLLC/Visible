#include "core/csv.hpp"
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string.hpp>


namespace spiritcsv
{
    
    Parser::Parser( std::ifstream& strm,
                   std::string const& escape,
                   std::string const& quote,
                   std::string const& separator,
                   bool no_header)
    : m_stream(strm)
    , m_escape(escape)
    , m_quote(quote)
    , m_separator(separator)
    , m_strict(false)
    , m_verbosity(0)
    , m_logstream(0)
    , m_no_header (no_header)
    , m_newline('\n')
    {
        initialize();
    }
    
    
    void Parser::initialize()
    {
        m_stream.seekg(0, std::ios::end);
        m_fileLength = m_stream.tellg();
        
        m_stream.seekg(0, std::ios::beg);
        detectNewlines();
        
        if (!m_separator.size())
        {
            m_stream.seekg(0, std::ios::beg);
            detectSeparator();
        }
        
        if (!m_escape.size())
        {
            m_stream.seekg(0, std::ios::beg);
            detectEscapes();
        }
        
        if (!m_quote.size())
        {
            m_stream.seekg(0, std::ios::beg);
            detectQuote();
        }
        
        try
        {
            m_grammer = boost::escaped_list_separator<char>(m_escape, m_separator, m_quote);
        }
        catch(const std::exception & ex)
        {
            std::ostringstream s;
            std::cerr << "Unable to initialize csv grammer: " << ex.what();
        }
        
        m_stream.seekg(0, std::ios::beg);
        
        if (! m_no_header)  parseHeaders();
        else parseRows ();
        
        m_stream.seekg(0, std::ios::beg);
    }
    
    void Parser::detectNewlines()
    {
        char newline = '\n';
        int newline_count = 0;
        int carriage_count = 0;
        
        if (m_fileLength < 0) return;
        for (unsigned idx = 0; idx < m_fileLength; ++idx)
        {
            char c = static_cast<char>(m_stream.get());
            if (c == '\n')
            {
                ++newline_count;
            }
            else if (c == '\r')
            {
                ++carriage_count;
            }
            
            // read at least 2000 bytes before testing
            if ((idx == static_cast<unsigned>(m_fileLength)-1) || (idx > 4000))
            {
                if (newline_count > carriage_count)
                {
                    if (m_logstream)
                        *m_logstream << "Using newline as endline";
                    break;
                }
                else if (carriage_count >= newline_count)
                {
                    m_newline = '\r';
                    if (m_logstream)
                        *m_logstream << "Using carriage return as endline";
                    break;
                }
            }
        }
    }
    
    void Parser::detectSeparator()
    {
        // get first line
        std::string csv_line;
        std::getline(m_stream, csv_line, m_newline);
        
        // if user has not passed a separator manually
        // then attempt to detect by reading first line
        std::string sep = boost::trim_copy(m_separator);
        if (sep.empty())
        {
            // default to ','
            m_separator = ",";
            int num_commas = std::count(csv_line.begin(), csv_line.end(), ',');
            // detect tabs
            int num_tabs = std::count(csv_line.begin(), csv_line.end(), '\t');
            if (num_tabs > 0)
            {
                if (num_tabs > num_commas)
                {
                    m_separator = "\t";
                    if (m_logstream)
                        *m_logstream << "auto detected 'tab' separator";
                }
            }
            else // pipes
            {
                int num_pipes = std::count(csv_line.begin(), csv_line.end(), '|');
                
                if (num_pipes > num_commas)
                {
                    m_separator = "|";
                    
                    if (m_logstream)
                        *m_logstream << "auto detected '|' separator";
                }
                else // semicolons
                {
                    int num_semicolons = std::count(csv_line.begin(), csv_line.end(), ';');
                    if (num_semicolons > num_commas)
                    {
                        m_separator = ";";
                        if (m_logstream)
                            *m_logstream << "auto detected ';' separator";
                    }
                }
            }
        }
    }
    
    void Parser::detectEscapes()
    {
        m_escape = boost::trim_copy(m_escape);
        if (m_escape.empty()) m_escape = "\\";
        
    }
    
    void Parser::detectQuote()
    {
        m_quote = boost::trim_copy(m_quote);
        if (m_quote.empty()) m_quote = "\"";
    }
    
    void Parser::parseHeaders()
    {
        // Our crappy algorithm for detecting headers:
        // Read the first line and put the csv-separated values into the headers as strings
        // Read the second line and put the csv-separated values into a list.
        // Walk each item of the second line and try casting it into a numeric type of some sort
        // if it succeeds, try casting the corresponding item in the headers list. If it *also*
        // successfully casts, our headers are real data, not headers.
        typedef boost::escaped_list_separator<char> escape_type;
        typedef boost::tokenizer< escape_type > Tokenizer;
        
        std::string first_line;
        
        std::getline(m_stream, first_line, m_newline);
        
        Tokenizer first(first_line, m_grammer);
        Tokenizer::iterator fbeg = first.begin();
        for (; fbeg != first.end(); ++fbeg)
        {
            std::string val = boost::trim_copy(*fbeg);
            m_headers.push_back(val);
        }
        
    }
    
    
    void Parser::parseRows()
    {
        // Our crappy algorithm for detecting headers:
        // Read the first line and put the csv-separated values into the headers as strings
        // Read the second line and put the csv-separated values into a list.
        // Walk each item of the second line and try casting it into a numeric type of some sort
        // if it succeeds, try casting the corresponding item in the headers list. If it *also*
        // successfully casts, our headers are real data, not headers.
        typedef boost::escaped_list_separator<char> escape_type;
        typedef boost::tokenizer< escape_type > Tokenizer;
        m_rows.resize (0);
        
        std::string first_line;
        while (std::getline(m_stream, first_line, m_newline))
        {
            std::getline(m_stream, first_line, m_newline);
            
            Tokenizer first(first_line, m_grammer);
            Tokenizer::iterator fbeg = first.begin();
            std::vector<std::string> columns;
            
            for (; fbeg != first.end(); ++fbeg)
            {
                std::string val = boost::trim_copy(*fbeg);
                columns.push_back(val);
            }
            m_rows.push_back(columns);
        }
    }
    
    
    std::shared_ptr<std::vector<entry_t>> spiritcsv::rankOutput (const std::string& str,
                                                      std::string const& escape,
                                                      std::string const& quote,
                                                      std::string const& separator,
                                                      bool no_header)
    {
        std::shared_ptr<std::vector<entry_t>> entries = std::shared_ptr<std::vector<entry_t>> (new std::vector<entry_t> );
        std::ifstream file(str.c_str(), std::ios_base::in|std::ios_base::binary);
        spiritcsv::Parser p(file, escape, quote, separator, no_header);
        
        auto rows = p.getRows ();
        for (const auto & ss : rows)
        {
            if (ss.size() == 3)
            {
                uint32_t rank = std::stoi(ss[0]);
                double score = std::stod(ss[1]);
                entry_t et = std::make_tuple(rank, score, ss[2]);
                entries->push_back (et);
            }
        }
        return entries;
    }
    
    
}

