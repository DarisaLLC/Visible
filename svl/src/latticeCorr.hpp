//
//  latticeCorr.hpp
//  
//
//  Created by Arman Garakani on 4/14/17.
//
//

#ifndef latticeCorr_hpp
#define latticeCorr_hpp

#include <stdio.h>

//@class rcLatticeSimilarator
//
class rcLatticeSimilarator : public virtual rcRefCounter
{
public:
    
    static const rcInt32 defaultTileSize = 4;
    typedef vector<rcIRect> rowOfRects;
    typedef vector<rcSimilaratorRef> rowOfSims;
    
    rcLatticeSimilarator ();
    
    rcLatticeSimilarator(rcInt32 tpipt, rcInt32 lrad);
    
    template<class Iterator>
    bool fill(Iterator starting, Iterator afterLast);
    
    template<class Iterator>
    bool update(Iterator nextImage);
    
    void ttc (rcWindow&, rcWindow&);
    
    double wholeEntropy () const;
    
private:
    rcInt32 mTemporal;
    rcIPair mSize;
    rcIPair mImageSize;
    rcPixel mDepth;
    rcInt32 mLrad;
    vector<rowOfRects> mTiles;
    vector<rowOfSims> mSims;
    rcSimilaratorRef mWholeSim;
};

typedef rcSharedPtr<rcLatticeSimilarator> rcLatticeSimilaratorRef;


template<class Iterator>
bool rcLatticeSimilarator::fill(Iterator starting, Iterator afterLast)
{
    typedef typename std::iterator_traits<Iterator>::value_type value_type;
    
    if (mTemporal != distance (starting, afterLast)) return false;
    
    rcInt32 width, height;
    value_type image (*starting);
    width = image.width(); height = image.height();
    rcIPair tileSize (defaultTileSize, defaultTileSize);
    
    // @note overlapping tiles. Amount of overlap is determined by step size.
    // if mLrad is set to defaultTileSize divided by 2, then neighboring tiles overlap 50 percent
    // that is 100 - mLrad / defaultTileSize.
    rcInt32 rows ((height - defaultTileSize) / mLrad);
    rcInt32 columns ((width - defaultTileSize) / mLrad);
    
    if (rows <= 0 || columns <=0) return false;
    mSize = rcIPair (columns, rows);
    mImageSize = rcIPair (width, height);
    mDepth = image.depth();
    
    // Allocate an array of tile rectangles
    mTiles.resize (rows);
    mSims.resize (rows);
    
    
    rcInt32 tly (0);
    
    for (rcInt32 j = 0; j < rows; j++)
    {
        mTiles[j].resize (columns);
        mSims[j].resize (columns);
        
        rcInt32 tlx (0);
        for (rcInt32 i = 0; i < columns; i++, tlx += mLrad)
        {
            rcIPair tl (tlx, tly);
            mTiles[j][i] = rcIRect (tl, tl + tileSize);
        }
        tly += mLrad;
    }
    
    bool rtn (false);
    for (rcInt32 j = 0; j < rows; j++)
        for (rcInt32 i = 0; i < columns; i++)
        {
            vector<rcWindow> wins;
            
            Iterator w = starting;
            while (w < afterLast)
            {
                value_type img (*w);
                rtn = (img.size() == mImageSize);
                if (rtn == false) return rtn;
                
                
                static const rcInt32 inside (0);
                rcWindow tile (img, mTiles[j][i], inside);
                wins.push_back (tile);
                w++;
            }
            
            mSims[j][i] = new rcSimilarator (rcSimilarator::eExhaustive,
                                             mDepth, mTemporal, mTemporal * 2);
            rtn = (mSims[j][i] && mSims[j][i]->fill (wins));
            if (rtn == false) return rtn;
            rmAssert (mSims[j][i]->longTermCache () == false);
            rtn = mSims[j][i]->longTermCache (true);
            if (rtn == false) return rtn;
        }
    
    if (rtn)
    {
        vector<rcWindow> wins;
        
        Iterator w = starting;
        while (w < afterLast)
        {
            value_type img (*w);
            rtn = (img.size() == mImageSize);
            if (rtn == false) return rtn;
            
            wins.push_back (img);
            w++;
        }
        
        mWholeSim = new rcSimilarator (rcSimilarator::eExhaustive,
                                       mDepth, mTemporal, mTemporal * 2);
        
        rtn = (mWholeSim && mWholeSim->fill (wins));
        if (rtn == false) return rtn;
        rmAssert (mWholeSim->longTermCache () == false);
        rtn = mWholeSim->longTermCache (true);
        if (rtn == false) return rtn;
    }
    
    return rtn;
}


template<class Iterator>
bool rcLatticeSimilarator::update(Iterator nextImage)
{
    typedef typename std::iterator_traits<Iterator>::value_type value_type;
    value_type image (*nextImage);  
    
    bool rtn (false);
    
    rtn = mWholeSim->update (image);
    if (rtn == false) return rtn;
    
    for (rcInt32 j = 0; j < mSize.y(); j++)
        for (rcInt32 i = 0; i < mSize.x(); i++)
        {
            static const rcInt32 inside (0);
            rcWindow tile (image, mTiles[j][i], inside);
            rtn = mSims[j][i]->update (tile);
            if (rtn == false) return rtn;
        }
    
    return rtn;
}


#endif /* latticeCorr_hpp */
