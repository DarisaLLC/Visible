

#ifndef _SIMILARITY_H
#define _SIMILARITY_H

#include <stdio.h>
#include <math.h>
#include <iterator>
#include <vector>
#include <deque>
#include <memory>
#include <algorithm>
#include "simple_timing.hpp"
#include "core/stats.hpp"
#include "registration.h"
#include "roiWindow.h"

using namespace std;

/* self_similarity_producer - Entropy signal generating class. There are two
 * steps to this process.  First, a self-similarity matrix is
 * calculated between a set of images.  Second, a measure of the
 * similarity rank between the images in this set is calculated based upon the
 * previously calculated self-similarity matrix.
 *
 * 
 */
template<typename P>
class self_similarity_producer 
{
 public:
  self_similarity_producer ();

    typedef P pixel_t;
    typedef svl::roiWindow<pixel_t> image_t;
    typedef typename std::vector<image_t>::iterator image_vector_iter_t;
    typedef typename std::deque<image_t>::iterator image_deque_iter_t;
    typedef std::function<double(const image_t&, const image_t&)> similarity_fn_t;
    
    
 // Destructor
  virtual ~self_similarity_producer();


  /* ctor - Build an self_similarity_producer object that will generate similarity rank
   * based upon the following arguments:
   *
   * matrixSz -  Used to size the internally used self-similarity
   *             matrix and must be greater than 1. It can also be
   *             thought of as defining the temporal window size to be
   *             used.
   *
   * depth -     Pixel depth of images to be analyzed.
   *
   * cacheSz -   The count of video cache entries the algorithm used to
   *             generate the self-similarity matrix should assume are
   *             available to it.
   *
   * notify -    If true, progress indication will be performed during
   *             potentially slow calculations.
   *
   * guiUpdate - Hook for GUI based user interface code to both get
   *             progress update information to display to the user
   *             and to allow users to terminate slow operations.
   *
   *             Note: guiUpdate is ignored if notify is false.  If
   *             notify is true, but guiUpdate is NULL, progress
   *             indication information is printed to stderr.
   *
   * tiny      - Give client a way to screw up signal generating
   *             calculation.
   *
   * 2nd ctor is for exhaustive a double scalar data
   *
   */
  self_similarity_producer(uint32_t matrixSz, uint32_t cacheSz,
        const similarity_fn_t& sf = self_similarity_producer<P>::similarity_fn_t (), 
		bool notify = false,
		double tiny = 1e-10);

  self_similarity_producer(uint32_t matrixSz, 	bool notify,		double tiny );


    
  /* Mutator Functions
   */
  /* fill - Allow client to specify an initial temporal window's worth
   * of images. If a full temporal window's worth of images are passed
   * in, an similarity rank signal is generated.
   *
   * Clears any existing intermediate results, so passing a 0 length
   * set of images can be used to just clear intermediate resutls.
   *
   * If firstImages is larger than the temporal window size, the
   * beginning images are ignored. In all events, up to a temporal
   * window's worth of self-similarity calculations are performed.
   * 
   * Up to the last temporal window size number of images from
   * firstImages are retained within the object for use in subsequent
   * calls to update().
   *
   * If firstImages is at least temporal window size large, then there
   * are enough images to calculate an similarity rank signal. In this case,
   * an similarity rank signal is generated and true will be returned.
   * Otherwise, no calculations are done and false is returned.
   */
  template <class Iterator>
  bool fill(Iterator Ib, Iterator Ie);
    
  bool fill(vector<image_t >& firstImages);
  bool fill(deque<image_t >& firstImages);
    
  /* update - Input the next image in a video stream. If a full
   * temporal window's worth of images are available, a new similarity rank
   * signal is generated.
   *
   * The image passed in is appended to the end of the internal
   * temporal window queue. If the queue was already full, the first
   * image is removed, along with any information relating to this
   * image stored in the self-similarity matrix.
   *
   * Similar to fill(), the self-similarity calculations are
   * performed. If this results in a full matrix of self-similarity
   * information being available, a new similarity rank signal is generated
   * and true will be returned.  Otherwise, no calculations are done
   * and false is returned.
   *
   * fillImageSize() returns the size of the first image in the filled pipeline
   */
  bool update(image_t nextImage);
    
  std::pair<int32_t,int32_t> fillImageSize () const;

  void setMask(const roiWindow<P8U>& mask);
  void clearMask();

  /* Accessor Functions
   */
  /* entropies - If an similarity rank signal has been calculated, store it
   * in signal and return true. Otherwise, return false.
   */
  bool entropies(deque<double>& signal) const;
    
    /* Accessor Functions
     */
    /* entropies - If an similarity rank signal has been calculated, 
     * take, produce average similarities to for every frame in the set around the median
     * in signal and return true. Otherwise, return false.
     */
   bool median_levelset_similarities (deque<double>& signal, float use_pct) const;
    


