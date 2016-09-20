/*
 * Copyright (c) 2002 Reify Corp. All rights reserved.
 */

// analysis
#include "vision/self_similarity.h"
#include "vision/pixel_traits.h"
#include "vision/registration.h"
#include "vision/rowfunc.h"
#include "core/svl_exception.hpp"
#include <vector>
#include <deque>
#include <iterator>

//#include  "image_correlation.h"
//#include "vf_math.h"

using namespace svl;

template<typename P>
self_similarity_producer<P>::~self_similarity_producer()
{
    
}


template<typename P>
void self_similarity_producer<P>::norm_scale (const std::deque<double>& src, std::deque<double>& dst, double pw) const
{
    deque<double>::const_iterator bot = std::min_element (src.begin (), src.end() );
    deque<double>::const_iterator top = std::max_element (src.begin (), src.end() );
	
    double scaleBy = *top - *bot;
    dst.resize (src.size ());
    for (int ii = 0; ii < src.size (); ii++)
    {
        double dd = (src[ii] - *bot) / scaleBy;
        dst[ii] = pow (dd, 1.0 / pw);
    }
}

static int32_t log2max(int32_t n);

template<typename P>
self_similarity_producer<P>::self_similarity_producer() : _matrixSz (0), _maskValid(false), _cacheSz (0),
_depth (P::depth()),  _cdl (eNorm),  _notify(NULL), _finished(true),_guiUpdate(NULL), _tiny(1e-10)
{
}

template<typename P>
self_similarity_producer<P>::self_similarity_producer(uint32_t matrixSz,
			     uint32_t cacheSz,
			     rcCorrelationDefinition cdl,
			     bool notify,
			     rcProgressIndicator* guiUpdate,
			     double tiny)
  : _maskValid(false),  _matrixSz(matrixSz),
    _cacheSz(cacheSz),  _cdl (cdl),
    _notify(notify), _finished(true),
    _guiUpdate(guiUpdate), _tiny(tiny)
{
//    _corrParams.pd = ByteWise; // use each byte of a multibyte pixel

    _depth = P::depth();
  _log2MSz = log2(_matrixSz);
  _ltOn = false;
}


template<typename P>
self_similarity_producer<P>::self_similarity_producer(uint32_t matrixSz,
			     bool notify,
			     double tiny)
  : _maskValid(false), _matrixSz(matrixSz), _notify(notify), _finished(true),_tiny(tiny), _cacheSz (matrixSz),
    _cdl (eNorm), _guiUpdate (NULL)
{
    _depth = P::depth();
  _log2MSz = log2(_matrixSz);
}

template<typename P>
bool self_similarity_producer<P>::fill(vector<image_t >& firstImages)
{
  assert(_matrixSz);

    image_vector_iter_t starti = firstImages.begin();

  if (firstImages.size() > _matrixSz)
    starti += (firstImages.size() - _matrixSz);

  _tw8.resize(0);
    return internalFill(starti, firstImages.end(), _tw8);

}

template<typename P>
bool self_similarity_producer<P>::fill(deque<image_t >& firstImages)
{
  assert(_matrixSz);
   image_deque_iter_t start = firstImages.begin();

  if (firstImages.size() > _matrixSz)
    start += (firstImages.size() - _matrixSz);

    _tw8.resize(0);
    return internalFill(start, firstImages.end(), _tw8);
}

template<typename P>
std::pair<int32_t,int32_t> self_similarity_producer<P>::fillImageSize() const
{
  assert(_matrixSz);
  return std::pair<int32_t,int32_t> (_tw8[0].width(), _tw8[0].height());
}

template<typename P>
bool self_similarity_producer<P>::update(image_t nextImage)
{
  assert(_matrixSz);
    return internalUpdate(nextImage, _tw8);
}


