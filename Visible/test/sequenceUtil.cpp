//
//  sequenceUtil.cpp
//  VisibleTestApp
//
//  Created by Arman Garakani on 1/11/19.
//

#include "sequenceUtil.hpp"
#include "imgui_internal.h"

using namespace ImCurveEdit;




int MySequence::GetFrameMin() const { return m_uicontainer.mFrameMin; }
int MySequence::GetFrameMax() const { return m_uicontainer.mFrameMax; }
void MySequence::BeginEdit(int index){
    //assert(undoRedoChange == nullptr);
    // undoRedoChange = new URChange<trackUIContainer::ImogenNode>(index, [](int index) { return &gNodeDelegate.mNodes[index]; });
}
void MySequence::EndEdit(){
    // delete undoRedoChange;
    // undoRedoChange = NULL;
}

int MySequence::GetItemCount() const { return 2; } //(int)gEvaluation.GetStagesCount(); }


void MySequence::CustomDraw(int index, ImDrawList* draw_list, const ImRect& rc, const ImRect& legendRect, const ImRect& clippingRect, const ImRect& legendClippingRect){

    if (mLastCustomDrawIndex != index)
    {
        mLastCustomDrawIndex = index;
        mbVisible.clear();
    }
    
    ImVec2 curveMin(float(m_uicontainer.mFrameMin), mCurveMin);
    ImVec2 curveMax(float(m_uicontainer.mFrameMax), mCurveMax);
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

void trackUIContainer::set(const duration_time_t& entire, const  std::shared_ptr<vectorOfnamedTrackOfdouble_t>& shared ){
    
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
    
void trackUIContainer::setTimeSlot (size_t index, int frameStart, int frameEnd){
        
    auto & node = mNodes[index];
    node.mStartFrame = frameStart;
    node.mEndFrame = frameEnd;
}

void trackUIContainer::setTimeDuration(size_t index, int duration){
    auto & node = mNodes[index];
    node.mEndFrame = node.mStartFrame + duration;
}

void trackUIContainer::setTime(int time, bool updateDecoder){
 //   gEvaluationTime = time;
 //   for (size_t i = 0; i < mNodes.size(); i++)
 //   {
  //      const ImogenNode& node = mNodes[i];
  //      gEvaluation.SetStageLocalTime(i, ImClamp(time - node.mStartFrame, 0, node.mEndFrame - node.mStartFrame), updateDecoder);
  //  }
}

void trackUIContainer::clear(){
    mSelectedNodeIndex = -1;
    mNodes.clear();
  //  mAnimTrack.clear();
  //  mEditingContext.Clear();
}

    
    
#if 0
    void trackUIContainer::SetParamBlock(size_t index, const std::vector<unsigned char>& parameters)
    {
        ImogenNode & node = mNodes[index];
        node.mParameters = parameters;
        gEvaluation.SetEvaluationParameters(index, parameters);
        gEvaluation.SetEvaluationSampler(index, node.mInputSamplers);
    }
#endif



Evaluation::Evaluation() 
{
    
}

void Evaluation::Init()
{
    APIInit();
}

void Evaluation::Finish()
{
    
}

void Evaluation::AddSingleEvaluation(size_t nodeType)
{
    EvaluationStage evaluation;
#ifdef _DEBUG
    evaluation.mNodeTypename          = gMetaNodes[nodeType].mName;;
#endif
    evaluation.mDecoder               = NULL;
    evaluation.mUseCountByOthers      = 0;
    evaluation.mNodeType              = nodeType;
    evaluation.mParametersBuffer      = 0;
    evaluation.mBlendingSrc           = ONE;
    evaluation.mBlendingDst           = ZERO;
    evaluation.mLocalTime             = 0;
    evaluation.gEvaluationMask        = gEvaluators.GetMask(nodeType);
    evaluation.mbDepthBuffer          = false;
    mStages.push_back(evaluation);
}

void Evaluation::StageIsAdded(int index)
{
    for (size_t i = 0;i< gEvaluation.mStages.size();i++)
    {
        if (i == index)
            continue;
        auto& evaluation = gEvaluation.mStages[i];
        for (auto& inp : evaluation.mInput.mInputs)
        {
            if (inp >= index)
                inp++;
        }
    }
}

void Evaluation::StageIsDeleted(int index)
{
    EvaluationStage& ev = gEvaluation.mStages[index];
    ev.Clear();
    
    // shift all connections
    for (auto& evaluation : gEvaluation.mStages)
    {
        for (auto& inp : evaluation.mInput.mInputs)
        {
            if (inp >= index)
                inp--;
        }
    }
}

void Evaluation::UserAddEvaluation(size_t nodeType)
{
    URAdd<EvaluationStage> undoRedoAddStage(int(mStages.size()), []() {return &gEvaluation.mStages; },
                                            StageIsDeleted, StageIsAdded);
    
    AddSingleEvaluation(nodeType);
}

void Evaluation::UserDeleteEvaluation(size_t target)
{
    URDel<EvaluationStage> undoRedoDelStage(int(target), []() {return &gEvaluation.mStages; },
                                            StageIsDeleted, StageIsAdded);
    
    StageIsDeleted(int(target));
    mStages.erase(mStages.begin() + target);
}

void Evaluation::SetEvaluationParameters(size_t target, const std::vector<unsigned char> &parameters)
{
    EvaluationStage& stage = mStages[target];
    stage.mParameters = parameters;
    
    if (stage.gEvaluationMask&EvaluationGLSL)
        BindGLSLParameters(stage);
    if (stage.mDecoder)
        stage.mDecoder = NULL;
}

void Evaluation::SetEvaluationSampler(size_t target, const std::vector<InputSampler>& inputSamplers)
{
    mStages[target].mInputSamplers = inputSamplers;
    gCurrentContext->SetTargetDirty(target);
}

void Evaluation::AddEvaluationInput(size_t target, int slot, int source)
{
    if (mStages[target].mInput.mInputs[slot] == source)
        return;
    mStages[target].mInput.mInputs[slot] = source;
    mStages[source].mUseCountByOthers++;
    gCurrentContext->SetTargetDirty(target);
}

void Evaluation::DelEvaluationInput(size_t target, int slot)
{
    mStages[mStages[target].mInput.mInputs[slot]].mUseCountByOthers--;
    mStages[target].mInput.mInputs[slot] = -1;
    gCurrentContext->SetTargetDirty(target);
}

void Evaluation::SetEvaluationOrder(const std::vector<size_t> nodeOrderList)
{
    mEvaluationOrderList = nodeOrderList;
}

void Evaluation::Clear()
{
    for (auto& ev : mStages)
        ev.Clear();
    
    mStages.clear();
    mEvaluationOrderList.clear();
}

void Evaluation::SetMouse(int target, float rx, float ry, bool lButDown, bool rButDown)
{
    for (auto& ev : mStages)
    {
        ev.mRx = -9999.f;
        ev.mRy = -9999.f;
        ev.mLButDown = false;
        ev.mRButDown = false;
    }
    auto& ev = mStages[target];
    ev.mRx = rx;
    ev.mRy = 1.f - ry; // inverted for UI
    ev.mLButDown = lButDown;
    ev.mRButDown = rButDown;
}

size_t Evaluation::GetEvaluationImageDuration(size_t target)
{
    auto& stage = mStages[target];
    if (!stage.mDecoder)
        return 1;
    if (stage.mDecoder->mFrameCount > 2000)
    {
        int a = 1;
    }
    return stage.mDecoder->mFrameCount;
}

void Evaluation::SetStageLocalTime(size_t target, int localTime, bool updateDecoder)
{
    auto& stage = mStages[target];
    int newLocalTime = ImMin(localTime, int(GetEvaluationImageDuration(target)));
    if (stage.mDecoder && updateDecoder && stage.mLocalTime != newLocalTime)
    {
        stage.mLocalTime = newLocalTime;
        Image_t image = stage.DecodeImage();
        SetEvaluationImage(int(target), &image);
        FreeImage(&image);
    }
    else
    {
        stage.mLocalTime = newLocalTime;
    }
}

int Evaluation::Evaluate(int target, int width, int height, Image *image)
{
    EvaluationContext *previousContext = gCurrentContext;
    EvaluationContext context(gEvaluation, true, width, height);
    gCurrentContext = &context;
    context.RunBackward(target);
    GetEvaluationImage(target, image);
    gCurrentContext = previousContext;
    return EVAL_OK;
}

FFMPEGCodec::Decoder* Evaluation::FindDecoder(const std::string& filename)
{
    for (auto& evaluation : mStages)
    {
        if (evaluation.mDecoder && evaluation.mDecoder->GetFilename() == filename)
            return evaluation.mDecoder.get();
    }
    auto decoder = new FFMPEGCodec::Decoder;
    decoder->Open(filename);
    return decoder;
}
