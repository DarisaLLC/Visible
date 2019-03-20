
#include <vector>
#include "core/core.hpp"
#include "vision/voxel_frequency.h"
#include "core/fit.hpp"

using namespace std;


// This file contains frequency domain tools as well as a periodicity measurement tool
// All FFT functions are based on Apple's vecLib framework

/*
   This computes an in-place complex-to-complex FFT 
   x and y are the real and imaginary arrays of 2^m points.
   dir =  1 gives forward transform
   dir = -1 gives reverse transform 
*/
void rf1Dfft (vector<float>& signal, vector<float>& imaginary, int32_t dir)
{
  int32_t i,i1,j,k,i2,l,l1,l2;
  float c1,c2,tx,ty,t1,t2,u1,u2,z;
  int n = static_cast<int>(signal.size());
  auto m = (int32_t) log2 ((double) signal.size());
  assert (n == (1 << m));
  vector<float>::iterator x = signal.begin();
  vector<float>::iterator y = imaginary.begin();
   
  /* Do the bit reversal */
  i2 = n >> 1;
  j = 0;
  for (i=0;i<n-1;i++) {
    if (i < j) {
      tx = x[i];
      ty = y[i];
      x[i] = x[j];
      y[i] = y[j];
      x[j] = tx;
      y[j] = ty;
    }
    k = i2;
    while (k <= j) {
      j -= k;
      k >>= 1;
    }
    j += k;
  }

  /* Compute the FFT */
  c1 = -1.0; 
  c2 = 0.0;
  l2 = 1;
  for (l=0;l<m;l++) {
    l1 = l2;
    l2 <<= 1;
    u1 = 1.0; 
    u2 = 0.0;
    for (j=0;j<l1;j++) {
      for (i=j;i<n;i+=l2) {
	i1 = i + l1;
	t1 = u1 * x[i1] - u2 * y[i1];
	t2 = u1 * y[i1] + u2 * x[i1];
	x[i1] = x[i] - t1; 
	y[i1] = y[i] - t2;
	x[i] += t1;
	y[i] += t2;
      }
      z =  u1 * c1 - u2 * c2;
      u2 = u1 * c2 + u2 * c1;
      u1 = z;
    }
    c2 = sqrt((1.0 - c1) / 2.0);
    if (dir == 1) 
      c2 = -c2;
    c1 = sqrt((1.0 + c1) / 2.0);
  }

  /* Scaling for forward transform */
  if (dir == 1) {
    for (i=0;i<n;i++) {
      x[i] /= n;
      y[i] /= n;
    }
  }

} 

/*
   This computes an in-place complex-to-complex DFT 
   x and y are the real and imaginary arrays of points.
   dir =  1 gives forward transform
   dir = -1 gives reverse transform 
*/
void rf1Ddft(vector<float>& signal, vector<float>& imaginary, int32_t dir)
{
  uint32_t m = static_cast<uint32_t>(signal.size());
  assert(m == imaginary.size());
  float arg;
  float cosarg, sinarg;
  vector<float> sigSum(m), imagSum(m);
  vector<float>::iterator si = signal.begin();
  vector<float>::iterator ii = imaginary.begin();
  vector<float>::iterator ssi = sigSum.begin();
  vector<float>::iterator isi = imagSum.begin();

  for (uint32_t i = 0; i < m; i++) {
    ssi[i] = 0;
    isi[i] = 0;
    arg = - dir * 2.0 * 3.141592654 * (float)i / (float)m;
    for (uint32_t k = 0; k < m; k++) {
      cosarg = cos(k * arg);
      sinarg = sin(k * arg);
      ssi[i] += (si[k] * cosarg - ii[k] * sinarg);
      isi[i] += (si[k] * sinarg + ii[k] * cosarg);
    }
  }

  /* Copy the data back */
  if (dir == 1) {
    for (uint32_t i = 0; i < m; i++) {
      si[i] = ssi[i] / (float)m;
      ii[i] = isi[i] / (float)m;
    }
  } else {
    for (uint32_t i = 0; i < m; i++) {
      si[i] = ssi[i];
      ii[i] = isi[i];
    }
  }
}