template<typename P>
void self_similarity_producer<P>::setMask(const roiWindow<P8U>& mask)
{
  assert(_matrixSz);
  assert(mask.depth() == _depth);

  _maskValid = true;
  if ((_mask.width() != mask.width()) ||
      (_mask.height() != mask.height()))
  _mask = roiWindow<P8U> (mask.width(), mask.height());
    _mask.copy_pixels_from(mask.rowPointer(0), mask.height(), mask.width(), mask.rowUpdate());
  _maskN = 0;
  uint32_t allOnes = 0xFF;
  if (_depth == DS_16U)
    allOnes = 0xFFFF;
  else if (_depth == DS_32S)
    allOnes = 0xFFFFFFFF;
  else
    assert(_depth == DS_8U);
  
  _maskN = 0;
  for (int32_t y = 0; y < _mask.height(); y++)
    for (int32_t x = 0; x < _mask.width(); x++) {
      uint32_t val = _mask.getPixel(x, y);
      if (val == allOnes)
	_maskN++;
      else
	assert(val == 0);
    }
}

template<typename P>
void self_similarity_producer<P>::clearMask()
{
  assert(_matrixSz);
  _maskValid = false;
  _mask = roiWindow<P8U>();
}

template<typename P>
bool self_similarity_producer<P>::entropies(deque<double>& signal) const
{
  assert(_matrixSz);

    if (_finished && !_entropies.empty())
    {
        signal = _entropies;
        for (unsigned int i = 0; i < signal.size(); i++)
        {
            signal[i] = 1 - signal[i];
        }
        
        return true;
    }
    
    return false;
}

/*
 *  Visual Entropy is used for mean projection versus ACI an entropic projection
 */
template<typename P>
bool self_similarity_producer<P>::meanProjection(deque<double>& signal) const
{
  assert(_matrixSz);
  if (_finished && !_sums.empty())
    {
      signal = _sums;
      return true;
    }
  return false;
}

template<typename P>
bool self_similarity_producer<P>::selfSimilarityMatrix(deque<deque<double> >& matrix) const
{
  assert(_matrixSz);

  if (_finished && !_entropies.empty() && !_SMatrix.empty()) {
    matrix = _SMatrix;
    return true;
  }

  return false;
}

template<typename P>
bool self_similarity_producer<P>::sequentialCorrelations (deque<double>& slist) const
{
  assert(_matrixSz);

  if (_finished && !_SList.empty()) {
    slist = _SList;
    return true;
  }

  return false;
}  

template<typename P>
bool self_similarity_producer<P>::longTermCache (bool onOrOff)
{
  assert(_matrixSz);
  _ltEntropies.resize (_matrixSz);
  _ltOn = onOrOff;
  return _ltOn;
}
template<typename P>
bool self_similarity_producer<P>::longTermCache () const
{
  return _ltOn;
}

template<typename P>
const vector<float>& self_similarity_producer<P>::longTermEntropy () const
{
  return _ltEntropies;
}

template<typename P>
template<typename Iterator>
bool self_similarity_producer<P>::internalFill(Iterator start, Iterator end,  deque<image_t >& tWin)
{
  _finished = true;

  for (; start < end; start++) {
    tWin.push_back(image_t(*start));
  //  (*start).frameBuf().unlock();
  }


  if (tWin.empty()) {
    if (!_SMatrix.empty())
      _SMatrix.resize(0);

    return false;
  }

  if (_SMatrix.empty()) {
    _SMatrix.resize(_matrixSz);
    for (uint32_t i = 0; i < _matrixSz; i++)
      _SMatrix[i].resize(_matrixSz);
  }
  else {
    assert(_SMatrix.size() == _matrixSz);
    for (uint32_t i = 0; i < _matrixSz; i++)
      assert(_SMatrix[i].size() == _matrixSz);
  }

  /* Initialize identity diagonal of _SMatrix.
   */
  unity();

  /*
   * if longtermCache is on, write the first matrix size entropies
   */ 
  bool rtn = (_finished = ssMatrixFill(tWin)) && genMatrixEntropy(tWin.size());
  if (rtn && _ltOn && !_entropies.empty())
    {
      for (uint32_t i = 0; i < matrixSz(); i++)
	_ltEntropies[i] = (float) _entropies[i];
    }

  return rtn;
}



