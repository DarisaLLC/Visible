//
//  visible_cli.hpp
//  Visible
//
//  Created by Arman Garakani on 12/22/18.
//

#ifndef visible_cli_h
#define visible_cli_h

/*
 // Get the Content file and Chapter / Serie from the command line arguments
 // args[0] = Run App Path
 // args[1] = content file full path
 // args[2] = Serie name
 // args[3] = content file extension
 // args[4] = content_type
 */

namespace visible_cli {
    

struct result
{
    std::string application_full_path;
    std::string content_file_full_path;
    std::string chapter_name;
    std::string extension_or_content_specifier;
    std::string content_type;
    
    friend inline ostream& operator<<(ostream& out, const result& m)
    {
        out << "{";
        out << m.application_full_path << std::endl;
        out << m.content_file_full_path << std::endl;
        out << m.chapter_name << std::endl;
        out << m.extension_or_content_specifier << std::endl;
        out << m.content_type<< std::endl;
        out << "}";
        return out;
    }
};

template <class Archive>
void serialize(Archive & ar, result & t)
{
    ar(
       cereal::make_nvp("Application Full Path", units_string(t.application_full_path)),
       cereal::make_nvp("Content File Full Path", units_string(t.content_file_full_path)),
       cereal::make_nvp("Chapter or Serie Name", units_string(t.chapter_name)),
       cereal::make_nvp("Extension or Content Specifier", units_string(t.extension_or_content_specifier)),
       cereal::make_nvp("Content Type", units_string(t.content_type)));
      
}


// To create a new json template
template <typename T>
void output_json(const std::string & title, const T & tt)
{
    cereal::JSONOutputArchive oar(std::cout);
    oar(cereal::make_nvp(title, tt));
}

    
}

#endif /* visible_cli_h */
