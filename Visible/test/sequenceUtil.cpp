//
//  sequenceUtil.cpp
//  VisibleTestApp
//
//  Created by Arman Garakani on 1/11/19.
//

#include "sequenceUtil.hpp"
#include "imgui_internal.h"

using namespace ImCurveEdit;




int MySequence::GetFrameMin() const { return mNodeGraphDelegate.mFrameMin; }
int MySequence::GetFrameMax() const { return mNodeGraphDelegate.mFrameMax; }
void MySequence::BeginEdit(int index)
{
    assert(undoRedoChange == nullptr);
   // undoRedoChange = new URChange<TileNodeEditGraphDelegate::ImogenNode>(index, [](int index) { return &gNodeDelegate.mNodes[index]; });
}
void MySequence::EndEdit()
{
    delete undoRedoChange;
    undoRedoChange = NULL;
}

int MySequence::GetItemCount() const { return 2; } //(int)gEvaluation.GetStagesCount(); }


void MySequence::CustomDraw(int index, ImDrawList* draw_list, const ImRect& rc, const ImRect& legendRect, const ImRect& clippingRect, const ImRect& legendClippingRect)
{
    if (mLastCustomDrawIndex != index)
    {
        mLastCustomDrawIndex = index;
        mbVisible.clear();
    }
    
    ImVec2 curveMin(float(mNodeGraphDelegate.mFrameMin), mCurveMin);
    ImVec2 curveMax(float(mNodeGraphDelegate.mFrameMax), mCurveMax);
  //  AnimCurveEdit curveEdit(curveMin, curveMax, gNodeDelegate.mAnimTrack, mbVisible, index);
  //  mbVisible.resize(curveEdit.GetCurveCount(), true);
    draw_list->PushClipRect(legendClippingRect.Min, legendClippingRect.Max, true);
#if 0
    for (int i = 0; i < curveEdit.GetCurveCount(); i++)
    {
        ImVec2 pta(legendRect.Min.x + 30, legendRect.Min.y + i * 14.f);
        ImVec2 ptb(legendRect.Max.x, legendRect.Min.y + (i + 1) * 14.f);
        draw_list->AddText(pta, mbVisible[i] ? 0xFFFFFFFF : 0x80FFFFFF, curveEdit.mLabels[i].c_str());
        if (ImRect(pta, ptb).Contains(ImGui::GetMousePos()) && ImGui::IsMouseClicked(0))
            mbVisible[i] = !mbVisible[i];
    }
#endif
    draw_list->PopClipRect();
    
    ImGui::SetCursorScreenPos(rc.Min);
  //  ImCurveEdit::Edit(curveEdit, rc.Max - rc.Min, 137 + index, &clippingRect, &mSelectedCurvePoints);
    mCurveMin = curveMin.y;
    mCurveMax = curveMax.y;
    
    getKeyFrameOrValue = ImVec2(FLT_MAX, FLT_MAX);
  
#if 0
    if (focused || curveEdit.focused)
    {
        if (ImGui::IsKeyPressedMap(ImGuiKey_Delete))
        {
            curveEdit.DeletePoints(mSelectedCurvePoints);
        }
    }
#endif
    if (setKeyFrameOrValue.x < FLT_MAX || setKeyFrameOrValue.y < FLT_MAX)
    {
   //     curveEdit.BeginEdit(0);
        for (int i = 0; i < mSelectedCurvePoints.size(); i++)
        {
         //   auto& selPoint = mSelectedCurvePoints[i];
        //    int keyIndex = selPoint.pointIndex;
        //    ImVec2 keyValue = curveEdit.mPts[selPoint.curveIndex][keyIndex];
       //     uint32_t parameterIndex = curveEdit.mParameterIndex[selPoint.curveIndex];
       //     AnimTrack* animTrack = gNodeDelegate.GetAnimTrack(gNodeDelegate.mSelectedNodeIndex, parameterIndex);
            //UndoRedo *undoRedo = nullptr;
            //undoRedo = new URChange<AnimTrack>(int(animTrack - gNodeDelegate.GetAnimTrack().data()), [](int index) { return &gNodeDelegate.mAnimTrack[index]; });
            
            if (setKeyFrameOrValue.x < FLT_MAX)
            {
         //       keyValue.x = setKeyFrameOrValue.x;
            }
            else
            {
         //       keyValue.y = setKeyFrameOrValue.y;
            }
      //      curveEdit.EditPoint(selPoint.curveIndex, selPoint.pointIndex, keyValue);
            //delete undoRedo;
        }
      //  curveEdit.EndEdit();
        setKeyFrameOrValue.x = setKeyFrameOrValue.y = FLT_MAX;
    }
    if (mSelectedCurvePoints.size() == 1)
    {
      //  getKeyFrameOrValue = curveEdit.mPts[mSelectedCurvePoints[0].curveIndex][mSelectedCurvePoints[0].pointIndex];
    }
}

trackUIContainer::trackUIContainer(const duration_time_t& entire, const  std::shared_ptr<vectorOfnamedTrackOfdouble_t>& shared ):
 mbMouseDragging(false),  mFrameMin(0), mFrameMax(1)
{
  
    
    m_weakRef = shared;
    // second is a vector of pairs where first is index_time_t and second is value
    mFrameMin = (int) (entire.first.first);
    mFrameMax = (int) (entire.second.first);
    
    // Fillup the UInodes
    for(auto const & namedTrack : *shared){
        node_t nt;
        nt.mName = namedTrack.first;
        nt.mStartFrame = (int) namedTrack.second.front().first.first;
        nt.mEndFrame = (int) namedTrack.second.front().first.first;
        mNodes.push_back(nt);
    }
}

//void TileNodeEditGraphDelegate::Clear()
//{
//    mSelectedNodeIndex = -1;
//    mNodes.clear();
  //  mAnimTrack.clear();
  //  mEditingContext.Clear();
//}
#if 0
void TileNodeEditGraphDelegate::SetParamBlock(size_t index, const std::vector<unsigned char>& parameters)
{
    ImogenNode & node = mNodes[index];
    node.mParameters = parameters;
    gEvaluation.SetEvaluationParameters(index, parameters);
    gEvaluation.SetEvaluationSampler(index, node.mInputSamplers);
}
#endif