template <typename P>
bool self_similarity_producer<P>::ssMatrixFill(deque<image_t >& tWin)
{
  assert(_SMatrix.size() == _matrixSz);

  const int32_t tWinSz = tWin.size();
  assert(tWinSz <= (int32_t)_matrixSz);

  int32_t cacheSz = _cacheSz;
  if (cacheSz <= 2)
    cacheSz = tWinSz + 2;
  const int32_t cacheBlkSz = cacheSz - 2;


  /* The following was designed to work well when the number of images
   * that can be stored in memory at any one time is space limited.
   * In the absence of other consumers of cache resources, it reads in
   * O((N^2)/C) images from disk, where N is the number of images in
   * the movie fifo and C is the count of cache entries available.
   * The basic algorithm works as follows:
   *
   * Step 1 - Use the first cache block size number of images
   * remaining and perform cross correlations on all of them.
   *
   * Step 2 - For all the remaining images, read in one image at a
   * time, correlating each with every image from step 1 before moving
   * on to the next image.
   *
   * Note: In both steps 1) and 2), passes through the cached images
   * are done in alternating directions to guarantee that the most
   * recently used images get processed first. This is done to make
   * performance degradation more linear in the presence of other
   * consumers of the cache. In particular, it limits pathological
   * cases where reading needed images into the cache causes other
   * needed images to be flushed from the cache before they get
   * used. Note that this assumes an LRU type caching policy.
   *
   * Step 3 - Update the current starting point by the cache block
   * size.
   *
   * Repeat the entire process until the new starting point is >= the
   * fifo size.
   */
  for (int32_t i = 0; i < tWinSz;
       /* Step 3 - Update start */ i += cacheBlkSz) {
    int32_t cacheIncr = 1, cacheBegin, cacheEnd;
    int32_t firstUncachedFrame = i + cacheBlkSz;
    if (firstUncachedFrame > tWinSz)
      firstUncachedFrame = tWinSz;

    /* Step 1 - Fill cache.
     */
    for (int32_t j = i + 1; j < firstUncachedFrame; j++) {
      if (cacheIncr == 1) {
	cacheIncr = -1;	cacheBegin = j - 1; cacheEnd = i - 1;
      }
      else {
	cacheIncr = 1;	cacheBegin = i; cacheEnd = j;
      }

      for (int32_t k = cacheBegin; k != cacheEnd; k += cacheIncr) {
	assert((j >= 0) && (j < tWinSz));
	assert((k >= 0) && (k < tWinSz));
    chronometer timeit;
	const double r = correlate(tWin[j], tWin[k]);
  _corrTimes.add((float) timeit.getTime ());          
	_SMatrix[j][k] = _SMatrix[k][j] = r;
//	tWin[k].frameBuf().unlock();
//	if (progress && progress->update()) {
//	  tWin[j].frameBuf().unlock();
//	  delete progress;
//	  return false;
//	}
      } // End of: for ( k = cacheBegin; k != cacheEnd; k += cacheIncr)
    //  tWin[j].frameBuf().unlock();
    } // End of: for (int32_t j = i + 1; j < firstUncachedFrame; j++)

    /* Step 2 - Correlate remaining images against cached images.
     */
    for (int32_t j = firstUncachedFrame; j < tWinSz; j++) {
      if (cacheIncr == 1) {
	cacheIncr = -1;	cacheBegin = firstUncachedFrame-1; cacheEnd = i-1;
      }
      else {
	cacheIncr = 1; cacheBegin = i; cacheEnd = firstUncachedFrame;
      }

      for (int32_t k = cacheBegin; k != cacheEnd; k += cacheIncr) {
	assert((j >= 0) && (j < tWinSz));
	assert((k >= 0) && (k < tWinSz));
          chronometer timeit;
	const double r = correlate(tWin[j], tWin[k]);
          _corrTimes.add((float) timeit.getTime ());
          
	_SMatrix[j][k] = _SMatrix[k][j] = r;
	//tWin[k].frameBuf().unlock();
//	if (progress && progress->update()) {
//	  tWin[j].frameBuf().unlock();
//	  delete progress;
//	  return false;
//	}
      } // End of: for (k = cacheBegin; k != cacheEnd; k += cacheIncr)
     // tWin[j].frameBuf().unlock();
    } // End of: for (int32_t j = i + 1; j < firstUncachedFrame; j++)
  }



  return true;
}