#if 0
void rfFrequencyHistogram (vector<float>& signal, vector<double>& histogram)
{

      COMPLEX_SPLIT cmplxSig, cmplRes;
      FFTSetup      setupReal;
      int32_t        log2n;
      int32_t        i, n, nOver2;
      int32_t        stride, cmplResStride;
      
      // Set the size of FFT.
      n = signal.size();
      log2n = (int32_t) log2 ((double) signal.size());
      assert (n == (1 << log2n));

      stride = 1;
      cmplResStride = 1;
      nOver2 = n / 2;                // half of n as real part and imag part.
      
      fprintf (stderr,  "1D real FFT out-of-place of length log2 ( %d ) = %d\n\n", (unsigned int)n, (unsigned int)log2n );
      
      // Allocate memory for the input operands and check its availability,
      // use the vector version to get 16-byte alignment.
      // We use rcWindows since they are guaranteed to be 16 bytes aligned and for similicity of de allocation
      rcWindow realpw (nOver2, 1, rcPixel32);
      rcWindow imagpw (nOver2, 1, rcPixel32);
      rcWindow resrealpw (nOver2, 1, rcPixel32);
      rcWindow resimagpw (nOver2, 1, rcPixel32);

      cmplxSig.realp = ( float* ) realpw.rowPointer (0);
      cmplxSig.imagp = ( float* ) imagpw.rowPointer (0);
      cmplRes.realp = ( float* ) resrealpw.rowPointer (0);
      cmplRes.imagp = ( float* ) resimagpw.rowPointer (0);

      // Look at the real signal as an interleaved complex vector by casting it.
      // Then call the transformation function ctoz to get a split complex vector,
      // which for a real signal, divides into an even-odd configuration.
      ctoz ( ( COMPLEX * ) &signal[0], 2, &cmplxSig, 1, nOver2 );

      // Set up the required memory for the FFT routines and check its availability.
      setupReal = create_fftsetup ( log2n, FFT_RADIX2);
      assert ( setupReal != NULL);
        
      // Carry out a Forward and Inverse FFT transform. It's very important to remember that this is an out-of-place transform. 
      // Therefore, cmplRes will contain the intermediate result which is the result of forward transform and the result from the
      // inverse transform will be stored again in cmplRes.
      fft_zrop ( setupReal, &cmplxSig, stride, &cmplRes, cmplResStride, log2n, FFT_FORWARD );

      // Allocate frequency histogram space
      histogram.resize (n/2);

      vector<double>::iterator rp = histogram.begin();

      // Write out the ABS ( of Fourier response )
      for ( i = 0; i < n/2; i++, rp++ )
	{
	  *rp = sqrt (rmSquare (cmplRes.realp[i] / (2*sqrt((double)n))) + rmSquare (cmplRes.imagp[i]/(2*sqrt((double)n))));
        }

      // Free the allocated memory.
      destroy_fftsetup ( setupReal );

}
#endif

#define PRT_DEBUG1 0
#define PRT_DEBUG2 0
#define PRT_DEBUG3 0

