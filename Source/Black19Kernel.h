#pragma once

#include <array>
#include <cstddef>

namespace bbk::black19
{
constexpr int sampleRateHz = 192000;
constexpr int numTaps = 19;
constexpr int groupDelaySamples = 9;
constexpr double groupDelayMicroseconds = 46.875;

// 19-tap Type-I linear-phase minimum-peak FIR for the Part Two test geometry:
// Fs = 192 kHz, passband 0..20 kHz, transition 20..76 kHz,
// stopband 76..96 kHz. Coefficients are symmetric and DC-normalised.
//
// The centre tap is derived from the nine unique side taps so the intended
// mathematical DC sum is exactly 1.0 (subject only to floating-point summation).
constexpr std::array<double, numTaps> makeTaps()
{
    std::array<double, numTaps> h
    {
        -9.3505572423227979e-05,
        -1.2020841188186376e-03,
        -2.2503193573389720e-03,
         6.0526536916332202e-03,
         1.6719804155154699e-02,
        -1.5575885148673401e-02,
        -6.7545377703453105e-02,
         2.6084707183493042e-02,
         3.0316698306502737e-01,
         0.0,
         3.0316698306502737e-01,
         2.6084707183493042e-02,
        -6.7545377703453105e-02,
        -1.5575885148673401e-02,
         1.6719804155154699e-02,
         6.0526536916332202e-03,
        -2.2503193573389720e-03,
        -1.2020841188186376e-03,
        -9.3505572423227979e-05
    };

    double sideSum = 0.0;
    for (int i = 0; i < groupDelaySamples; ++i)
        sideSum += h[static_cast<std::size_t> (i)];

    h[static_cast<std::size_t> (groupDelaySamples)] = 1.0 - 2.0 * sideSum;
    return h;
}

inline constexpr auto taps = makeTaps();
}
