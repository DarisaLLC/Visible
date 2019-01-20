#pragma once

#include <cstddef>

struct ImDrawList;
struct ImRect;
namespace ImSequencer
{
    enum SEQUENCER_OPTIONS
    {
        SEQUENCER_EDIT_NONE = 0,
        SEQUENCER_EDIT_STARTEND = 1 << 1,
        SEQUENCER_CHANGE_FRAME = 1 << 3,
        SEQUENCER_ADD = 1 << 4,
        SEQUENCER_DEL = 1 << 5,
        SEQUENCER_COPYPASTE = 1 << 6,
        SEQUENCER_EDIT_ALL = SEQUENCER_EDIT_STARTEND | SEQUENCER_CHANGE_FRAME
    };

    struct SequenceInterface
    {
        bool focused = false;
        virtual int GetFrameMin() const = 0;
        virtual int GetFrameMax() const = 0;
        virtual int GetItemCount() const = 0;

        virtual void BeginEdit(int /*index*/) {}
        virtual void EndEdit() {}
        virtual int GetItemTypeCount() const { return 0; }
        virtual const char *GetItemTypeName(int /*typeIndex*/) const { return ""; }
        virtual const char *GetItemLabel(int /*index*/) const { return ""; }

        virtual void Get(int index, int** start, int** end, int *type, unsigned int *color) = 0;

        virtual void Add(int /*type*/) {}
        virtual void Del(int /*index*/) {}
        virtual void Duplicate(int /*index*/) {}

        virtual void Copy() {}
        virtual void Paste() {}

        virtual size_t GetCustomHeight(int /*index*/) { return 0; }
        virtual void DoubleClick(int /*index*/) {}
        virtual void CustomDraw(int /*index*/, ImDrawList* /*draw_list*/, const ImRect& /*rc*/, const ImRect& /*legendRect*/, const ImRect& /*clippingRect*/, const ImRect& /*legendClippingRect*/) {}
    };


    // return true if selection is made
    bool Sequencer(SequenceInterface *sequence, int *currentFrame, bool *expanded, int *selectedEntry, int *firstFrame, int sequenceOptions);

}

#if 0
struct MySequence : public ImSequencer::SequenceInterface
{
    MySequence(TileNodeEditGraphDelegate &nodeGraphDelegate) : mNodeGraphDelegate(nodeGraphDelegate), setKeyFrameOrValue(FLT_MAX, FLT_MAX){}
    
    void Clear()
    {
        mbExpansions.clear();
        mbVisible.clear();
        mLastCustomDrawIndex = -1;
        setKeyFrameOrValue.x = setKeyFrameOrValue.y = FLT_MAX;
        mCurveMin = 0.f;
        mCurveMax = 1.f;
    }
    virtual int GetFrameMin() const { return gNodeDelegate.mFrameMin; }
    virtual int GetFrameMax() const { return gNodeDelegate.mFrameMax; }
    
    virtual void BeginEdit(int index)
    {
        assert(undoRedoChange == nullptr);
        undoRedoChange = new URChange<TileNodeEditGraphDelegate::ImogenNode>(index, [](int index) { return &gNodeDelegate.mNodes[index]; });
    }
    virtual void EndEdit()
    {
        delete undoRedoChange;
        undoRedoChange = NULL;
    }
    
    virtual int GetItemCount() const { return (int)gEvaluation.GetStagesCount(); }
    
    virtual int GetItemTypeCount() const { return 0; }
    virtual const char *GetItemTypeName(int typeIndex) const { return NULL; }
    virtual const char *GetItemLabel(int index) const
    {
        size_t nodeType = gEvaluation.GetStageType(index);
        return gMetaNodes[nodeType].mName.c_str();
    }
    
    virtual void Get(int index, int** start, int** end, int *type, unsigned int *color)
    {
        size_t nodeType = gEvaluation.GetStageType(index);
        
        if (color)
            *color = gMetaNodes[nodeType].mHeaderColor;
        if (start)
            *start = &mNodeGraphDelegate.mNodes[index].mStartFrame;
        if (end)
            *end = &mNodeGraphDelegate.mNodes[index].mEndFrame;
        if (type)
            *type = int(nodeType);
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
        for (auto& item : mbExpansions)
            item = false;
        mbExpansions[index] = !mbExpansions[index];
    }
    
