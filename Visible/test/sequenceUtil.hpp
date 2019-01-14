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

struct UndoRedo
{
    UndoRedo();
    virtual ~UndoRedo();
    
    virtual void Undo()
    {
        if (mSubUndoRedo.empty())
            return;
        for (int i = int(mSubUndoRedo.size()) - 1; i >= 0; i--)
        {
            mSubUndoRedo[i]->Undo();
        }
    }
    virtual void Redo()
    {
        for (auto& undoRedo : mSubUndoRedo)
        {
            undoRedo->Redo();
        }
    }
    template <typename T> void AddSubUndoRedo(const T& subUndoRedo)
    {
        mSubUndoRedo.push_back(std::make_shared<T>(subUndoRedo));
    }
    void Discard() { mbDiscarded = true; }
    bool IsDiscarded() const { return mbDiscarded; }
protected:
    std::vector<std::shared_ptr<UndoRedo> > mSubUndoRedo;
    bool mbDiscarded;
};

struct UndoRedoHandler
{
    UndoRedoHandler() : mbProcessing(false), mCurrent(NULL) {}
    ~UndoRedoHandler()
    {
        Clear();
    }
    
    void Undo()
    {
        if (mUndos.empty())
            return;
        mbProcessing = true;
        mUndos.back()->Undo();
        mRedos.push_back(mUndos.back());
        mUndos.pop_back();
        mbProcessing = false;
    }
    
    void Redo()
    {
        if (mRedos.empty())
            return;
        mbProcessing = true;
        mRedos.back()->Redo();
        mUndos.push_back(mRedos.back());
        mRedos.pop_back();
        mbProcessing = false;
    }
    
    template <typename T> void AddUndo(const T &undoRedo)
    {
        if (undoRedo.IsDiscarded())
            return;
        if (mCurrent && &undoRedo != mCurrent)
            mCurrent->AddSubUndoRedo(undoRedo);
        else
            mUndos.push_back(std::make_shared<T>(undoRedo));
        mbProcessing = true;
        mRedos.clear();
        mbProcessing = false;
    }
    
    void Clear()
    {
        mbProcessing = true;
        mUndos.clear();
        mRedos.clear();
        mbProcessing = false;
    }
    
    bool mbProcessing;
    UndoRedo* mCurrent;
    //private:
    
    std::vector<std::shared_ptr<UndoRedo> > mUndos;
    std::vector<std::shared_ptr<UndoRedo> > mRedos;
};

extern UndoRedoHandler gUndoRedoHandler;

inline UndoRedo::UndoRedo() : mbDiscarded(false)
{
    if (!gUndoRedoHandler.mCurrent)
    {
        gUndoRedoHandler.mCurrent = this;
    }
}

inline UndoRedo::~UndoRedo()
{
    if (gUndoRedoHandler.mCurrent == this)
    {
        gUndoRedoHandler.mCurrent = NULL;
    }
}

template<typename T> struct URChange : public UndoRedo
{
    URChange(int index, T* (*GetElements)(int index), void(*Changed)(int index) = [](int index) {}) : GetElements(GetElements), mIndex(index), Changed(Changed)
    {
        if (gUndoRedoHandler.mbProcessing)
            return;
        
        mPreDo = *GetElements(mIndex);
    }
    virtual ~URChange()
    {
        if (gUndoRedoHandler.mbProcessing || mbDiscarded)
            return;
        
        if (*GetElements(mIndex) != mPreDo)
        {
            mPostDo = *GetElements(mIndex);
            gUndoRedoHandler.AddUndo(*this);
        }
        else
        {
            // TODO: should not be here unless asking for too much useless undo
        }
    }
    virtual void Undo()
    {
        *GetElements(mIndex) = mPreDo;
        Changed(mIndex);
        UndoRedo::Undo();
    }
    virtual void Redo()
    {
        UndoRedo::Redo();
        *GetElements(mIndex) = mPostDo;
        Changed(mIndex);
    }
    
    T mPreDo;
    T mPostDo;
    int mIndex;
    
    T* (*GetElements)(int index);
    void(*Changed)(int index);
};


struct URDummy : public UndoRedo
{
    URDummy() : UndoRedo()
    {
        if (gUndoRedoHandler.mbProcessing)
            return;
    }
    virtual ~URDummy()
    {
        if (gUndoRedoHandler.mbProcessing)
            return;
        
        gUndoRedoHandler.AddUndo(*this);
    }
    virtual void Undo()
    {
        UndoRedo::Undo();
    }
    virtual void Redo()
    {
        UndoRedo::Redo();
    }
};


