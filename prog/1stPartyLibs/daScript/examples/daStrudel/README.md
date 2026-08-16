# daStrudel examples

Audio examples and live-coding demos built on the daslang
[strudel module](../../modules/dasAudio/strudel/). All examples compile
under interpreter, AOT, and JIT.

## Layout

| Directory | Purpose |
|---|---|
| [features/](features/) | ~100 short, single-feature snippets — one combinator or effect per file. Built around [`feature_common`](features/feature_common.das); render to WAV with `--wav PATH` for offline inspection. |
| [hrtf/](hrtf/) | 3D positional ("bumblebee") demo using the per-event HRTF override (azimuth + elevation). Three sources rendered simultaneously: one orbiting source plus two stationary positions. |
| [drum_compare/](drum_compare/) | A/B drum kit comparison (built-in synth vs sample bank). |
| [synth_demo/](synth_demo/) | 100% synthesised mini-track (no sample files). |
| [piano_demo/](piano_demo/) | Piano patterns over the SF2 synth. |
| [sf2_demo/](sf2_demo/) | SoundFont rendering with bank/program selection. |
| [midi_demo/](midi_demo/) | MIDI file playback through the strudel scheduler. |
| [midi_sf2_demo/](midi_sf2_demo/) | MIDI files + SF2 rendering. See its own [README](midi_sf2_demo/README.md). |
| [player_demo/](player_demo/) | Foundational `strudel_player` API usage. |
| [strudel_live/](strudel_live/) | Live-coding harness with `daslang-live` hot-reload. |
| [strudel_sf2_live/](strudel_sf2_live/) | Live-coding with SF2 synthesis. |
| [strudel_visualizer/](strudel_visualizer/) | Pattern + scope visualiser. |

## Running

```sh
# Interpreter (fastest iteration)
bin/daslang examples/daStrudel/synth_demo/main.das

# AOT (production performance)
test_aot examples/daStrudel/synth_demo/main.das -- --use-aot

# JIT (hybrid; falls back to interpreter for changed functions)
bin/daslang examples/daStrudel/synth_demo/main.das -jit
```

### Feature snippets

Each `features/*.das` file uses `play_feature_cps` from `feature_common.das`
and supports three flags:

```sh
bin/daslang examples/daStrudel/features/orbit_chorus_only.das               # play 10s
bin/daslang examples/daStrudel/features/orbit_chorus_only.das --duration 4  # play N seconds
bin/daslang examples/daStrudel/features/orbit_chorus_only.das --wav out.wav # render offline
bin/daslang examples/daStrudel/features/orbit_chorus_only.das --track-memory
```

## See also

- Module reference: [doc/source/stdlib/sec_strudel.rst](../../doc/source/stdlib/sec_strudel.rst)
- Tutorials: [tutorials/daStrudel/](../../tutorials/daStrudel/)
- Mini-notation compatibility & daslang extensions (HRTF, etc.):
  [doc/source/reference/strudel_vs_strudel_cc.rst](../../doc/source/reference/strudel_vs_strudel_cc.rst)
- Porting notes: [skills/strudel_port.md](../../skills/strudel_port.md)
