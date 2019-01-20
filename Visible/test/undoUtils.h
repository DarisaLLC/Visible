//
//  undoUtils.h
//  Visible
//
//  Created by Arman Garakani on 1/17/19.
//

#ifndef undoUtils_h
#define undoUtils_h

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

#endif /* undoUtils_h */