void rf1DFourierAnalysis(const deque<double>& signal,
			 rs1DFourierResult& rslt, uint32_t ctrl)
{
    const uint32_t sigSz = static_cast<uint32_t>(signal.size());

  if (PRT_DEBUG1) {
    fprintf(stderr, "Ctrl Flags: Force FFT %s Zero Fill %s "
	    "Raised Cosine %s\nInput signal:\n",
	    ((ctrl & rc1DFourierForceFFT) ? "TRUE" : "FALSE"),
	    ((ctrl & rc1DFourierZeroFill) ? "TRUE" : "FALSE"),
	    ((ctrl & rc1DFourierRaisedCosine) ? "TRUE" : "FALSE"));
    for (uint32_t i = 0; i < sigSz; i++)
      fprintf(stderr, "%d: %0.25f\n", i, signal[i]);
  }

  if (sigSz == 0) {
    rslt.real.resize(0);
    rslt.imag.resize(0);
    rslt.phase.resize(0);
    rslt.amplitude.resize(0);
    rslt.peakFrequency = 0.0;
    rslt.peakPhase = 0.0;
    rslt.peakAmplitude = 0.0;
    rslt.workingSz = (int32_t)sigSz;
    return;
  }

  /* Step 1 - Calculate the working signal size.
   */
  uint32_t workingSz = sigSz;
  uint32_t index;
  for (index = 0; index < 31; index++)
    if ((uint32_t)(1 << index) >= sigSz)
      break;

  assert(index != 31);

  if ((ctrl & rc1DFourierForceFFT) && (ctrl & rc1DFourierZeroFill)) {
    index++;
    assert(index != 31);
    workingSz = (1 << index);
  }
  else if (ctrl & rc1DFourierForceFFT) {
    workingSz = (1 << index);
  }
  else if (ctrl & rc1DFourierZeroFill) {
    workingSz = workingSz * 2;
  }

  rslt.workingSz = (int32_t)workingSz;
  vector<double> workingSignal(workingSz);

  /* Step 2 -Figure out the signal's DC component.
   */
  double avg = 0.0;
  for (uint32_t i = 0; i < sigSz; i++)
    avg += signal[i];
  
  avg = avg/sigSz;

  /* Step 3 - Create a modified version of the signal by: A)
   * subtracting out the DC component and (optionally) B) applying the
   * Hanning filter.
   */
  if ((ctrl & rc1DFourierRaisedCosine) && sigSz > 1) {
    const double invSz = 1.0/(sigSz-1);
    for (uint32_t n = 0; n < sigSz; n++) {
      double wgt = (1.0 - cos(svl::constants::two_pi*n*invSz))*0.5;
      workingSignal[n] = wgt*(signal[n] - avg);
    }
  }
  else {
    for (uint32_t n = 0; n < sigSz; n++)
      workingSignal[n] = signal[n] - avg;
  }

  /* Step 4 - Do any required zero filling.
   */
  for (uint32_t n = sigSz; n < workingSz; n++)
    workingSignal[n] = 0.0;

  if (rslt.real.size() != workingSz)
    rslt.real.resize(workingSz);

  if (rslt.imag.size() != workingSz)
    rslt.imag.resize(workingSz);

  for (uint32_t ii = 0; ii < workingSz; ii++) {
    rslt.real[ii] = (float)workingSignal[ii];
    rslt.imag[ii] = 0.0;
  }

  if (PRT_DEBUG2) {
    fprintf(stderr, "\nWorking signal:\n");
    for (uint32_t i = 0; i < workingSz; i++)
      fprintf(stderr, "%d: %0.25f\n", i, rslt.real[i]);
  }

  if (workingSz != (uint32_t)(1 << index)) // signal size is not a multiple of 2
    rf1Ddft(rslt.real, rslt.imag, 1);
  else
    rf1Dfft(rslt.real, rslt.imag, 1);

  uint32_t rsltSz = (workingSz + 1)/2;

  if (rslt.phase.size() != rsltSz)
    rslt.phase.resize(rsltSz);

  if (rslt.amplitude.size() != rsltSz)
    rslt.amplitude.resize(rsltSz);
  
  rslt.amplitude[0] = sqrt(rslt.real[0]*rslt.real[0] + rslt.imag[0]*rslt.imag[0]);
  rslt.phase[0] = atan2(rslt.imag[0], rslt.real[0]);
  if (rslt.phase[0] < 0.0)
    rslt.phase[0] += svl::constants::two_pi;

  if (rsltSz == 1) {
    rslt.peakFrequency = 0.0;
    rslt.peakPhase = 0.0;
    rslt.peakAmplitude = 0.0;
    return;
  }

  rslt.amplitude[1] = sqrt(rslt.real[1]*rslt.real[1] + rslt.imag[1]*rslt.imag[1]);
  rslt.phase[1] = atan2(rslt.imag[1], rslt.real[1]);
  if (rslt.phase[1] < 0.0)
    rslt.phase[1] += svl::constants::two_pi;

  if (rsltSz == 2) {
    rslt.peakFrequency = 1;
    rslt.peakPhase = rslt.phase[1];
    rslt.peakAmplitude = rslt.amplitude[1];
    return;
  }

  uint32_t maxFreq = 1;
  float maxAmplitude = rslt.amplitude[1];
  
  for (uint32_t ii = 2; ii < rsltSz; ii++) {
    rslt.amplitude[ii] =
      sqrt(rslt.real[ii]*rslt.real[ii] + rslt.imag[ii]*rslt.imag[ii]);
    rslt.phase[ii] = atan2(rslt.imag[ii], rslt.real[ii]);
    if (rslt.phase[ii] < 0.0)
      rslt.phase[ii] += svl::constants::two_pi;

    if (rslt.amplitude[ii] > maxAmplitude) {
      maxAmplitude = rslt.amplitude[ii];
      maxFreq = ii;
    }
  }

  if (PRT_DEBUG3) {
    const float rkDegree = 180/svl::constants::pi;
    fprintf(stderr, "\nResult signal (FREQ PHASE AMPLITUDE):\n");
    for (uint32_t i = 0; i < rslt.phase.size(); i++)
      fprintf(stderr, " (%d  %f    %f)\n", 
	      i, rslt.phase[i]*rkDegree, rslt.amplitude[i]);
  }

  float frequencyDelta;
  
  if (maxFreq == (rsltSz-1))  { // Peak is pegged at end
    frequencyDelta = parabolicFit(rslt.amplitude[maxFreq-1],
				  rslt.amplitude[maxFreq],
				  (float)0.0,
				  &maxAmplitude);
    assert(frequencyDelta <= 0.0);
  }
  else if (maxFreq == 1) { // Peak is pegged at start
    frequencyDelta = parabolicFit((float)0.0,
				  rslt.amplitude[1],
				  rslt.amplitude[2],
				  &maxAmplitude);
    assert(frequencyDelta >= 0.0);
  }
  else { // Interpolate peak location
    frequencyDelta = parabolicFit(rslt.amplitude[maxFreq-1],
				  rslt.amplitude[maxFreq],
				  rslt.amplitude[maxFreq+1],
				  &maxAmplitude);
  }

  rslt.peakFrequency = maxFreq + frequencyDelta;
  rslt.peakAmplitude = maxAmplitude;
  
  if (frequencyDelta == 0.0) {
    rslt.peakPhase = rslt.phase[maxFreq];
    return;
  }
  
  float theta1, theta2;
  if (frequencyDelta > 0.0) {
    theta1 = rslt.phase[maxFreq];
    theta2 = rslt.phase[maxFreq + 1];
  }
  else {
    frequencyDelta += 1.0;
    theta1 = rslt.phase[maxFreq - 1];
    theta2 = rslt.phase[maxFreq];
  }
  
  float thetaDiff = theta2 - theta1;
  if (thetaDiff < 0)
    thetaDiff += svl::constants::two_pi;
  
  rslt.peakPhase = theta1 + thetaDiff*frequencyDelta;
  if (rslt.peakPhase >= svl::constants::two_pi)
    rslt.peakPhase -= svl::constants::two_pi;
}



