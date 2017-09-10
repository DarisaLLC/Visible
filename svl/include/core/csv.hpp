#ifndef __CSV__
#define __CSV__


#include <boost/utility.hpp>
#include <boost/tokenizer.hpp>

// stl
#include <iosfwd>

#include <string>
#include <vector>
#include <stdexcept>
#include <string>
#include <sstream>
#include <iostream>
#include <fstream>
#include <tuple>

namespace spiritcsv
{
    
    class csv_error : public std::runtime_error
    {
    public:
        csv_error(std::string const& msg)
        : std::runtime_error(msg)
        {}
    };
    
    
    class Parser : boost::noncopyable
    {
    public:
        
        Parser( std::ifstream& strm,
               std::string const& escape="",
               std::string const& quote="",
               std::string const& separator="",
               bool no_header = true);
        
        void parse();
        
        inline void setLogStream(std::ostream& output) { m_logstream = &output; }
        inline void verbosity(unsigned level) { m_verbosity = level; }
        inline unsigned verbosity() const { return m_verbosity; }
        
        inline char getNewline() const { return m_newline; }
        inline std::ios::streampos getFileLength() const { return m_fileLength; }
        
        inline std::string const& getSeparator() const { return m_separator; }
        inline std::string const& getQuote() const { return m_quote; }
        inline std::string const& getEscape() const { return m_escape; }
        inline std::vector<std::string> const& getHeaders() const { return m_headers; }
        
        std::vector<std::vector<std::string>> const& getRows() const { return m_rows; }
        
        
    private:
        
        void initialize();
        void detectNewlines();
        void detectSeparator();
        void detectEscapes();
        void detectQuote();
        void parseHeaders();
        void parseRows();
        
        bool m_no_header;
        std::ifstream& m_stream;
        std::string m_escape;
        std::string m_separator;
        std::string m_quote;
        bool m_strict;
        unsigned m_verbosity;
        std::ostream* m_logstream;
        char m_newline;
        std::ios::streampos m_fileLength;
        std::vector<std::string> m_headers;
        std::vector<std::vector<std::string>> m_rows;
        boost::escaped_list_separator<char> m_grammer;
    };
    

    typedef std::tuple<uint32_t, double, std::string> entry_t;
    
    std::shared_ptr<std::vector<entry_t>> rankOutput (const std::string& str,
                                       std::string const& escape="",
                                       std::string const& quote="",
                                       std::string const& separator="",
                                                      bool no_header = true);

}


#endif


