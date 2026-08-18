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

// The taps sum to exactly 1.0 (correct steady-state/DC gain), but that
// alone does not bound the filter's worst-case output level on transient
// program material. The true worst-case peak gain for any input bounded
// by +/-1.0 is the L1 norm of the impulse response (sum of absolute tap
// values). Because this filter has negative side lobes, that L1 norm is
// measurably greater than 1.0 (about 1.347, i.e. the wet path can momentarily
// amplify a transient by roughly +2.6 dB above the source). On loud,
// hot-mastered material this can push the filtered (wet) output past
// 0 dBFS, which hard-clips at the DAC - an audible, digital-sounding
// glitch that only ever appears on the wet path, since the dry/bypass
// path is just an unmodified delayed copy of whatever headroom the
// source already had. Scaling the wet output by 1/L1 guarantees it can
// never exceed +/-1.0 for any input within +/-1.0, closing that failure
// mode unconditionally rather than hoping typical program material stays
// under the worst case.
constexpr double makeWetSafetyGain()
{
    double sumAbs = 0.0;
    for (int i = 0; i < numTaps; ++i)
    {
        const double v = taps[static_cast<std::size_t> (i)];
        sumAbs += (v < 0.0 ? -v : v);
    }
    return sumAbs > 1.0 ? (1.0 / sumAbs) : 1.0;
}

inline constexpr double wetSafetyGain = makeWetSafetyGain();
}
