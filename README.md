# Uberknall Inspired VST3 (starter rebuild)

A minimal JUCE 8 / CMake VST3 project with two independent bypasses:

- Drive stage: Drive control and Drive Bypass.
- Tone stack: Bass, Mid, Treble, Presence and Tone Stack Bypass.
- Master output level.

The editor uses JUCE's generic parameter editor, so the controls are generated from the plugin parameters.

## Build with GitHub Actions

1. Create a new GitHub repository.
2. Upload the **contents** of this folder to the repository root. Do not upload the enclosing folder as a nested directory.
3. Ensure `.github/workflows/build.yml` is present at the repository root.
4. Commit to the `main` branch.
5. Open the Actions tab and select **Build VST3**.
6. When the run succeeds, download the `Uberknall-Inspired-Windows-VST3` artifact.

The workflow builds the VST3 target on a Windows GitHub runner. JUCE is fetched automatically by CMake from the pinned JUCE 8.0.6 tag.

## Controls

- Drive: increases the input excitation into a smooth saturation curve.
- Drive Bypass: bypasses only the nonlinear drive stage.
- Bass: low-shelf adjustment, centered approximately at 120 Hz.
- Mid: broad peaking adjustment, centered approximately at 420 Hz.
- Treble: high-shelf adjustment, centered approximately at 2.2 kHz.
- Presence: additional high-shelf adjustment, centered approximately at 3.9 kHz.
- Tone Stack Bypass: bypasses the four tone filters together.
- Master: final output level.

## Important modeling note

This is a clean, compilable starting implementation, not a circuit-accurate simulation of the Bogner Uberschall or the PCB Guitar Mania Uberknall clone. The drive stage is a simple smooth saturation model. The tone stack is an initial DSP approximation using separate filters; it does not yet reproduce the passive TMB network's component interactions, loading, or the original Presence feedback topology. Frequencies and ranges are provisional listening targets, not verified measurements. The two bypasses are independent, but currently bypass their processing completely rather than emulating circuit loading while bypassed.

The next engineering step is to derive and validate the exact drive and tone-stack transfer behavior from the selected schematic and compare it against the supplied reference recording and/or hardware.

## JUCE licensing

JUCE is fetched from its official repository. Review JUCE's current licensing terms before distributing binaries or source. This project does not include JUCE source in the repository; CMake downloads it during configuration.
