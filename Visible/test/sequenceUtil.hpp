//
//  sequenceUtil.hpp
//  VisibleTestApp
//
//  Created by Arman Garakani on 1/11/19.
//

#ifndef sequenceUtil_hpp
#define sequenceUtil_hpp
#include "ImSequencer.h"
#include "ImCurveEdit.h"
#include "async_tracks.h"
#include <vector>
#include "imgui.h"
#include <stdio.h>
#include <string>


#if 0
struct Node
{
    int     mType;
    ImVec2  Pos, Size;
    size_t InputsCount, OutputsCount;
    bool mbSelected;
    Node() : mbSelected(false) {}
    Node(int type, const ImVec2& pos);
    
    ImVec2 GetInputSlotPos(int slot_no, float factor) const { return ImVec2(Pos.x*factor, Pos.y*factor + Size.y * ((float)slot_no + 1) / ((float)InputsCount + 1)); }
    ImVec2 GetOutputSlotPos(int slot_no, float factor) const { return ImVec2(Pos.x*factor + Size.x, Pos.y*factor + Size.y * ((float)slot_no + 1) / ((float)OutputsCount + 1)); }
};
#endif

struct Evaluation
{
    Evaluation();
    
    void Init();
    void Finish();
    
    
    void AddSingleEvaluation(size_t nodeType);
    void UserAddEvaluation(size_t nodeType);
    void UserDeleteEvaluation(size_t target);
    
    //
    size_t GetEvaluationImageDuration(size_t target);
    
    void SetEvaluationParameters(size_t target, const std::vector<unsigned char>& parameters);
    void AddEvaluationInput(size_t target, int slot, int source);
    void DelEvaluationInput(size_t target, int slot);
    void SetMouse(int target, float rx, float ry, bool lButDown, bool rButDown);
    void Clear();
    
    void SetStageLocalTime(size_t target, int localTime, bool updateDecoder);
    
private:
    // mouse
    float mRx;
    float mRy;
    bool mLButDown;
    bool mRButDown;
    
protected:
    std::map<std::string, unsigned int> mSynchronousTextureCache;

    
};



class trackUIContainer //: public NodeGraphDelegate
{
public:
    trackUIContainer():mFrameMin(0), mFrameMax(0),  mbMouseDragging(false), mIsSet(false) {}
    
    
    void set(const duration_time_t& entire, const  std::shared_ptr<vectorOfnamedTrackOfdouble_t>& );
    void setTimeSlot(size_t index, int frameStart, int frameEnd);
    void setTimeDuration(size_t index, int duration);
    void setTime(int time, bool updateDecoder);
    void clear ();
    
    struct UInode
    {
        std::string mName;
        size_t mType;
        std::vector<unsigned char> mParameters;
        unsigned int mRuntimeUniqueId;
        int mStartFrame, mEndFrame;
        
    //    Mat4x4 mParameterViewMatrix = Mat4x4::GetIdentity();
        
        bool operator != (const UInode& other) const
        {
            if (mType != other.mType)
                return true;
            if (mParameters != other.mParameters)
                return true;
            if (mRuntimeUniqueId != other.mRuntimeUniqueId)
                return true;
            if (mStartFrame != other.mStartFrame)
                return true;
            if (mEndFrame != other.mEndFrame)
                return true;
      
        //    if (mParameterViewMatrix != other.mParameterViewMatrix)
         //       return true;
            return false;
        }
    };
    typedef trackUIContainer::UInode node_t;
    
    std::weak_ptr<vectorOfnamedTrackOfdouble_t> m_weakRef;
    int mSelectedNodeIndex;
    int mFrameMin, mFrameMax;
    bool mbMouseDragging;
    std::vector<node_t> mNodes;
    bool mIsSet;
    
protected:
//    void InitDefault(UInode& node);
};

struct MySequence : public ImSequencer::SequenceInterface
{
    MySequence(trackUIContainer &nodeGraphDelegate) : m_uicontainer(nodeGraphDelegate), setKeyFrameOrValue(FLT_MAX, FLT_MAX){}
    
    void Clear()
    {
        mbExpansions.clear();
        mbVisible.clear();
        mLastCustomDrawIndex = -1;
        setKeyFrameOrValue.x = setKeyFrameOrValue.y = FLT_MAX;
        mCurveMin = 0.f;
        mCurveMax = 1.f;
    }
    virtual int GetFrameMin() const;// { return gNodeDelegate.mFrameMin; }
    virtual int GetFrameMax() const;// { return gNodeDelegate.mFrameMax; }
    
    virtual void BeginEdit(int index);
  
    virtual void EndEdit();
  
    
    virtual int GetItemCount() const;// { return (int)gEvaluation.GetStagesCount(); }
    
    virtual int GetItemTypeCount() const { return 0; }
    virtual const char *GetItemTypeName(int typeIndex) const { return NULL; }
    virtual const char *GetItemLabel(int index) const
    {
      //  size_t nodeType = gEvaluation.GetStageType(index);
      //  return gMetaNodes[nodeType].mName.c_str();
        return std::to_string(index).c_str();
    }
    
    virtual void Get(int index, int** start, int** end, int *type, unsigned int *color)
    {
      //  size_t nodeType = gEvaluation.GetStageType(index);
        
        if (color)
            *color = 0xFFAAAAAA; //gMetaNodes[nodeType].mHeaderColor;
        if (start)
            *start = &m_uicontainer.mNodes[index].mStartFrame;
        if (end)
            *end = &m_uicontainer.mNodes[index].mEndFrame;
        if (type)
            *type = 0; //int(nodeType);
    }
    virtual void Add(int type) { };
    virtual void Del(int index) { }
    virtual void Duplicate(int index) { }
    
    virtual size_t GetCustomHeight(int index)
    {
        if (index >= mbExpansions.size())
            return false;
        return mbExpansions[index]? 300 : 0;
    }
    virtual void DoubleClick(int index)
    {
        if (index >= mbExpansions.size())
            mbExpansions.resize(index + 1, false);
        if (mbExpansions[index])
        {
            mbExpansions[index] = false;
            return;
        }
        for (auto&& item : mbExpansions)
            item = false;
        mbExpansions[index] = !mbExpansions[index];
    }
    
    virtual void CustomDraw(int index, ImDrawList* draw_list, const ImRect& rc, const ImRect& legendRect, const ImRect& clippingRect, const ImRect& legendClippingRect);
   
    
    trackUIContainer &m_uicontainer;
    std::vector<bool> mbExpansions;
    std::vector<bool> mbVisible;
    ImVector<ImCurveEdit::EditPoint> mSelectedCurvePoints;
    int mLastCustomDrawIndex;
    ImVec2 setKeyFrameOrValue;
    ImVec2 getKeyFrameOrValue;
    float mCurveMin, mCurveMax;
//    URChange<trackUIContainer::UInode> *undoRedoChange;
};



#endif /* sequenceUtil_hpp */
