//  main.cpp
//  AccelerateFFT
//
//	Demo of how to use Apple's blazingly fast but maddeningly confusing
//	real->complex and complex->real FFT functions.
//
//  Created by Gerry Beauregard (g.beauregard [at] ieee.org) on 2013-01-28.
//
//	Use this code however you like. No credit required, but if this code
//	saves you some hair-pulling, a hat-tip and kind comment is always
// 	appreciated 😉

#include <vector>
#include <algorithm>

#include <stdio.h>
#include <Accelerate/Accelerate.h>
using namespace std;
const int LOG_N = 9; // Typically this would be at least 10 (i.e. 1024pt FFTs)
const int N = 1 << LOG_N;
const float PI = 4*atan(1);


////////////////////////////////////////////////////////////////////////////////
//
//    log2max
//
//    This function computes the ceiling of log2(n).  For example:
//
//    log2max(7) = 3
//    log2max(8) = 3
//    log2max(9) = 4
//
////////////////////////////////////////////////////////////////////////////////
static  vDSP_Length log2max(size_t n)
{
    vDSP_Length power = 1;
    int32_t k = 1;
    
    if (n==1) {
        return 0;
    }
    
    while ((k <<= 1) < n) {
        power++;
    }
    
    return power;
}
std::vector<int> v1 = {118,117,121,118,125,126,117,123,120,119,118,121,116,119,123,122,120,122,116,113,122,118,118,118,116,120,124,123,117,120,123,118,118,119,120,121,116,123,119,118,119,116,123,123,116,120,117,118,121,121,120,121,121,124,119,121,120,120,120,119,121,122,120,126,119,121,122,119,125,121,119,123,124,123,118,124,122,119,121,116,121,118,120,116,122,115,117,122,117,121,119,119,112,117,121,119,123,121,117,126,118,121,120,124,119,120,124,124,123,117,119,119,116,120,118,117,119,115,122,118,121,120,121,120,127,122,118,119,116,117,123,121,122,118,114,121,123,119,123,119,118,118,119,115,119,117,119,129,119,125,124,117,120,127,122,118,121,118,120,119,116,119,124,121,123,117,123,124,123,121,118,124,123,122,119,117,121,122,122,115,122,119,119,121,118,124,119,120,118,115,119,119,118,123,119,119,116,119,120,123,120,115,118,120,126,122,124,117,117,124,121,119,115,122,119,120,116,117,119,117,116,117,118,120,117,118,118,114,120,116,117,122,116,120,120,115,117,121,117,113,114,117,115,110,114,112,113,115,111,111,111,115,111,109,115,108,113,108,103,101,103,105,104,103,102,99,104,101,101,103,105,100,104,108,104,104,106,104,111,104,101,106,108,110,109,108,106,111,113,111,109,111,117,116,114,111,111,107,116,113,109,110,112,116,111,110,116,110,109,111,114,117,114,110,113,111,120,112,113,110,111,112,118,119,114,115,116,113,115,115,124,114,112,113,118,115,112,116,117,120,114,109,115,114,114,118,116,116,117,114,113,114,118,114,115,114,115,117,113,109,116,119,115,117,113,113,118,115,114,114,117,112,113,111,116,113,111,116,115,119,118,119,117,115,119,119,116,109,114,115,117,119,117,111,120,116,116,115,115,118,117,113,116,117,118,116,115,119,118,117,113,116,118,121,117,113,119,120,120,117,118,119,125,119,120,118,118,121,117,121,117,119,120,115,118,119,120,120,118,115,123,119,120,115,114,119,112,124,120,118,124,122,117,121,121,114,122,122,118,120,120,121,119,114,120,119,114,121,122,118,110,117,120,124,122,117,122,119,113,117,118,124,117,118,116,116,120,116,118,118,118,118,116,120,118,115,124,113,118,    128};

class OneDfftWorker
{
public:
    
    OneDfftWorker (const std::vector<float>& src){
        auto sz = src.size();
        m_log_length = log2max(sz);
        m_length = 1 << m_log_length;
        m_half_length = m_length / 2;
        assert(m_length == sz);
        m_opeque = vDSP_create_fftsetup(m_log_length, 2);
        m_x = src;
        m_y = m_x;
        m_split_realp.resize(m_half_length);
        m_split_imagp.resize(m_half_length);
        m_mag.resize(m_half_length);
        m_phase.resize(m_half_length);
        m_tempComplex.resize(m_half_length);
        m_tempSplitComplex.realp = &m_split_realp[0];
        m_tempSplitComplex.imagp = &m_split_imagp[0];
    }