template <typename P>
bool self_similarity_producer<P>::ssListFill(deque<image_t >& tWin)
{
  if (tWin.size() < 2)
    return true;

  assert(_SList.size() >= (tWin.size()-1));

  for (uint32_t i = 1; i < tWin.size(); i++) {
    _SList[i-1] = correlate(tWin[i-1], tWin[i]);
  //  tWin[i-1].frameBuf().unlock();
//    if (progress && progress->update()) {
//      tWin[i].frameBuf().unlock();
//      delete progress;
//      return false;
//    }
  }

 // tWin[tWin.size()-1].frameBuf().unlock();
  return true;
}




template <typename P>
bool self_similarity_producer<P>::internalUpdate(image_t& nextImage, deque<image_t >& tWin)
{
  /* First, see if an image and its associated results needs to be
   * removed.
   */
  if (tWin.size() == _matrixSz) {
    tWin.pop_front();
      shiftSMatrix();
  }
  
    if (_SMatrix.empty()) {
      _SMatrix.resize(_matrixSz);
      for (uint32_t i = 0; i < _matrixSz; i++)
	_SMatrix[i].resize(_matrixSz);

    assert(_SMatrix.size() == _matrixSz);
    for (uint32_t i = 0; i < _matrixSz; i++)
      assert(_SMatrix[i].size() == _matrixSz);
  }

  tWin.push_back(nextImage);
 // nextImage.frameBuf().unlock();

  /*
   * if longtermCache is on, write the last entropy value to the cache line
   */ 

  bool rtn = (_finished=ssMatrixUpdate(tWin)) && genMatrixEntropy(tWin.size());
  if (rtn && _ltOn && !_entropies.empty())
    {
      _ltEntropies.push_back ((float) _entropies.back ());
    }

  return rtn;
}

template<typename P>
void self_similarity_producer<P>::shiftSList()
{
  assert(!_SList.empty());
  _SList.pop_front();
  _SList.push_back(-1.0);
}

template<typename P>
void self_similarity_producer<P>::shiftSMatrix()
{
  if (_SMatrix.size() != _matrixSz)
      throw svl::assertion_error("Similarity Engine Failure");

  _SMatrix.pop_front();

  for (uint32_t i = 0; i < (_matrixSz - 1); i++) {
    _SMatrix[i].pop_front();
    _SMatrix[i].push_back(-1.0);
  }

  deque<double> empty;
  _SMatrix.push_back(empty);
  _SMatrix.back().resize(_matrixSz);
}




template<typename P>
bool self_similarity_producer<P>::ssMatrixUpdate(deque<image_t>& tWin)
{
  assert(_SMatrix.size() == _matrixSz);
  assert(!tWin.empty());

  const uint32_t lastImgIndex = tWin.size() - 1;
  assert(lastImgIndex < _matrixSz);

//  progressNotification* progress = 0;
//  if (_notify) progress =
//    new progressNotification(lastImgIndex, _guiUpdate);

  _SMatrix[lastImgIndex][lastImgIndex] = 1.0 + _tiny;

  for (uint32_t i = 0; i < lastImgIndex; i++) {
    _SMatrix[i][lastImgIndex] = _SMatrix[lastImgIndex][i] = 
      correlate(tWin[i], tWin[lastImgIndex]);
 //   tWin[i].frameBuf().unlock();
//    if (progress && progress->update()) {
//      tWin[lastImgIndex].frameBuf().unlock();
//      delete progress;
//      return false;
//    }
  }

 // tWin[lastImgIndex].frameBuf().unlock();

  return true;
}


