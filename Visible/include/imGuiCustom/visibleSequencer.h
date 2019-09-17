#pragma once

#include "ImSequencer.h"

#include <math.h>
#include <vector>
#include <algorithm>
#include "ImCurveEdit.h"
#include "imgui_internal.h"
#include "imgui_common.h"
#include "timed_value_containers.h"

#include <cstddef>

using namespace std;

/*
 Notes:
 TimeDataCollection supports ImCurveEdit::delegate interface ( from ImGuizmo + modifications ).
 Each time data is is represented by vectors of these attributes at index curveIndex :
    1. Plot Data vector<vector<vec2>>
    2. Plot data point counts
    3. Plot Names
    4. Plot Colors
    5. Plot Visibility
 
 To add a new plot:
    1. TimeDataCollection contains thier representation. It is typically added using load
    2. Load can either update an existing plot index or add one to the end by passing index of -1 ( it is pushed_back )
    3. Load is typically called in the update function and therefore it is time critical.
    4. Entries have to be added to maps of plot types and color
 
 */

/**
 Operator - for ImVec2

 @param lhs ImVec2
 @param rhs ImVec2
 @return lhs - rhs
 */
//static inline ImVec2 operator-(const ImVec2& lhs, const ImVec2& rhs) { return ImVec2(lhs.x - rhs.x, lhs.y - rhs.y); }


class TimeDataCollection : public ImCurveEdit::Delegate
{
    std::vector<std::vector<ImVec2>> mPts; // [editable plot data][8];
    std::vector<size_t> mPointCount;
    std::vector<std::string> mPlotNames;
    std::vector<unsigned int> mPlotColors;
    std::vector<bool> mbVisible;
    
    ImVec2 mMin;
    ImVec2 mMax;
    
public:
    TimeDataCollection(size_t plot_count)
    {
        mPts.resize(plot_count);
        mPointCount.resize(plot_count);
        mbVisible.resize(plot_count);
        mPlotNames.resize(plot_count);
        mPlotColors.resize(plot_count);
        
        mMax = ImVec2(1.f, 1.f);
        mMin = ImVec2(0.f, 0.f);
    }

    //
    /**
     load

     @param track track
     @param color color in 4 byte format i.e 0xfffb2141
     @param at_index If at_index is -1, push_back, else if index is valid, load data at that index
     @return return number of tracks
     */
    int load (const namedTrack_t& track, unsigned int color, int at_index)
    {
        
        const timedVecOfVals_t& ds = track.second;
        std::vector<float> mBuffer;
        timedVecOfVals_t::const_iterator reader = ds.begin();
        while (reader != ds.end()){
            mBuffer.push_back (reader->second);
            reader++;
        }
        svl::norm_min_max (mBuffer.begin(), mBuffer.end(), true);
        
        std::vector<ImVec2> pts;
        pts.clear();
        reader = ds.begin ();
        std::vector<float>::const_iterator bItr = mBuffer.begin();
        while (reader != ds.end() && bItr != mBuffer.end() ){
            pts.emplace_back(reader->first.first, *bItr);
            reader++;
            bItr++;
        }

        // Visibility state is set in the app.
        if (at_index >= 0 && at_index < mPts.size()){
            mPts[at_index] = pts;
            mPlotNames[at_index] = track.first;
            mPlotColors[at_index] = color;
            mPointCount[at_index] = pts.size();
            return at_index;
        }
        if (at_index == -1 ){
            mPts.push_back(pts);
            mPlotNames.push_back(track.first);
            mPlotColors.push_back(color);
            mPointCount.push_back(pts.size());
            mbVisible.push_back(false);
            return static_cast<int>(mPts.size()-1);
        }
        return -1;
    }
    
    const std::vector<size_t>& pointCount () { return mPointCount; }
    const size_t plotCount () { return mPlotColors.size(); }
    const std::vector<std::vector<ImVec2>>& points () { return mPts; }
    std::vector<bool>& visibles () { return mbVisible; }
    const std::vector<std::string>& names () { return mPlotNames; }
    const std::vector<unsigned int>& colors () { return mPlotColors; }
    
    
    //@todo document this
    size_t GetCurveCount()
    {
        return mPlotColors.size();
    }
    
    bool IsVisible(size_t curveIndex)
    {
        return mbVisible[curveIndex];
    }
    void ToggleVisible(size_t curveIndex)
    {
        mbVisible[curveIndex] = ! mbVisible[curveIndex];
    }
    size_t GetPointCount(size_t curveIndex)
    {
        return mPointCount[curveIndex];
    }
    
    uint32_t GetCurveColor(size_t curveIndex)
    {
        return mPlotColors[curveIndex];
    }
    const vector<ImVec2>& GetPoints(size_t curveIndex)
    {
        return mPts[curveIndex];
    }
    virtual ImCurveEdit::CurveType GetCurveType(size_t curveIndex) const { return ImCurveEdit::CurveSmooth; }
    virtual int EditPoint(size_t curveIndex, int pointIndex, ImVec2 value)
    {
        mPts[curveIndex][pointIndex] = ImVec2(value.x, value.y);
        SortValues(curveIndex);
        for (size_t i = 0; i < GetPointCount(curveIndex); i++)
        {
            if (mPts[curveIndex][i].x == value.x)
                return (int)i;
        }
        return pointIndex;
    }
    virtual void AddPoint(size_t curveIndex, ImVec2 value)
    {
        if (mPointCount[curveIndex] >= 8)
            return;
        mPts[curveIndex][mPointCount[curveIndex]++] = value;
        SortValues(curveIndex);
    }
    virtual ImVec2& GetMax() { return mMax; }
    virtual ImVec2& GetMin() { return mMin; }
    virtual void SetMax(const ImVec2& maxv) { mMax = maxv; }
    virtual void SetMin(const ImVec2& minv) { mMin = minv; }
    
