# BBK WavTap

Minimal JUCE VST3 "tap" plugin. It does not process audio in any way - the
output is always bit-identical to the input. Its only function is an
optional, real-time-safe capture of the signal passing through it to a
32-bit float WAV file on disk.

## Why

Built to get a bit-perfect, file-based copy of whatever a streaming/player
app (Tidal via Audirvana, etc.) is sending through a host's plugin chain
(e.g. Blue Cat PatchWork), without going through a DAC/ADC hardware
loopback. Insert WavTap in the chain, hit record, play the track, stop -
the resulting WAV can then be loaded into any DAW (e.g. REAPER) to audition
other plugins against it offline, with zero conversion loss and no
analog-loopback questions (clock sync, channel count, noise floor, etc.).

Architecture is deliberately as simple/stateless as possible in the audio
path (no delay line, no per-sample smoothing, no persistent DSP state) -
matching the pattern of BBK ILD Matrix and BBK E280F Triode, both confirmed
glitch-free when hosted inside PatchWork/Audirvana.

## Use

1. Insert BBK WavTap in the plugin chain (e.g. in PatchWork).
2. Open its editor, confirm the host sample rate shown matches what you
   expect (e.g. 192000 Hz).
3. Click "START RECORDING", play the track, click "STOP RECORDING".
4. The WAV file is written to:

       Documents\BBK WavTap Captures\WavTap_<timestamp>.wav

5. Load that file into REAPER (or any DAW) and insert whatever plugin you
   want to audition (e.g. BBK Black-19) on that track to listen with FX
   applied - bit-perfect source, zero DAC/ADC round trip.

## Windows build

Prerequisites:
- Visual Studio 2022 with Desktop development with C++
- CMake 3.22+
- Git (only needed if JUCE is not copied into `./JUCE`)

From "Developer Command Prompt for VS 2022":

    cmake -S . -B build -G "Visual Studio 17 2022" -A x64
    cmake --build build --config Release --target BBKWavTap_VST3

Expected bundle:

    build\BBKWavTap_artefacts\Release\VST3\BBK WavTap.vst3

Copy the entire `.vst3` bundle to:

    C:\Program Files\Common Files\VST3\

Then restart/rescan your host.

JUCE is pinned to 8.0.15. If a `JUCE` folder exists beside this CMakeLists,
the project uses that local checkout instead of downloading JUCE.