    ~OneDfftWorker() { vDSP_destroy_fftsetup(m_opeque); }
    
    
    const vDSP_Length& nN() { return m_length; }
    const vDSP_Length& logN() { return m_log_length;}

 
    void compute_spectrum (){
        // ----------------------------------------------------------------
        // Forward FFT
        
        // Scramble-pack the real data into complex buffer in just the way that's
        // required by the real-to-complex FFT function that follows.
        vDSP_ctoz((DSPComplex*)(&m_x[0]), 2, &m_tempSplitComplex, 1, m_half_length);
        
        // Do real->complex forward FFT
        vDSP_fft_zrip(m_opeque, &m_tempSplitComplex, 1, m_log_length, kFFTDirection_Forward);
        
        // Print the complex spectrum. Note that since it's the FFT of a real signal,
        // the spectrum is conjugate symmetric, that is the negative frequency components
        // are complex conjugates of the positive frequencies. The real->complex FFT
        // therefore only gives us the positive half of the spectrum from bin 0 ("DC")
        // to bin N/2 (Nyquist frequency, i.e. half the sample rate). Typically with
        // audio code, you don't need to worry much about the DC and Nyquist values, as
        // they'll be very close to zero if you're doing everything else correctly.
        //
        // Bins 0 and N/2 both necessarily have zero phase, so in the packed format
        // only the real values are output, and these are stuffed into the real/imag components
        // of the first complex value (even though they are both in fact real values). Try
        // replacing BIN above with N/2 to see how sinusoid at Nyquist appears in the spectrum.
        printf("\nSpectrum:\n");
        for (int k = 0; k < m_half_length; k++)
        {
            printf("%3d\t%6.2f\t%6.2f\n", k, m_tempSplitComplex.realp[k], m_tempSplitComplex.imagp[k]);
        }

    }
    void compute_phase_mag (){
        // ----------------------------------------------------------------
        // Convert from complex/rectangular (real, imaginary) coordinates
        // to polar (magnitude and phase) coordinates.
        
        // Compute magnitude and phase. Can also be done using vDSP_polar.
        // Note that when printing out the values below, we ignore bin zero, as the
        // real/complex values for bin zero in tempSplitComplex actually both correspond
        // to real spectrum values for bins 0 (DC) and N/2 (Nyquist) respectively.
        vDSP_zvabs(&m_tempSplitComplex, 1, &m_mag[0], 1, m_half_length);
        vDSP_zvphas(&m_tempSplitComplex, 1, &m_phase[0], 1, m_half_length);
        
        printf("\nMag / Phase:\n");
        for (int k = 1; k < m_half_length; k++)
        {
            printf("%3d\t%6.2f\t%6.2f\n", k, m_mag[k], m_phase[k]);
        }


    }
    
private:
    FFTSetup m_opeque;
    vDSP_Length m_length;
    vDSP_Length m_half_length;
    vDSP_Length m_log_length;
    std::vector<float> m_x, m_y, m_split_realp, m_split_imagp, m_mag, m_phase;
    // We need complex buffers in two different formats!
    std::vector<DSPComplex> m_tempComplex;
    DSPSplitComplex m_tempSplitComplex;
    
};