template <typename P>
double self_similarity_producer<P>::correlate(image_t& i, image_t& m) const
{
    CorrelationParts cp;

// @todo support mask
    Correlation::point(i, m, cp);
    return cp.r();
}


template <typename P>
bool self_similarity_producer<P>::genMatrixEntropy(uint32_t tWinSz)
{
  if (tWinSz != _matrixSz)
    return false;

  /* Create sums array and initialize all the elements.
   */

  assert(_SMatrix.size() == _matrixSz);

    if (_sums.empty())
        _sums.resize(_matrixSz);
    else
        assert(_sums.size() == _matrixSz);

  if (_entropies.empty())
    _entropies.resize(_matrixSz);
  else
    assert(_entropies.size() == _matrixSz);

  for (uint32_t i = 0; i < _matrixSz; i++) {
    assert(_SMatrix[i].size() == _matrixSz);
    _sums[i] = _SMatrix[i][i];
    _entropies[i] = 0.0;
  }

  for (uint32_t i = 0; i < (_matrixSz-1); i++)
    for (uint32_t j = (i+1); j < _matrixSz; j++) {
      _sums[i] += _SMatrix[i][j];
      _sums[j] += _SMatrix[i][j];
    }

    for (uint32_t  i = 0; i < _matrixSz; i++) {
    for (uint32_t j = i; j < _matrixSz; j++) {
      double rr =
	_SMatrix[i][j]/_sums[i]; // Normalize for total energy in samples
      _entropies[i] += shannon(rr);
      
      if (i != j) {
	rr = _SMatrix[i][j]/_sums[j];//Normalize for total energy in samples
	_entropies[j] += shannon(rr);
      }
    }
    _entropies[i] = _entropies[i]/_log2MSz;// Normalize for cnt of samples
  }

    for (int ii = 0; ii < _sums.size (); ii++)
    {
        _sums[ii] /= _matrixSz;
//        _sums[ii] = _sums[ii] - 1;
    }

  return true;
}

template<typename P>
void self_similarity_producer<P>::unity ()
{
  assert(_SMatrix.size() == _matrixSz);

  for (uint32_t i = 0; i < _matrixSz; i++)
    _SMatrix[i][i] = 1.0 + _tiny;
}

template<typename P>
ostream& operator<< (ostream& ous, const self_similarity_producer<P>& rc)
{
  self_similarity_producer<P>* rc2 = const_cast<self_similarity_producer<P>*>(&rc);
  deque<deque<double> >& cm = rc2->_SMatrix;
  if (!cm.empty()) {
    ous << "{";
    for (uint32_t i = 0; i < cm.size(); i++) {
      ous << "{";
      deque<double>::iterator ds = cm[i].begin();
      for (uint32_t j = 0; j < cm.size() - 1; j++, ds++)	   
	ous << *ds << ",";

      if (i < cm.size() - 1)
	ous << *ds << "}," << endl;
      else ous << *ds << "}" << endl;
    }
    ous << "}" << endl;
  }
  return ous;
}

#if 0
  
template <class T>
bool self_similarity_producer::filterOp (vector<T>& signal)
{
  float zf (0.0f);
  Vec_O_DP tmp(9);

  _kernel.resize(9);

  NR::savgol (tmp, _kernel.size(),_kernel.size()/2,
	      _kernel.size()/2,0,4);

  // Copy from the wrap arounb order of savgol
  int32_t i (0), j;
  for (j = 0, i = (int32_t) _kernel.size()/2; i>= 0; i--, j++)
    _kernel[j] = tmp[i];
  for (i = 0; i < (int32_t) _kernel.size()/2; i++, j++)
    _kernel[j] = tmp[_kernel.size()-i-1];

  int32_t pow2 = log2max (signal.size());
  vector<double> dSig (1 << pow2, zf);
  dSig.assign (signal.begin(), signal.end());
  if ((1 << pow2) != (int32_t) dSig.size())
    dSig.resize (1<< pow2, zf);

  Vec_O_DP vsig (&dSig[0], dSig.size());
  Vec_O_DP vfilt (dSig.size());
  
  NR::convlv (vsig, tmp, 1, vfilt);

  for (i = 0; i < (int32_t) signal.size(); i++)
    signal[i] = vfilt[i];

  return true;
}