    virtual void CustomDraw(int index, ImDrawList* draw_list, const ImRect& rc, const ImRect& legendRect, const ImRect& clippingRect, const ImRect& legendClippingRect)
    {
        if (mLastCustomDrawIndex != index)
        {
            mLastCustomDrawIndex = index;
            mbVisible.clear();
        }
        
        ImVec2 curveMin(float(gNodeDelegate.mFrameMin), mCurveMin);
        ImVec2 curveMax(float(gNodeDelegate.mFrameMax), mCurveMax);
        AnimCurveEdit curveEdit(curveMin, curveMax, gNodeDelegate.mAnimTrack, mbVisible, index);
        mbVisible.resize(curveEdit.GetCurveCount(), true);
        draw_list->PushClipRect(legendClippingRect.Min, legendClippingRect.Max, true);
        for (int i = 0; i < curveEdit.GetCurveCount(); i++)
        {
            ImVec2 pta(legendRect.Min.x + 30, legendRect.Min.y + i * 14.f);
            ImVec2 ptb(legendRect.Max.x, legendRect.Min.y + (i + 1) * 14.f);
            draw_list->AddText(pta, mbVisible[i] ? 0xFFFFFFFF : 0x80FFFFFF, curveEdit.mLabels[i].c_str());
            if (ImRect(pta, ptb).Contains(ImGui::GetMousePos()) && ImGui::IsMouseClicked(0))
                mbVisible[i] = !mbVisible[i];
        }
        draw_list->PopClipRect();
        
        ImGui::SetCursorScreenPos(rc.Min);
        ImCurveEdit::Edit(curveEdit, rc.Max - rc.Min, 137 + index, &clippingRect, &mSelectedCurvePoints);
        mCurveMin = curveMin.y;
        mCurveMax = curveMax.y;
        
        getKeyFrameOrValue = ImVec2(FLT_MAX, FLT_MAX);
        
        if (focused || curveEdit.focused)
        {
            if (ImGui::IsKeyPressedMap(ImGuiKey_Delete))
            {
                curveEdit.DeletePoints(mSelectedCurvePoints);
            }
        }
        
        if (setKeyFrameOrValue.x < FLT_MAX || setKeyFrameOrValue.y < FLT_MAX)
        {
            curveEdit.BeginEdit(0);
            for (int i = 0; i < mSelectedCurvePoints.size(); i++)
            {
                auto& selPoint = mSelectedCurvePoints[i];
                int keyIndex = selPoint.pointIndex;
                ImVec2 keyValue = curveEdit.mPts[selPoint.curveIndex][keyIndex];
                uint32_t parameterIndex = curveEdit.mParameterIndex[selPoint.curveIndex];
                AnimTrack* animTrack = gNodeDelegate.GetAnimTrack(gNodeDelegate.mSelectedNodeIndex, parameterIndex);
                //UndoRedo *undoRedo = nullptr;
                //undoRedo = new URChange<AnimTrack>(int(animTrack - gNodeDelegate.GetAnimTrack().data()), [](int index) { return &gNodeDelegate.mAnimTrack[index]; });
                
                if (setKeyFrameOrValue.x < FLT_MAX)
                {
                    keyValue.x = setKeyFrameOrValue.x;
                }
                else
                {
                    keyValue.y = setKeyFrameOrValue.y;
                }
                curveEdit.EditPoint(selPoint.curveIndex, selPoint.pointIndex, keyValue);
                //delete undoRedo;
            }
            curveEdit.EndEdit();
            setKeyFrameOrValue.x = setKeyFrameOrValue.y = FLT_MAX;
        }
        if (mSelectedCurvePoints.size() == 1)
        {
            getKeyFrameOrValue = curveEdit.mPts[mSelectedCurvePoints[0].curveIndex][mSelectedCurvePoints[0].pointIndex];
        }
    }
    
    TileNodeEditGraphDelegate &mNodeGraphDelegate;
    std::vector<bool> mbExpansions;
    std::vector<bool> mbVisible;
    ImVector<ImCurveEdit::EditPoint> mSelectedCurvePoints;
    int mLastCustomDrawIndex;
    ImVec2 setKeyFrameOrValue;
    ImVec2 getKeyFrameOrValue;
    float mCurveMin, mCurveMax;
    URChange<TileNodeEditGraphDelegate::ImogenNode> *undoRedoChange;
};
#endif

