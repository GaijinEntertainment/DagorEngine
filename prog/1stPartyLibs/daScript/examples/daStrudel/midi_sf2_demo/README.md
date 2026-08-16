# MIDI SF2 Demo

Plays a MIDI file using a SoundFont (SF2) for instrument sounds.

## Setup

Download a GM SoundFont and place it in this folder. The demo tries them in
order (best first): Stolen -> Musyng Kite -> FluidR3 -> GeneralUser GS.

- **Stolen Soundfont v2.08.2 (rev.3)** (~1.0 GB `.sf2`, distributed as a
  ~720 MB `.7z`) - the highest-quality GM bank we tried.
  - Author thread: https://www.doomworld.com/forum/topic/135393-stolen-soundfont-v2082-rev3-april-30th-2025-a-high-quality-replacement-for-the-windows-midi-synth/
  - Download (Google Drive folder): https://drive.google.com/drive/folders/1gYcGnncispXLUGr0HM_-0yNn5UcyC4Sj
  - It is a `.7z` archive - extract it so this folder ends up with the file
    `Stolen Soundfont v2.08.2 (rev.3).sf2` (the exact name `find_sf2` looks for).
    Scripted: `gdown 122fbYjBKWw2SxZZF0GwfeTI-BISdXUSC` then
    `7z x "Stolen Soundfont v2.08.2 (rev.3).7z"` (or
    `python -m py7zr x "Stolen Soundfont v2.08.2 (rev.3).7z"`).
  - **Legal status: unknown.** The author themselves notes the copyright
    situation is murky; we make no claim that distributing or using it is
    legal. Download at your own discretion - local use only.

- **Musyng Kite.sf2** (~1 GB) - very high quality, 5737 multi-velocity samples.
  - https://archive.org/details/musyng-kite
    (direct: https://archive.org/download/musyng-kite/Musyng%20Kite.sf2)
  - Gray-area compilation bank; redistribution status unclear.

- **FluidR3_GM.sf2** (~141 MB) - full GM bank, good quality, freely
  redistributable (Frank Wen, MIT-style license).
  - https://sourceforge.net/projects/androidframe/files/soundfonts/FluidR3_GM.sf2/download
  - https://musical-artifacts.com/artifacts/738

- **GeneralUser GS v1.471.sf2** (~30 MB) - lighter alternative, rich
  modulators, freely redistributable (S. Christian Collins).
  - https://raw.githubusercontent.com/ROCKNIX/generaluser-gs/main/GeneralUser%20GS%20v1.471.sf2

If no SoundFont is found, the demo falls back to basic oscillators.

## Usage

```
daslang.exe examples/daStrudel/midi_sf2_demo/main.das
daslang.exe examples/daStrudel/midi_sf2_demo/main.das -- path/to/song.mid
```