template<typename T> struct URDel : public UndoRedo
{
    URDel(int index, std::vector<T>* (*GetElements)(), void(*OnDelete)(int index) = [](int index) {}, void(*OnNew)(int index) = [](int index) {}) : GetElements(GetElements), mIndex(index), OnDelete(OnDelete), OnNew(OnNew)
    {
        if (gUndoRedoHandler.mbProcessing)
            return;
        
        mDeletedElement = (*GetElements())[mIndex];
    }
    virtual ~URDel()
    {
        if (gUndoRedoHandler.mbProcessing || mbDiscarded)
            return;
        // add to handler
        gUndoRedoHandler.AddUndo(*this);
    }
    virtual void Undo()
    {
        GetElements()->insert(GetElements()->begin() + mIndex, mDeletedElement);
        OnNew(mIndex);
        UndoRedo::Undo();
    }
    virtual void Redo()
    {
        UndoRedo::Redo();
        OnDelete(mIndex);
        GetElements()->erase(GetElements()->begin() + mIndex);
    }
    
    T mDeletedElement;
    int mIndex;
    
    std::vector<T>* (*GetElements)();
    void(*OnDelete)(int index);
    void(*OnNew)(int index);
};

template<typename T> struct URAdd : public UndoRedo
{
    URAdd(int index, std::vector<T>* (*GetElements)(), void(*OnDelete)(int index)  = [](int index) {}, void(*OnNew)(int index) = [](int index) {}) : GetElements(GetElements), mIndex(index), OnDelete(OnDelete), OnNew(OnNew)
    {
    }
    virtual ~URAdd()
    {
        if (gUndoRedoHandler.mbProcessing || mbDiscarded)
            return;
        
        mAddedElement = (*GetElements())[mIndex];
        // add to handler
        gUndoRedoHandler.AddUndo(*this);
    }
    virtual void Undo()
    {
        OnDelete(mIndex);
        GetElements()->erase(GetElements()->begin() + mIndex);
        UndoRedo::Undo();
    }
    virtual void Redo()
    {
        UndoRedo::Redo();
        GetElements()->insert(GetElements()->begin() + mIndex, mAddedElement);
        OnNew(mIndex);
    }
    
    T mAddedElement;
    int mIndex;
    
    std::vector<T>* (*GetElements)();
    void(*OnDelete)(int index);
    void(*OnNew)(int index);
};

struct NodeGraphDelegate
{
    NodeGraphDelegate() : mSelectedNodeIndex(-1), mCategoriesCount(0), mCategories(0)
    {}
    
    int mSelectedNodeIndex;
    int mCategoriesCount;
    const char ** mCategories;
    
#if 0
    virtual void UpdateEvaluationList(const std::vector<size_t> nodeOrderList) = 0;
    virtual void AddLink(int InputIdx, int InputSlot, int OutputIdx, int OutputSlot) = 0;
    virtual void DelLink(int index, int slot) = 0;
    virtual unsigned int GetNodeTexture(size_t index) = 0;
    // A new node has been added in the graph. Do a push_back on your node array
    // add node for batch(loading graph)
    virtual void AddSingleNode(size_t type) = 0;
    // add  by user interface
    virtual void UserAddNode(size_t type) = 0;
    // node deleted
    virtual void UserDeleteNode(size_t index) = 0;
    virtual ImVec2 GetEvaluationSize(size_t index) = 0;
    virtual void DoForce() = 0;
    virtual void SetParamBlock(size_t index, const std::vector<unsigned char>& paramBlock) = 0;
    virtual void SetTimeSlot(size_t index, int frameStart, int frameEnd) = 0;
    virtual bool NodeHasUI(size_t nodeIndex) = 0;
    virtual bool NodeIsProcesing(size_t nodeIndex) = 0;
    virtual bool NodeIsCubemap(size_t nodeIndex) = 0;
    // clipboard
    virtual void CopyNodes(const std::vector<size_t> nodes) = 0;
    virtual void CutNodes(const std::vector<size_t> nodes) = 0;
    virtual void PasteNodes() = 0;
#endif
    
};

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

class trackUIContainer //: public NodeGraphDelegate
{
public:
    
 
    
    trackUIContainer(const duration_time_t& entire, const  std::shared_ptr<vectorOfnamedTrackOfdouble_t>& );
    
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
    
    int mFrameMin, mFrameMax;
    bool mbMouseDragging;
    std::vector<node_t> mNodes;
    
protected:
    void InitDefault(UInode& node);
};

struct MySequence : public ImSequencer::SequenceInterface
{
    MySequence(trackUIContainer &nodeGraphDelegate) : mNodeGraphDelegate(nodeGraphDelegate), setKeyFrameOrValue(FLT_MAX, FLT_MAX){}
    
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
            *start = &mNodeGraphDelegate.mNodes[index].mStartFrame;
        if (end)
            *end = &mNodeGraphDelegate.mNodes[index].mEndFrame;
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
   
    
    trackUIContainer &mNodeGraphDelegate;
    std::vector<bool> mbExpansions;
    std::vector<bool> mbVisible;
    ImVector<ImCurveEdit::EditPoint> mSelectedCurvePoints;
    int mLastCustomDrawIndex;
    ImVec2 setKeyFrameOrValue;
    ImVec2 getKeyFrameOrValue;
    float mCurveMin, mCurveMax;
    URChange<trackUIContainer::UInode> *undoRedoChange;
};


#endif /* sequenceUtil_hpp */
