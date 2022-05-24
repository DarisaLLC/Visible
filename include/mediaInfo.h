
#ifndef __GEN_MOVIE_INFO__
#define __GEN_MOVIE_INFO__

#include <iostream>
#include "core/pair.hpp"

using namespace std;

// Media spec supports ROIs that are sections of the root buffer. Sections are assumed to have same width
// and height that is equal to height / number of sections.

class mediaSpec
{
public:
    mediaSpec () { mValid = false; }
    mediaSpec (int width, int height, int sections, double duration, int64_t number_of_frames,
               const std::vector<std::string>& channel_names,
               const std::vector<std::string>& names = std::vector<std::string> ()):
    mSize(width,height), mSections(sections),
    mDuration(duration), mFrameCount(number_of_frames),
    mChannelNames (channel_names), mSectionNames(names){
        mSectionSize.first = width;
        mSectionSize.second = height / mSections;
        for (auto ss = 0; ss < mSections; ss++){
            mROIxRanges.emplace_back(0, width);
            auto y_anchor = ss * mSectionSize.second;
            mROIyRanges.emplace_back(y_anchor,y_anchor+mSectionSize.second);
        }
        mFps = number_of_frames / duration;
        if (names.size() != mSections){
            mSectionNames.resize(mSections);
            for (auto ss = 0; ss < mSections; ss++){
                std::string default_str = " " + svl::toString(ss) + " ";
                mSectionNames[ss] = default_str;
            }
        }
        mChannelCount = int(channel_names.size());
        mValid = true;
    }
    
    const bool& valid () { return mValid; }
    int32_t getSectionCount () const { return mSections; }
    const int32_t& getWidth () const { return mSize.first; }
    const int32_t& getHeight () const { return mSize.second; }
    const iPair& getSize () const { return mSize; }
    const iPair& getSectionSize () const { return mSectionSize; }
    const std::vector<iPair>& getROIxRanges () const { return mROIxRanges; }
    const std::vector<iPair>& getROIyRanges () const { return mROIyRanges; }
    const double duration () const { return mDuration; }
    const int64_t frameCount () const { return mFrameCount; }
    const float Fps () const { return mFps; }
    const std::vector<std::string>& names () const { return mSectionNames; }
    
    
    static bool test (int width, int height, int sections, double duration, int64_t count){
        std::vector<std::string> channel_names ({"Y"});
        mediaSpec sp (width, height, sections, duration, count, channel_names);
        bool ans = sp.getSectionCount() == sections;
        if (! ans) return false;
        ans = sp.getWidth() == width;
        if (! ans) return false;
        ans = sp.getHeight() == width;
        if (! ans) return false;
        ans = sp.getROIxRanges().size() == sections;
        if (! ans) return false;
        ans = sp.getROIyRanges().size() == sections;
        if (! ans) return false;
        return true;
    }
    
private:
    bool mValid;
    double mDuration;
    int64_t mFrameCount;
    int mChannelCount;
    iPair mSize;
    iPair mSectionSize;
    int mSections;
    float mFps;
    std::vector<iPair> mROIxRanges;
    std::vector<iPair> mROIyRanges;
    std::vector<std::string> mSectionNames;
    std::vector<std::string> mChannelNames;
};




#endif

