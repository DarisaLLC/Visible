//
//  voxel_frequency.h
//  Visible
//
//  Created by Arman Garakani on 3/20/19.
//

#ifndef voxel_frequency_h
#define voxel_frequency_h

#include <vector>
#include <algorithm>

#include <stdio.h>
#include <Accelerate/Accelerate.h>
#include "core/core.hpp"

using namespace std;

/* @note Does not subtract DC component
 * incomplete API
 */


class fft1D
{
public:
    
    fft1D (const std::vector<float>& src){
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
    
    ~fft1D() { vDSP_destroy_fftsetup(m_opeque); }
    
    
    const vDSP_Length& nN() { return m_length; }
    const vDSP_Length& logN() { return m_log_length;}
    const std::vector<float>& phase () const { return m_phase; }
    const std::vector<float>& magnitude () const { return m_mag; }
    const std::vector<float>& spectrum_realp () const { return  m_split_realp; }
    const std::vector<float>& spectrum_imagp () const { return  m_split_imagp; }
    
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
//        printf("\nSpectrum:\n");
//        for (int k = 0; k < m_half_length; k++)
//        {
//            printf("%3d\t%6.2f\t%6.2f\n", k, m_tempSplitComplex.realp[k], m_tempSplitComplex.imagp[k]);
//        }
        
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
//
//        printf("\nMag / Phase:\n");
//        for (int k = 1; k < m_half_length; k++)
//        {
//            printf("%3d\t%6.2f\t%6.2f\n", k, m_mag[k], m_phase[k]);
//        }
    }
    
    static bool test (){
        std::vector<float> tst;
        auto gen = [](std::vector<float>& vv){
            int N = 16;
            vv.resize(N);
            // Initialize the input buffer with a sinusoid
            int BIN = 3;
            for (int k = 0; k < N; k++)
                vv[k] = cos(2*svl::constants::pi*BIN*k/N);
        };
        gen(tst);
        fft1D f1(tst);
        f1.compute_spectrum();
        float feps = 1.e-6;
        bool tst_1 = svl::equal(16.0f,f1.spectrum_realp()[3], feps);
        f1.compute_phase_mag();
        bool tst_2 = svl::equal(16.0f,f1.magnitude()[3], feps);
        bool tst_3 = svl::equal(0.0f,f1.phase()[3], feps);
        return tst_1 && tst_2 && tst_3;
    }
    
private:

    FFTSetup m_opeque;
    vDSP_Length m_length;
    vDSP_Length m_half_length;
    vDSP_Length m_log_length;
    mutable std::vector<float> m_x, m_y, m_split_realp, m_split_imagp, m_mag, m_phase;
    // We need complex buffers in two different formats!
    std::vector<DSPComplex> m_tempComplex;
    DSPSplitComplex m_tempSplitComplex;

    
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
    vDSP_Length log2max(size_t n)
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
    
};


#endif /* voxel_frequency_h */
