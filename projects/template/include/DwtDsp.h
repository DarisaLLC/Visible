#pragma once

#include <string>

namespace dsp {

enum class MotherWavelet
{
    Haar,

    Daubechies1, Daubechies2, Daubechies3,
    Daubechies4, Daubechies5, Daubechies6,
    Daubechies7, Daubechies8, Daubechies9,
    Daubechies10, Daubechies11, Daubechies12,
    Daubechies13, Daubechies14, Daubechies15,
    Daubechies16, Daubechies17, Daubechies18,
    Daubechies19, Daubechies20, Daubechies21,
    Daubechies22, Daubechies23, Daubechies24,
    Daubechies25, Daubechies26, Daubechies27,
    Daubechies28, Daubechies29, Daubechies30,
    Daubechies31, Daubechies32, Daubechies33,
    Daubechies34, Daubechies35, Daubechies36,
    Daubechies37, Daubechies38,

    Bior1_1, Bior1_3, Bior1_5,
    Bior2_2, Bior2_4, Bior2_6, Bior2_8,
    Bior3_1, Bior3_3, Bior3_5, Bior3_7, Bior3_9,
    Bior4_4, Bior5_5, Bior6_8,

    RBior1_1, RBior1_3, RBior1_5,
    RBior2_2, RBior2_4, RBior2_6, RBior2_8,
    RBior3_1, RBior3_3, RBior3_5, RBior3_7, RBior3_9,
    RBior4_4, RBior5_5, RBior6_8,

    Coiflet1, Coiflet2, Coiflet3, Coiflet4,
    Coiflet5, Coiflet6, Coiflet7, Coiflet8,
    Coiflet9, Coiflet10, Coiflet11, Coiflet12,
    Coiflet13, Coiflet14, Coiflet15, Coiflet16,

    Symmlet2, Symmlet3, Symmlet4, Symmlet5,
    Symmlet6, Symmlet7, Symmlet8, Symmlet9,
    Symmlet10, Symmlet11, Symmlet12, Symmlet13,
    Symmlet14, Symmlet15, Symmlet16, Symmlet17,
    Symmlet18, Symmlet19, Symmlet20
};

std::string     WavelettoString(MotherWavelet wavelet);
MotherWavelet   StringtoWavelet(const std::string& wavelet);

}