    virtual unsigned int GetBackgroundColor() { return 0; }
 
private:
    void SortValues(size_t curveIndex)
    {
        auto b = std::begin(mPts[curveIndex]);
        auto e = std::begin(mPts[curveIndex]) + GetPointCount(curveIndex);
        std::sort(b, e, [](ImVec2 a, ImVec2 b) { return a.x < b.x; });
        
    }
};


/*
 @notes:
 SequenceInterface provides functionality of time / frame / duration editing and visualization
 timeLineSequence encapsulates operation of such sequencer with a TimeDataCollection.
 */

struct timeLineSequence : public ImSequencer::SequenceInterface
{
    
    int mFrameMin, mFrameMax;
    std::vector<std::string> mSequencerItemTypeNames;
    int mLastCustomDrawIndex;
    
    struct timeline_item
    {
        int mType;
        int mFrameStart, mFrameEnd;
        bool mExpanded;
    };
    std::vector<timeline_item> items;
    TimeDataCollection m_time_data;
    
    // interface with sequencer
    
    virtual int64_t GetFrameMin() const {
        return mFrameMin;
    }
    virtual int64_t GetFrameMax() const {
        return mFrameMax;
    }
    virtual size_t GetItemCount() const { return items.size(); }
    
    virtual size_t GetItemTypeCount() const { return mSequencerItemTypeNames.size(); }
    virtual const char *GetItemTypeName(int typeIndex) const { return mSequencerItemTypeNames[typeIndex].c_str(); }
    virtual const char *GetItemLabel(int index) const
    {
        static char tmps[512];
        sprintf(tmps, "[%02d] %s", index, mSequencerItemTypeNames[items[index].mType].c_str());
        return tmps;
    }
    
    virtual void Get(int index, int** start, int** end, int *type, unsigned int *color)
    {
        timeline_item &item = items[index];
        if (color)
            *color = 0xFFAA8080; // same color for everyone, return color based on type
        if (start)
            *start = &item.mFrameStart;
        if (end)
            *end = &item.mFrameEnd;
        if (type)
            *type = item.mType;
    }
    virtual void Add(int type) { items.push_back(timeline_item{ type, 0, 10, false }); };
    virtual void Del(int index) { items.erase(items.begin() + index); }
    virtual void Duplicate(int index) { items.push_back(items[index]); }
    
    virtual size_t GetCustomHeight(int index) { return items[index].mExpanded ? 300 : 0; }
    
    // my datas
    // @todo remove hard-wired 4 standing for Red, Green and PCI
    // @note where adding to the plots needs to accounted
    timeLineSequence() : m_time_data(4), mFrameMin(0), mFrameMax(0) {}

    
    virtual void DoubleClick(int index) {
        if (items[index].mExpanded)
        {
            items[index].mExpanded = false;
            return;
        }
        for (auto& item : items)
            item.mExpanded = false;
        items[index].mExpanded = !items[index].mExpanded;
    }
    
    virtual void CustomDraw(int index, ImDrawList* draw_list, const ImRect& rc, const ImRect& legendRect, const ImRect& clippingRect, const ImRect& legendClippingRect)
    {

        m_time_data.SetMax(ImVec2(float(mFrameMax), 1.f));
        m_time_data.SetMin(ImVec2(float(mFrameMin), 0.f));
        
        draw_list->PushClipRect(legendClippingRect.Min, legendClippingRect.Max, true);
        for (int i = 0; i <  m_time_data.plotCount(); i++)
        {
            ImVec2 pta(legendRect.Min.x + 30, legendRect.Min.y + i * 14.f);
            ImVec2 ptb(legendRect.Max.x, legendRect.Min.y + (i+1) * 14.f);
            draw_list->AddText(pta, m_time_data.visibles()[i]?0xFFFFFFFF:0x80FFFFFF, m_time_data.names()[i].c_str());
            if (ImRect(pta, ptb).Contains(ImGui::GetMousePos()) && ImGui::IsMouseClicked(0)){
                bool tmp = m_time_data.visibles()[i];
                m_time_data.visibles()[i] = !tmp;
            }
        }
        draw_list->PopClipRect();
        
        ImGui::SetCursorScreenPos(rc.Min);
        ImCurveEdit::Edit(m_time_data, rc.Max-rc.Min, 137 + index, &clippingRect);
    }
    
    virtual void CustomDrawCompact(int index, ImDrawList* draw_list, const ImRect& rc, const ImRect& clippingRect)
    {
        m_time_data.SetMax(ImVec2(float(mFrameMax), 1.f));
        m_time_data.SetMin(ImVec2(float(mFrameMin), 0.f));
        
        draw_list->PushClipRect(clippingRect.Min, clippingRect.Max, true);
        for (int i = 0; i < m_time_data.plotCount(); i++)
        {
            for (int j = 0; j < m_time_data.pointCount()[i]; j++)
            {
                float p = m_time_data.points()[i][j].x;
                if (p < items[index].mFrameStart || p > items[index].mFrameEnd)
                    continue;
                float r = (p - mFrameMin) / float(mFrameMax - mFrameMin);
                float x = ImLerp(rc.Min.x, rc.Max.x, r);
                draw_list->AddLine(ImVec2(x, rc.Min.y + 6), ImVec2(x, rc.Max.y - 4), 0xAA000000, 4.f);
            }
        }
        draw_list->PopClipRect();
    }
};