int main(int argc, const char * argv[])
{
//    vector<float> data(N);
    {
//        int BIN = 2;
//        for (int k = 0; k < N; k++)
//            data[k] = cos(2*PI*BIN*k/N);
        vector<float> data;
        double sum = 0;
        float vmin, vmax;
        vmax = 0;
        vmin = 255;
        for (auto intv : v1){
            float vf = (float)intv;
            if (vf > vmax) vmax = vf;
            if (vf < vmin) vmin = vf;
        }
        float vrange = vmax - vmin;
        vmin = 0.0f;
        vrange = 255.0f;
        for (auto intv : v1){
            float vf = (float)intv;
            vf = (vf - vmin)/vrange;
            sum += vf;
        }
        float vmean = sum / v1.size();
        for (auto intv : v1){
            float vf = (float)intv;
            vf = (vf - vmin)/vrange;
            vf = vf - vmean;
            data.push_back(vf);
        }
        auto p2 = log2max(data.size());
        auto pad = (1 << p2) - data.size();
        for(auto pp = 0; pp < pad; pp++ ){
            data.push_back(0.0f);
        }
        
        OneDfftWorker worker (data);
        worker.compute_spectrum();
        worker.compute_phase_mag();
        
        
    }
    
	// Set up a data structure with pre-calculated values for
	// doing a very fast FFT. The structure is opaque, but presumably
	// includes sin/cos twiddle factors, and a lookup table for converting
	// to/from bit-reversed ordering. Normally you'd create this once
	// in your application, then use it for many (hundreds! thousands!) of
	// forward and inverse FFTs.
    FFTSetup fftSetup = vDSP_create_fftsetup(LOG_N, kFFTRadix2);

	// -------------------------------
	// Set up a bunch of buffers

	// Buffers for real (time-domain) input and output signals.
	float *x = new float[N];
	float *y = new float[N];

	// Initialize the input buffer with a sinusoid
	int BIN = 3;
	for (int k = 0; k < N; k++)
		x[k] = cos(2*PI*BIN*k/N);

	// We need complex buffers in two different formats!
	DSPComplex *tempComplex = new DSPComplex[N/2];

	DSPSplitComplex tempSplitComplex;
	tempSplitComplex.realp = new float[N/2];
	tempSplitComplex.imagp = new float[N/2];

	// For polar coordinates
	float *mag = new float[N/2];
	float *phase = new float[N/2];

	// ----------------------------------------------------------------
	// Forward FFT

	// Scramble-pack the real data into complex buffer in just the way that's
	// required by the real-to-complex FFT function that follows.
	vDSP_ctoz((DSPComplex*)x, 2, &tempSplitComplex, 1, N/2);

	// Do real->complex forward FFT
	vDSP_fft_zrip(fftSetup, &tempSplitComplex, 1, LOG_N, kFFTDirection_Forward);

	// Print the complex spectrum. Note that since it's the FFT of a real signal,
	// the spectrum is conjugate symmetric, that is the negative frequency components
	// are complex conjugates of the positive frequencies. The real->complex FFT
	// therefore only gives us the positive half of the spectrum from bin 0 ("DC")
	// to bin N/2 (Nyquist frequency, i.e. half the sample rate). Typically with
	// audio code, you don't need to worry much about the DC and Nyquist values, as
	// they'll be very close to zero if you're doing everything else correctly.
	//
	// Bins 0 and N/2 both necessarily have zero phase, so in the packed format
	// only the real values are output, and these are stuffed into the real/imag components
	// of the first complex value (even though they are both in fact real values). Try
	// replacing BIN above with N/2 to see how sinusoid at Nyquist appears in the spectrum.
	printf("\nSpectrum:\n");
	for (int k = 0; k < N/2; k++)
	{
		printf("%3d\t%6.2f\t%6.2f\n", k, tempSplitComplex.realp[k], tempSplitComplex.imagp[k]);
	}

	// ----------------------------------------------------------------
	// Convert from complex/rectangular (real, imaginary) coordinates
	// to polar (magnitude and phase) coordinates.

	// Compute magnitude and phase. Can also be done using vDSP_polar.
	// Note that when printing out the values below, we ignore bin zero, as the
	// real/complex values for bin zero in tempSplitComplex actually both correspond
	// to real spectrum values for bins 0 (DC) and N/2 (Nyquist) respectively.
	vDSP_zvabs(&tempSplitComplex, 1, mag, 1, N/2);
	vDSP_zvphas(&tempSplitComplex, 1, phase, 1, N/2);

	printf("\nMag / Phase:\n");
	for (int k = 1; k < N/2; k++)
	{
		printf("%3d\t%6.2f\t%6.2f\n", k, mag[k], phase[k]);
	}

	// ----------------------------------------------------------------
	// Convert from polar coordinates back to rectangular coordinates.

	tempSplitComplex.realp = mag;
	tempSplitComplex.imagp = phase;

	vDSP_ztoc(&tempSplitComplex, 1, tempComplex, 2, N/2);
	vDSP_rect((float*)tempComplex, 2, (float*)tempComplex, 2, N/2);
	vDSP_ctoz(tempComplex, 2, &tempSplitComplex, 1, N/2);

	// ----------------------------------------------------------------
	// Do Inverse FFT

	// Do complex->real inverse FFT.
	vDSP_fft_zrip(fftSetup, &tempSplitComplex, 1, LOG_N, kFFTDirection_Inverse);

	// This leaves result in packed format. Here we unpack it into a real vector.
	vDSP_ztoc(&tempSplitComplex, 1, (DSPComplex*)y, 2, N/2);

	// Neither the forward nor inverse FFT does any scaling. Here we compensate for that.
	float scale = 0.5/N;
	vDSP_vsmul(y, 1, &scale, y, 1, N);

	// Assuming it's all correct, the input x and output y vectors will have identical values
	printf("\nInput & output:\n");
	for (int k = 0; k < N; k++)
	{
		printf("%3d\t%6.2f\t%6.2f\n", k, x[k], y[k]);
	}

    return 0;
}













