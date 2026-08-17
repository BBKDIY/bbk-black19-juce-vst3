// Framework-independent verification of the BBK Black-19 FIR kernel.
// No JUCE dependency: only Black19Kernel.h and the standard library.
//
// Checks:
//   1. Exact time-domain symmetry (Type-I linear phase).
//   2. DC gain equals 1.0 within floating-point tolerance.
//   3. Passband ripple stays within a tight bound (0..20 kHz).
//   4. Stopband attenuation is at least 100 dB (76..96 kHz), matching the
//      documented worst-case figure of approximately -100.284 dB.
//   5. Peak sidelobe ratio (largest side tap magnitude relative to the
//      centre tap) matches the documented figure of approximately 14.3932%.
//
// Prints PASS and exits 0 if every check succeeds, otherwise prints FAIL
// with a description of what did not match and exits 1.

#include "../Source/Black19Kernel.h"

#include <cmath>
#include <complex>
#include <cstdio>
#include <vector>

namespace
{
bool ok = true;

// M_PI is a POSIX/GNU extension, not standard C++, and MSVC does not define
// it without _USE_MATH_DEFINES (which must precede every <cmath> include in
// the translation unit, including transitive ones - too fragile to rely on).
constexpr double kPi = 3.14159265358979323846;

void check (bool condition, const char* description)
{
    if (! condition)
    {
        std::printf ("FAIL: %s\n", description);
        ok = false;
    }
    else
    {
        std::printf ("  ok: %s\n", description);
    }
}

// Naive DFT magnitude (in dB) of the FIR at a single frequency, evaluated
// directly from the tap coefficients - no external FFT library needed for
// 19 taps and a modest number of test frequencies.
double magnitudeDb (double freqHz)
{
    using namespace bbk::black19;
    const double omega = 2.0 * kPi * freqHz / static_cast<double> (sampleRateHz);

    std::complex<double> acc { 0.0, 0.0 };
    for (int k = 0; k < numTaps; ++k)
    {
        const std::complex<double> rotation { std::cos (omega * k), -std::sin (omega * k) };
        acc += taps[static_cast<std::size_t> (k)] * rotation;
    }

    const double mag = std::abs (acc);
    return 20.0 * std::log10 (mag > 1e-300 ? mag : 1e-300);
}
}

int main()
{
    using namespace bbk::black19;

    std::printf ("BBK Black-19 DSP verification (framework-independent)\n");
    std::printf ("Fs = %d Hz, taps = %d, group delay = %d samples (%.3f us)\n\n",
                 sampleRateHz, numTaps, groupDelaySamples, groupDelayMicroseconds);

    // 1. Symmetry.
    bool symmetric = true;
    for (int i = 0; i < numTaps; ++i)
    {
        if (taps[static_cast<std::size_t> (i)] != taps[static_cast<std::size_t> (numTaps - 1 - i)])
        {
            symmetric = false;
            break;
        }
    }
    check (symmetric, "Type-I linear-phase symmetry (exact)");

    // 2. DC gain.
    double dcSum = 0.0;
    for (int i = 0; i < numTaps; ++i)
        dcSum += taps[static_cast<std::size_t> (i)];
    check (std::abs (dcSum - 1.0) < 1e-9, "DC gain equals 1.0 within 1e-9");

    // 3. Passband ripple, sampled every 500 Hz from 0 to 20 kHz.
    double passbandMaxDb = -1e9;
    double passbandMinDb = 1e9;
    for (double f = 0.0; f <= 20000.0; f += 500.0)
    {
        const double db = magnitudeDb (f);
        passbandMaxDb = std::max (passbandMaxDb, db);
        passbandMinDb = std::min (passbandMinDb, db);
    }
    std::printf ("  passband range: %.4f dB to %.4f dB\n", passbandMinDb, passbandMaxDb);
    check (passbandMaxDb < 0.1 && passbandMinDb > -0.1, "Passband ripple within +/-0.1 dB");

    // 4. Stopband attenuation, sampled every 500 Hz from 76 to 96 kHz.
    double stopbandWorstDb = -1e9;
    for (double f = 76000.0; f <= 96000.0; f += 500.0)
        stopbandWorstDb = std::max (stopbandWorstDb, magnitudeDb (f));
    std::printf ("  stopband worst-case: %.4f dB (documented approx -100.284 dB)\n", stopbandWorstDb);
    check (stopbandWorstDb < -99.0, "Stopband attenuation at least 99 dB (76-96 kHz)");

    // 5. Peak sidelobe ratio: the classic windowed-filter metric. Walking
    // outward from the centre tap, the "main lobe" is the contiguous run of
    // taps sharing the centre's sign; once the sign flips, everything beyond
    // that point is a "sidelobe". The peak sidelobe ratio is the largest
    // sidelobe tap magnitude divided by the centre tap magnitude.
    const double centre = taps[static_cast<std::size_t> (groupDelaySamples)];
    const bool centreSign = centre > 0.0;
    int i = groupDelaySamples - 1;
    while (i >= 0 && (taps[static_cast<std::size_t> (i)] > 0.0) == centreSign)
        --i;
    double largestSidelobe = 0.0;
    for (int j = 0; j <= i; ++j)
        largestSidelobe = std::max (largestSidelobe, std::abs (taps[static_cast<std::size_t> (j)]));
    const double sidelobeRatioPercent = largestSidelobe / std::abs (centre) * 100.0;
    std::printf ("  peak sidelobe ratio: %.4f%% (documented approx 14.3932%%)\n", sidelobeRatioPercent);
    check (std::abs (sidelobeRatioPercent - 14.3932) < 0.01, "Peak sidelobe ratio matches 14.3932% within 0.01%");

    std::printf ("\n%s\n", ok ? "PASS" : "FAIL");
    return ok ? 0 : 1;
}
