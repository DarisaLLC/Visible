//
//  voxel_frequency.h
//  Visible
//
//  Created by Arman Garakani on 3/20/19.
//

#ifndef voxel_frequency_h
#define voxel_frequency_h

#include <vector>

using namespace std;


void rf1Dfft(vector<float>& signal, vector<float>& imaginary, int32_t dir);
void rf1Ddft(vector<float>& signal, vector<float>& imaginary, int32_t dir);

typedef struct rsAnalysisVoxels {
    uint32_t weightsSz;
    float* weights;
    uint32_t workSigSz;
    float* workSig;
    char* workSigBase;
} rsAnalysisVoxels;

typedef struct rs1DFourierResult {
    vector<float> real;
    vector<float> imag;
    vector<float> phase;
    vector<float> amplitude;
    float peakFrequency;
    float peakPhase;
    float peakAmplitude;
    int32_t workingSz;
} rs1DFourierResult;

/* rf1DFourierAnalysis - Takes the 1D input signal and performs a
 * fourier analysis of the data, leaving all the results in rslt. The
 * real and imag vectors in the result structure are both resized to
 * the size of the input signal. The phase and amplitude vectors only
 * contain the entries for frequencies that are within the nyqist
 * limit (approximately half the size of the input signal).
 *
 * In addition, the result amplitudes are searched. The location of
 * the frequency at which the peak amplitude occurred is returned as
 * the peak frequency. A parabolic fit is done to find the final vaues
 * for peak frequency and peak amplitude. The peak phase is computed
 * by computing a linear fit of the two phase angles closest to the
 * computed peak frequency.
 *
 * The bits in the control word works as follows:
 *
 *   rc1DFourierForceFFT - If set, the input signal will be rounded up
 *   in size to a power of 2. This "rounding" is done by adding zeros
 *   to the end of the signal. If rc1DFourierZeroFill is also set, the
 *   input signal will be rounded up to the nearest power of 2 plus 1,
 *   otherwise it will be rounded up to the nearest power of 2.
 *
 *   rc1DFourierZeroFill - If set, zero padding is added to the end of
 *   the signal. If rc1DFourierForceFFT is also set, then zero filling
 *   is as described above, otherwise signal size number of zeros are
 *   added.
 *
 *   rc1DFourierRaisedCosine - If set, a Hanning filter is applied to
 *   the input signal.
 *
 * Note: If the modified signal (with any zero filling added) has a
 * size that is a power of 2, then an FFT is performed, otherwise a
 * DFT is performed.
 *
 * Note: In all cases, the DC component is subtracted from the input
 * signal before FFT processing is done.
 *
 * Note: Choosing to support an input signal that is a deque of
 * doubles was chosen for compatibility with the rcSimilarity's output
 * signal.
 */
enum rc1DFourierControl {
    rc1DFourierForceFFT = 1,
    rc1DFourierZeroFill = 2,
    rc1DFourierRaisedCosine = 4,
    rc1DFourierGenPhase = 8
};

void rf1DFourierAnalysis(const deque<double>& signal,
                         rs1DFourierResult& rslt,
                         uint32_t ctrl);

void rf1DFourierAnalysisVoxels(const uint8_t* sig, const uint32_t sigSz,
                               rsAnalysisVoxels& analysisInfo,
                               rs1DFourierResult& rslt, const uint32_t ctrl);

#endif /* voxel_frequency_h */