template <class T>
double self_similarity_producer::genPeriodicity (const vector<T>& signal, const vector<T>& absc, rcDPair& freq)
{
  const double ofac (4.0f);
  const double hfac (1.0f);
  const double nout (0.5f * ofac * hfac * signal.size());
  Vec_O_DP px (signal.size() * 2);
  Vec_O_DP py (px.size());
  double prob;
  int32_t jmax, nf;

  Vec_O_DP dSig (&signal[0], signal.size());
  Vec_O_DP vSig (&absc[0], absc.size());

  NR::period (vSig, dSig, ofac, hfac, px, py, nf, jmax, prob);

  freq.x() = jmax;
  freq.y() = py[jmax];

  vector<uint32_t> peakLocs;
  vector<double> interLocs, interVals, dSigVec (py.size());
  for (int32 i = 0; i < py.size(); i++)
    dSigVec[i] = py[i];

  rf1DPeakDetect(dSigVec, peakLocs, interLocs,
		 interVals, 0.10);

  // Assume min frequency is 1/2 as many frames per period
  // ==> 2 
  float minFrequency = 2.0;

  /* Find the the frequency, greater than minFrequency, with the
   * greatest amplitude.
   */
  uint32_t maxFreqIndex = 0;
  double maxAmplitude = -1.0;
  for (uint32_t ii = 0; ii < peakLocs.size(); ii++) {
    if ((interLocs[ii] > minFrequency) && (interVals[ii] > maxAmplitude)) {
      maxFreqIndex = ii;
      maxAmplitude = interVals[ii];
    }
    if (1)
      fprintf(stderr, " (%d %f) inter: (%f, %f)\n",
	      peakLocs[ii], dSigVec[peakLocs[ii]],
	      interLocs[ii], interVals[ii]);
  }
  
  if (1)
    fprintf(stderr, "maxAmplitude %f maxFreqIndex %d minFrequency %f\n",
	    maxAmplitude, maxFreqIndex, minFrequency);

  if (maxAmplitude == -1.0)
    return 0.0;

  if (1)
    fprintf(stderr, "Return max peak - @ %d: loc %f amp %f\n",
	    maxFreqIndex, interLocs[maxFreqIndex],
	    interVals[maxFreqIndex]);

  return interLocs[maxFreqIndex]/nout;

}


bool self_similarity_producer::filter (vector<double>& signal)
{
  return filterOp (signal);
}

double self_similarity_producer::periodicity (const vector<double>& signal, const vector<double>& absc, rcDPair& freq)
{
  return genPeriodicity (signal, absc, freq);
}

#endif


////////////////////////////////////////////////////////////////////////////////
//
//	log2max
//
//	This function computes the ceiling of log2(n).  For example:
//
//	log2max(7) = 3
//	log2max(8) = 3
//	log2max(9) = 4
//
////////////////////////////////////////////////////////////////////////////////
static int32_t log2max(int32_t n)
{
  int32_t power = 1;
  int32_t k = 1;
  
  if (n==1) {
    return 0;
  }
	
  while ((k <<= 1) < n) {
    power++;
  }
	
  return power;
}

      

  


// 1D Distance Histogram
//
void rf1DdistanceHistogram (const vector<double>& signal, vector<double>& dHist)
{
  auto n = signal.size();
  assert(n);
  dHist.resize (n);

  // Get container value type
  vector<double>::const_iterator ip, jp;

  for (ip = signal.begin(); ip < signal.end(); ip++)
    for (jp = ip + 1; jp < signal.end(); jp++)
    {
      double iv (*ip);
      double jv (*jp);
      auto dj = jp - signal.begin();
      auto di = ip - signal.begin();
      dj = dj - di;
      assert(dj >= 0 && dj < n);
        iv = iv - jv;
        dHist[dj] += iv*iv;
    }

}



template class self_similarity_producer<P8U>;