void rf1DFourierAnalysisVoxels(const uint8_t* voxel, const uint32_t voxelSz,
			       rsAnalysisVoxels& analysisInfo,
			       rs1DFourierResult& rslt, const uint32_t ctrl)
{
  assert(voxel);

  if (PRT_DEBUG1) {
    fprintf(stderr, "Ctrl Flags: Force FFT %s Zero Fill %s "
	    "Raised Cosine %s Generate Phase %s\nInput signal:\n",
	    ((ctrl & rc1DFourierForceFFT) ? "TRUE" : "FALSE"),
	    ((ctrl & rc1DFourierZeroFill) ? "TRUE" : "FALSE"),
	    ((ctrl & rc1DFourierRaisedCosine) ? "TRUE" : "FALSE"),
	    ((ctrl & rc1DFourierGenPhase) ? "TRUE" : "FALSE"));
    for (uint32_t i = 0; i < voxelSz; i++)
      fprintf(stderr, "%d: %d\n", i, voxel[i]);
  }

  if (voxelSz == 0) {
    rslt.real.resize(0);
    rslt.imag.resize(0);
    rslt.phase.resize(0);
    rslt.amplitude.resize(0);
    rslt.peakFrequency = 0.0;
    rslt.peakPhase = 0.0;
    rslt.peakAmplitude = 0.0;
    rslt.workingSz = (int32_t)voxelSz;
    return;
  }

  /* Step 1 - Calculate the working signal size.
   */
  uint32_t workingSz = voxelSz;
  uint32_t index;
  for (index = 0; index < 31; index++)
    if ((uint32_t)(1 << index) >= voxelSz)
      break;

  assert(index != 31);

  assert(ctrl & rc1DFourierForceFFT);

  if (ctrl & rc1DFourierZeroFill) {
    index++;
    assert(index != 31);
    workingSz = (1 << index);
  }
  else
    workingSz = (1 << index);
  
  rslt.workingSz = (int32_t)workingSz;
  if (analysisInfo.workSigSz < workingSz) {
    if (analysisInfo.workSigBase) {
      free(analysisInfo.workSigBase);
      analysisInfo.workSigBase = 0;
      analysisInfo.workSig = 0;
    }
    analysisInfo.workSigSz = workingSz;
  }
  
  if (!analysisInfo.workSigBase) {
    analysisInfo.workSigBase =
      (char*)malloc(analysisInfo.workSigSz*sizeof(float) + 16);
    assert(analysisInfo.workSigBase);
    analysisInfo.workSig =
      (float*)(((int)analysisInfo.workSigBase+15) & 0xFFFFFFF0);
  }

  /* Step 2 -Figure out the voxel's DC component.
   */
  double avg = 0.0;
  for (uint32_t i = 0; i < voxelSz; i++)
    avg += voxel[i];
  
  avg = avg/voxelSz;

  /* Step 3 - Create a modified version of the voxel by: A)
   * subtracting out the DC component and (optionally) B) applying the
   * Hanning filter.
   */
  if ((ctrl & rc1DFourierRaisedCosine) && voxelSz > 1) {
    if (analysisInfo.weightsSz != voxelSz) {
      if (analysisInfo.weights)
	free(analysisInfo.weights);
      analysisInfo.weights = (float*)malloc(voxelSz*sizeof(float));
      assert(analysisInfo.weights);
      analysisInfo.weightsSz = voxelSz;

      const float invSz = 1.0/(voxelSz-1);
      for (uint32_t n = 0; n < voxelSz; n++)
	analysisInfo.weights[n] = (1.0 - cos(svl::constants::two_pi*n*invSz))*0.5;
    }
    else
      assert(analysisInfo.weights);

    for (uint32_t n = 0; n < voxelSz; n++)
      analysisInfo.workSig[n] = analysisInfo.weights[n]*(voxel[n] - avg);
  }
  else {
    for (uint32_t n = 0; n < voxelSz; n++)
      analysisInfo.workSig[n] = voxel[n] - avg;
  }

  /* Step 4 - Do any required zero filling.
   */
  for (uint32_t n = voxelSz; n < workingSz; n++)
    analysisInfo.workSig[n] = 0.0;

  if (PRT_DEBUG2) {
    fprintf(stderr, "\nWorking signal:\n");
    for (uint32_t i = 0; i < workingSz; i++)
      fprintf(stderr, "%d: %0.25f\n", i, analysisInfo.workSig[i]);
  }
  
  assert(analysisInfo.workSig);
  
  int32_t retval = FFTRealForward(analysisInfo.workSig, workingSz);
  if (retval != 0)
    fprintf(stderr, "Retval %d != 0\n", retval);
  assert(retval == 0);

  const uint32_t rsltSz = workingSz/2;

  if (rslt.real.size() != rsltSz)
    rslt.real.resize(rsltSz);

  if (rslt.imag.size() != rsltSz)
    rslt.imag.resize(rsltSz);

  {
    uint32_t ri = 0;
    for (uint32_t wi = 0; wi < rsltSz; wi += 2, ri++) {
      rslt.real[ri] = analysisInfo.workSig[wi];
      rslt.imag[ri] = analysisInfo.workSig[wi+1];
    }
  }

  if ((ctrl & rc1DFourierGenPhase) && (rslt.phase.size() != rsltSz))
      rslt.phase.resize(rsltSz);

  if (rslt.amplitude.size() != rsltSz)
    rslt.amplitude.resize(rsltSz);
  
  rslt.amplitude[0] = sqrt(rslt.real[0]*rslt.real[0] + rslt.imag[0]*rslt.imag[0]);
  if (ctrl & rc1DFourierGenPhase) {
    rslt.phase[0] = atan2(rslt.imag[0], rslt.real[0]);
    if (rslt.phase[0] < 0.0)
      rslt.phase[0] += svl::constants::two_pi;
  }

  if (rsltSz == 1) {
    rslt.peakFrequency = 0.0;
    rslt.peakPhase = 0.0;
    rslt.peakAmplitude = 0.0;
    return;
  }

  rslt.amplitude[1] = sqrt(rslt.real[1]*rslt.real[1] + rslt.imag[1]*rslt.imag[1]);
  if (ctrl & rc1DFourierGenPhase) {
    rslt.phase[1] = atan2(rslt.imag[1], rslt.real[1]);
    if (rslt.phase[1] < 0.0)
      rslt.phase[1] += svl::constants::two_pi;
  }

  if (rsltSz == 2) {
    rslt.peakFrequency = 1;
    if (ctrl & rc1DFourierGenPhase) rslt.peakPhase = rslt.phase[1];
    rslt.peakAmplitude = rslt.amplitude[1];
    return;
  }

  uint32_t maxFreq = 1;
  float maxAmplitude = rslt.amplitude[1];

  if (ctrl & rc1DFourierGenPhase) {
    for (uint32_t ii = 2; ii < rsltSz; ii++) {
      rslt.amplitude[ii] =
	sqrt(rslt.real[ii]*rslt.real[ii] + rslt.imag[ii]*rslt.imag[ii]);
      rslt.phase[ii] = atan2(rslt.imag[ii], rslt.real[ii]);
      if (rslt.phase[ii] < 0.0)
	rslt.phase[ii] += svl::constants::two_pi;
      
      if (rslt.amplitude[ii] > maxAmplitude) {
	maxAmplitude = rslt.amplitude[ii];
	maxFreq = ii;
      }
    }
  }
  else {
    for (uint32_t ii = 2; ii < rsltSz; ii++) {
      rslt.amplitude[ii] =
	sqrt(rslt.real[ii]*rslt.real[ii] + rslt.imag[ii]*rslt.imag[ii]);
      
      if (rslt.amplitude[ii] > maxAmplitude) {
	maxAmplitude = rslt.amplitude[ii];
	maxFreq = ii;
      }
    }
  }

  if (PRT_DEBUG3) {
    const float rkDegree = 180/svl::constants::pi;
    fprintf(stderr, "\nResult signal (FREQ PHASE AMPLITUDE):\n");
    for (uint32_t i = 0; i < rslt.phase.size(); i++)
      fprintf(stderr, " (%d  %f    %f)\n", 
	      i, 
	      ((ctrl & rc1DFourierGenPhase) ? rslt.phase[i]*rkDegree : 0.0),
	      rslt.amplitude[i]);
  }

  float frequencyDelta;
  
  if (maxFreq == (rsltSz-1))  { // Peak is pegged at end
    frequencyDelta = parabolicFit(rslt.amplitude[maxFreq-1],
				  rslt.amplitude[maxFreq],
				  (float)0.0,
				  &maxAmplitude);
    assert(frequencyDelta <= 0.0);
  }
  else if (maxFreq == 1) { // Peak is pegged at start
    frequencyDelta = parabolicFit((float)0.0,
				  rslt.amplitude[1],
				  rslt.amplitude[2],
				  &maxAmplitude);
    assert(frequencyDelta >= 0.0);
  }
  else { // Interpolate peak location
    frequencyDelta = parabolicFit(rslt.amplitude[maxFreq-1],
				  rslt.amplitude[maxFreq],
				  rslt.amplitude[maxFreq+1],
				  &maxAmplitude);
  }

  rslt.peakFrequency = maxFreq + frequencyDelta;
  rslt.peakAmplitude = maxAmplitude;
  
  if ((ctrl & rc1DFourierGenPhase) == 0) {
    rslt.peakPhase = 0.0;
    return;
  }
  else if (frequencyDelta == 0.0) {
    rslt.peakPhase = rslt.phase[maxFreq];
    return;
  }
  
  float theta1, theta2;
  if (frequencyDelta > 0.0) {
    theta1 = rslt.phase[maxFreq];
    theta2 = rslt.phase[maxFreq + 1];
  }
  else {
    frequencyDelta += 1.0;
    theta1 = rslt.phase[maxFreq - 1];
    theta2 = rslt.phase[maxFreq];
  }
  
  float thetaDiff = theta2 - theta1;
  if (thetaDiff < 0)
    thetaDiff += svl::constants::two_pi;
  
  rslt.peakPhase = theta1 + thetaDiff*frequencyDelta;
  if (rslt.peakPhase >= svl::constants::two_pi)
    rslt.peakPhase -= svl::constants::two_pi;
}

float interPeak(const vector<float>& signal, uint32_t index, float* ampP)
{
  float delta;
  if (index == (signal.size()-1))  { // Peak is pegged at end
    delta = parabolicFit(signal[index-1], signal[index], (float)0.0, ampP);
    assert(delta <= 0.0);
  }
  else if (index == 0) { // Peak is pegged at start
    delta = parabolicFit((float)0.0, signal[0], signal[1], ampP);
    assert(delta >= 0.0);
  }
  else { // Interpolate peak location
    delta = parabolicFit(signal[index-1], signal[index], signal[index+1], ampP);
  }

  return index + delta;
}

