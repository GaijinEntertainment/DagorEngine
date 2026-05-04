# MIDI SF2 Demo

Plays a MIDI file using a SoundFont (SF2) for instrument sounds.

## Setup

Download a GM SoundFont and place it in this folder. The demo tries them in order (best first):

- **Musyng Kite.sf2** (~1 GB) - highest quality, 5737 samples with multi-velocity layers
  - https://musical-artifacts.com/artifacts/433
  - License: CC BY 3.0 (S. Christian Collins)

- **FluidR3_GM.sf2** (~141 MB) - full GM bank, good quality
  - https://sourceforge.net/projects/androidframe/files/soundfonts/FluidR3_GM.sf2/download
  - https://musical-artifacts.com/artifacts/738

- **GeneralUser GS v1.471.sf2** (~30 MB) - lighter alternative, rich modulators
  - https://raw.githubusercontent.com/ROCKNIX/generaluser-gs/main/GeneralUser%20GS%20v1.471.sf2

If no SoundFont is found, the demo falls back to basic oscillators.

## Usage

```
daslang.exe examples/daStrudel/midi_sf2_demo/main.das
daslang.exe examples/daStrudel/midi_sf2_demo/main.das -- path/to/song.mid
```