  /* selfSimilarityMatrix - If an similarity rank signal has been calculated,
   * and its self-similarity matrix has been saved, save a copy of it
   * in matrix. Otherwise, return false.
   */
  bool selfSimilarityMatrix(deque<deque<double> >& matrix) const;

    
  /*
   * Timing Information
   */
  const svl::stats<float>& timeStats () const { return _corrTimes; }
    
    
  /* aborted - Returns true if an update() or a fill() operation failed
   * because user aborted the operation, false otherwise.
   */
  bool aborted() const { return !_finished; }

  size_t matrixSz() const { return _matrixSz; }
  size_t cacheSz() const { return _cacheSz; }
  int32_t depth() const { return _depth; }

  friend ostream& operator<< (ostream&, const self_similarity_producer&);

 private:
  self_similarity_producer(const self_similarity_producer& rhs);
  self_similarity_producer& operator=(const self_similarity_producer& rhs);

  bool operator<(const self_similarity_producer& rhs) const;
  bool operator==(const self_similarity_producer& rhs) const;

  /*
   * specific similarity rank calculations
   */
  bool meanProjection (deque<double>& signal) const;
  void norm_scale (const std::deque<double>& src, std::deque<double>& dst, double pw) const;

  /* Internal helper fcts
   */
  /* internalFill - Called by fill() fcts to perform pixel size
   * specific fill() functionality.
   */
   template<typename Iterator>
   bool internalFill(Iterator start, Iterator end,  deque<image_t >& tWin);

  /* s*Fill - Take initial fill worth of images and perform
   * correlations required to calculate self-similarity information
   * required to generate similarity rank signal. If enough self-similarity
   * info is available, generate the similarity rank signal.
   */
  bool ssMatrixFill(deque<image_t >& tWin);

  /* internalUpdate - Called by update() fct to perform pixel size
   * specific update() functionality.
   */
  bool internalUpdate(image_t & nextImage, deque<image_t >& tWin);

  /* shiftS* - Fcts that shift self-similarity results by one image.
   */
  void shiftSMatrix();

  /* s*Update - Perform correlations between the last image in the
   * temporal window and all the other images in the window. Use this
   * to update the current self-similarity information. If enough
   * self-similarity info is available, generate the similarity rank signal.
   */
    bool ssMatrixUpdate(deque<image_t >& tWin);
    
  /* correlate - Perform correlation on the given two images and
   * return the correlation score.
   */
    double relativeCorrelate(image_t& i, image_t& m) const;

  /* correlate - Perform correlation on the given two images and
   * return the correlation score.
   */
  double correlate(image_t& i, image_t& m) const;

  /* HistogramInstersection - Perform histogram intersection on the given two images and
   * return the correlation score.
   */
  float histogramIntersection(image_t& i, image_t& m) const;

  /* genMatrixEntropy - If a full matix worth of self-similarity info
   * is available, generate an similarity rank signal and return true.
   * Otherwise, just return false.
   */
  bool genMatrixEntropy(size_t tWinSz);

  /* unity - Initialize self-similarity matrix to have identity
   * value along the identity diagonal.
   */
  void unity();

  double shannon (double r) const { return (-1.0 * r * log2 (r)); }

  similarity_fn_t               _corr_fn;
    
  /* Masking related information
   */
  bool                               _maskValid;
  roiWindow<P8U>                     _mask;
  int32_t                            _maskN;

  /* Control information
   */
   int32_t                _depth;
  const size_t                     _matrixSz;
  const size_t                     _cacheSz;
  const bool                         _notify;
  bool                               _finished;
  const double                       _tiny;
  double                             _log2MSz;
  
  /* Inputs - Temporal windows used to store the images required to
   * calculate the similarity rank signal. Pixel depth is assumed to be same
   * in all images, so only one of these may be in use at any one
   * time.
   */
  deque<image_t>  _tw8;

  /*
   * Inputs - Storage of scalar sequence of data
   *
   */
  deque<double> _seqD;

  /* Outputs
   */
  deque<deque<double> >        _SMatrix;   // Used in eExhaustive and
                                           // eApproximate cases
  deque<double>                m_entropies; // Final similarity rank signal
  std::vector<int>               m_median_ranked;
  mutable deque<double>                _sums;     // Final mean signal
  vector<double>               _kernel;    // Filtering Operation Kernel

  svl::stats<float>              _corrTimes;
    
};


typedef std::shared_ptr<self_similarity_producer<P8U> > self_similarity_producerRef;

void rf1DdistanceHistogram (const vector<double>& signal, vector<double>& dHist);

#endif /* __RC_SIMILARITY_H */
