#pragma once

#include "ImSequencer.h"

#include <math.h>
#include <vector>
#include <algorithm>
#include "ImCurveEdit.h"
#include "imgui_internal.h"
#include "async_tracks.h"

#include <cstddef>

using namespace std;

static inline ImVec2 operator-(const ImVec2& lhs, const ImVec2& rhs) { return ImVec2(lhs.x - rhs.x, lhs.y - rhs.y); }

class RampEdit : public ImCurveEdit::Delegate
{
    std::vector<std::vector<ImVec2>> mPts; // [3][8];
    std::vector<size_t> mPointCount;
    std::vector<std::string> mPlotNames;
    std::vector<unsigned int> mPlotColors;
    std::vector<bool> mbVisible;
    
    ImVec2 mMin;
    ImVec2 mMax;
    
public:
    RampEdit(size_t plot_count)
    {
        mPts.resize(plot_count);
        mPointCount.resize(plot_count);
        mbVisible.resize(plot_count);
        mPlotNames.resize(plot_count);
        mPlotColors.resize(plot_count);
        
        mMax = ImVec2(1.f, 1.f);
        mMin = ImVec2(0.f, 0.f);
    }

    // If at_index is -1, push_back, else if index is valid, load data at that index
    int load (const namedTrackOfdouble_t& track, unsigned int color, int at_index,  bool visible = true)
    {
        
        const timed_double_vec_t& ds = track.second;
        std::vector<float> mBuffer;
        std::vector<timed_double_t>::const_iterator reader = ds.begin ();
        while (reader++ != ds.end())mBuffer.push_back (reader->second);
        svl::norm_min_max (mBuffer.begin(), mBuffer.end(), true);
        
        std::vector<ImVec2> pts;
        pts.clear();
        reader = ds.begin ();
        std::vector<float>::const_iterator bItr = mBuffer.begin();
        while (reader++ != ds.end() && bItr++ != mBuffer.end() ){
            pts.emplace_back(reader->first.first, *bItr);
        }
        
        if (at_index >= 0 && at_index < mPts.size()){
            mPts[at_index] = pts;
            mbVisible[at_index] = visible;
            mPlotNames[at_index] = track.first;
            mPlotColors[at_index] = color;
            mPointCount[at_index] = pts.size();
            return at_index;
        }
        return -1;
    }
    
    const std::vector<size_t>& pointCount () { return mPointCount; }
    const std::vector<std::vector<ImVec2>>& points () { return mPts; }
    std::vector<bool>& visibles () { return mbVisible; }
    const std::vector<std::string>& names () { return mPlotNames; }
    const std::vector<unsigned int>& colors () { return mPlotColors; }
    
    
    
    size_t GetCurveCount()
    {
        return 3;
    }
    
    bool IsVisible(size_t curveIndex)
    {
        return mbVisible[curveIndex];
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

struct timeLineSequence : public ImSequencer::SequenceInterface
{
    // interface with sequencer
    
    virtual int64_t GetFrameMin() const {
        return mFrameMin;
    }
    virtual int64_t GetFrameMax() const {
        return mFrameMax;
    }
    virtual size_t GetItemCount() const { return myItems.size(); }
    
    virtual size_t GetItemTypeCount() const { return mSequencerItemTypeNames.size(); }
    virtual const char *GetItemTypeName(int typeIndex) const { return mSequencerItemTypeNames[typeIndex].c_str(); }
    virtual const char *GetItemLabel(int index) const
    {
        static char tmps[512];
        sprintf(tmps, "[%02d] %s", index, mSequencerItemTypeNames[myItems[index].mType].c_str());
        return tmps;
    }
    
    virtual void Get(int index, int** start, int** end, int *type, unsigned int *color)
    {
        MySequenceItem &item = myItems[index];
        if (color)
            *color = 0xFFAA8080; // same color for everyone, return color based on type
        if (start)
            *start = &item.mFrameStart;
        if (end)
            *end = &item.mFrameEnd;
        if (type)
            *type = item.mType;
    }
    virtual void Add(int type) { myItems.push_back(MySequenceItem{ type, 0, 10, false }); };
    virtual void Del(int index) { myItems.erase(myItems.begin() + index); }
    virtual void Duplicate(int index) { myItems.push_back(myItems[index]); }
    
    virtual size_t GetCustomHeight(int index) { return myItems[index].mExpanded ? 300 : 0; }
    
    // my datas
    timeLineSequence() : m_editable_plot_data(3), mFrameMin(0), mFrameMax(0) {}
    
    int mFrameMin, mFrameMax;
    std::vector<std::string> mSequencerItemTypeNames;
    std::vector<std::string> mPlotNames;
    
    struct MySequenceItem
    {
        int mType;
        int mFrameStart, mFrameEnd;
        bool mExpanded;
    };
    std::vector<MySequenceItem> myItems;
    RampEdit m_editable_plot_data;
    
    virtual void DoubleClick(int index) {
        if (myItems[index].mExpanded)
        {
            myItems[index].mExpanded = false;
            return;
        }
        for (auto& item : myItems)
            item.mExpanded = false;
        myItems[index].mExpanded = !myItems[index].mExpanded;
    }
    
    virtual void CustomDraw(int index, ImDrawList* draw_list, const ImRect& rc, const ImRect& legendRect, const ImRect& clippingRect, const ImRect& legendClippingRect)
    {
       
        m_editable_plot_data.SetMax(ImVec2(float(mFrameMax), 1.f));
        m_editable_plot_data.SetMin(ImVec2(float(mFrameMin), 0.f));
        
        draw_list->PushClipRect(legendClippingRect.Min, legendClippingRect.Max, true);
        for (int i = 0; i < mPlotNames.size(); i++)
        {
            ImVec2 pta(legendRect.Min.x + 30, legendRect.Min.y + i * 14.f);
            ImVec2 ptb(legendRect.Max.x, legendRect.Min.y + (i+1) * 14.f);
            draw_list->AddText(pta, m_editable_plot_data.visibles()[i]?0xFFFFFFFF:0x80FFFFFF, mPlotNames[i].c_str());
            if (ImRect(pta, ptb).Contains(ImGui::GetMousePos()) && ImGui::IsMouseClicked(0))
                m_editable_plot_data.visibles()[i] = !m_editable_plot_data.visibles()[i];
        }
        draw_list->PopClipRect();
        
        ImGui::SetCursorScreenPos(rc.Min);
        ImCurveEdit::Edit(m_editable_plot_data, rc.Max-rc.Min, 137 + index, &clippingRect);
    }
    
    virtual void CustomDrawCompact(int index, ImDrawList* draw_list, const ImRect& rc, const ImRect& clippingRect)
    {
        m_editable_plot_data.SetMax(ImVec2(float(mFrameMax), 1.f));
        m_editable_plot_data.SetMin(ImVec2(float(mFrameMin), 0.f));
        
        draw_list->PushClipRect(clippingRect.Min, clippingRect.Max, true);
        for (int i = 0; i < mPlotNames.size(); i++)
        {
            for (int j = 0; j < m_editable_plot_data.pointCount()[i]; j++)
            {
                float p = m_editable_plot_data.points()[i][j].x;
                if (p < myItems[index].mFrameStart || p > myItems[index].mFrameEnd)
                    continue;
                float r = (p - mFrameMin) / float(mFrameMax - mFrameMin);
                float x = ImLerp(rc.Min.x, rc.Max.x, r);
                draw_list->AddLine(ImVec2(x, rc.Min.y + 6), ImVec2(x, rc.Max.y - 4), 0xAA000000, 4.f);
            }
        }
        draw_list->PopClipRect();
    }
};
