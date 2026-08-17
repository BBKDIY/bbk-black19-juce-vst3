# BBK Black-19

Minimal JUCE VST3 for the 192 kHz wide-transition listening experiment.

DSP:
- Host rate: 192000 Hz only
- Type-I linear-phase FIR
- 19 taps
- Passband: 0-20 kHz
- Transition: 20-76 kHz
- Stopband: 76-96 kHz
- Group delay: 9 samples = 46.875 us
- Internal BYPASS is also delayed 9 samples at 192 kHz
- ON/OFF transition crossfades over 10 ms
- Unsupported host rates are hard-bypassed with zero reported latency

The coefficients are embedded in `Source/Black19Kernel.h` and DC-normalised.
The standalone `BBKBlack19_DSPTest` target verifies symmetry, DC gain,
frequency response and peak sidelobe ratio without requiring JUCE.

## Windows build

Prerequisites:
- Visual Studio 2022 with Desktop development with C++
- CMake 3.22+
- Git (only needed if JUCE is not copied into `./JUCE`)

From "Developer Command Prompt for VS 2022":

    cmake -S . -B build -G "Visual Studio 17 2022" -A x64
    cmake --build build --config Release --target BBKBlack19_VST3

Expected bundle:

    build\BBKBlack19_artefacts\Release\VST3\BBK Black-19.vst3

Copy the entire `.vst3` bundle to:

    C:\Program Files\Common Files\VST3\

Then restart/rescan Audirvana.

JUCE is pinned to 8.0.15. If a `JUCE` folder exists beside this CMakeLists,
the project uses that local checkout instead of downloading JUCE.
