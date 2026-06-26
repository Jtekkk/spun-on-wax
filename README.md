# Spun on Wax 🎚️

A vinyl / lo-fi colouring effect plugin built with [JUCE](https://juce.com).
Run any signal through it to make it sound like it was *spun on wax* — warbling
turntable pitch, worn-needle grit, and the crackle and hiss of a well-loved
record.

Builds as **VST3**, **AU**, and a **Standalone** app on macOS, Windows, and Linux.

## The sound

The wet signal flows through:

```
input → wow & flutter → saturation → tone (age) low-pass → + surface noise
      → stereo width → wet/dry blend → output trim
```

| Control   | What it does |
|-----------|--------------|
| **Wow**     | Slow (~0.6 Hz) pitch drift — the long, lazy sag of a warped record. |
| **Flutter** | Fast (~7 Hz) pitch warble — the nervous shimmer of a worn capstan. |
| **Crackle** | Density of sparse pops and ticks. |
| **Hiss**    | Level of the continuous surface-noise bed. |
| **Drive**   | Soft asymmetric saturation for needle grit. |
| **Tone**    | Low-pass "age" — open and bright at full, dull and worn toward zero. |
| **Width**   | Mid/side stereo width of the wet signal (0 = mono, 2 = wide). |
| **Mix**     | Dry/wet blend. |
| **Output**  | Final output trim (−24 .. +12 dB). |

## Building

You need [CMake](https://cmake.org) ≥ 3.22 and a C++17 compiler. JUCE is pulled
in automatically via CMake's `FetchContent`, so no manual setup is required:

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

The built plugins land under `build/SpunOnWax_artefacts/`.

### Windows installer (Setup.exe)

A one-click installer is produced by CI on every push: open the latest
[**build run**](https://github.com/Jtekkk/spun-on-wax/actions/workflows/build.yml),
pick the Windows run, and download the **SpunOnWax-Windows-Installer** artifact
(`SpunOnWax-1.0.0-Setup.exe`). It installs the VST3 into the system VST3 folder
and, optionally, the standalone app.

To build the installer locally on Windows (after a Release build), with
[Inno Setup](https://jrsoftware.org/isinfo.php) installed:

```bat
iscc packaging\windows\SpunOnWax.iss
```

The `.exe` is written to `build\installer\`.

### Using a local JUCE checkout

To avoid the download (or to pin your own JUCE), point CMake at an existing
checkout:

```bash
cmake -B build -DSPUNONWAX_JUCE_PATH=/path/to/JUCE
```

### Linux dependencies

On Linux, JUCE needs the usual audio/GUI development headers, e.g. on Debian/Ubuntu:

```bash
sudo apt install libasound2-dev libjack-jackd2-dev libgtk-3-dev \
                 libfreetype6-dev libfontconfig1-dev libx11-dev \
                 libxext-dev libxinerama-dev libxrandr-dev libxcursor-dev \
                 libcurl4-openssl-dev
```

## Project layout

```
CMakeLists.txt            JUCE plugin target (VST3 / AU / Standalone)
Source/
  PluginProcessor.{h,cpp} Parameters, signal flow, state
  PluginEditor.{h,cpp}    Dark vinyl-themed UI
  dsp/
    WowFlutter.h          Modulated fractional-delay pitch instability
    VinylNoise.h          Hiss bed + crackle/pop generator
    Saturation.h          Soft asymmetric waveshaper
```

## License

This project is released under the MIT License — see [LICENSE](LICENSE).

JUCE is fetched at build time and carries its own license terms.
