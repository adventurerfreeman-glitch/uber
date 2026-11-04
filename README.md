# Uberknall Inspired VST3 — DSP Rebuild v3

A JUCE 8 / CMake VST3 starting point inspired by the Bogner Uberschall pedal and the Uberknall clone reference. This remains an experimental DSP model, not a validated component-by-component circuit simulation.

## Included controls

- Drive
- Drive Bypass
- Bass
- Mid
- Treble
- Presence
- Tone Stack Bypass
- Master

The drive and tone-stack bypasses are independent. The editor is JUCE's generic parameter editor for this first functional build.

## What changed in v3

- Corrected a stereo-processing bug: each channel now has independent biquad filter state. The previous version shared filter memory between left and right channels, which could cause channel interaction and strange artifacts.
- Added short parameter ramps for Drive, EQ controls, bypasses, and Master to reduce abrupt changes and zipper/click artifacts.
- Reworked the processing loop so automation smoothing advances once per sample and is shared consistently across stereo channels.
- Added finite-value guards to the filter coefficients and output.
- Reduced the default drive-stage excitation compared with the previous build to avoid unnecessarily hard saturation at the default setting.

## Important modeling note

Bass, Mid, Treble, and Presence are currently implemented as stable cascaded biquad filters (low shelf, broad mid bell, high shelf, and presence shelf). They are useful musical DSP controls, but they are **not yet an exact passive Uberknall/Bogner tone-stack simulation**. The drive stage is a simple soft-clipping approximation. The supplied recording is a listening target; matching it requires controlled A/B tests at matched input/output levels and, ideally, the actual schematic topology and component values.

## Build on GitHub Actions

1. Create a new GitHub repository.
2. Upload the *contents* of this folder to the repository root, including the hidden `.github` folder.
3. Commit to the `main` branch.
4. Open **Actions** and run **Build VST3** (or let the push trigger it).
5. Download the `Uberknall-Inspired-Windows-VST3` artifact from the completed workflow run.

The workflow builds the VST3 target on `windows-latest` using CMake and JUCE 8.0.6.

## Current status

Source update prepared; not yet compiled or tested in a DAW in this environment. A successful GitHub Actions build is the next verification step. After installation, test at unity Master with the Drive and Tone Stack bypasses individually, then compare against the reference recording using the same DI/input level and loudness matching.


## v4 gain correction

The front-panel control is named **Gain**. It applies up to +24 dB of pre-drive gain before the soft-clipping curve. There is no automatic normalization, so turning Gain up increases both level and saturation. Use Master to set the final output level.
