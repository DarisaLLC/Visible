#include <vector>
#include <string>
#include <memory>

#include "core/ExifTool.h"

using namespace std;

bool getExifTimes(const std::string& fqfn, std::vector<std::string>& times)
{
    std::shared_ptr<ExifTool> et (new ExifTool ("/usr/local/bin/exiftool"));
    std::shared_ptr<TagInfo> info (et->ImageInfo(fqfn.c_str()));
    
    if (! info ) return false;
    
    if (info) {
        times.clear();
        std::string original ("DateTimeOriginal");
        std::string date ("Date");
        for (const TagInfo* tag = &(*info); tag ; tag = tag->next)
        {
            std::string nstr = tag->name;
            if (nstr.find(original) != string::npos)
            {
                times.emplace_back(tag->value);
                return true;
            }
        }
        return false;
    }
    return false;
}
