# soundSystem/voice

The voice chat audio layer: mic capture, voice processing, the echo reference and the playback of voice
sources through FMOD. The public API is `publicInclude/soundSystem/voiceCommunicator.h`. Proximity voice
(`prog/gameLibs/proximityVoiceChat`) and tamVoice use it through `VoiceCommunicator`, `IVoiceSink` and
`IVoiceSource`.

## Tech stack

| Library | Location | Use |
| --- | --- | --- |
| FMOD | through `gameLibs/soundSystem` | mic record, the playback DSPs, the echo reference DSP on the master channel group |
| WebRTC APM | `3rdPartyLibs/webrtc-apm` (mid-2025 snapshot, local changes in `gaijin.patch`) | AEC3 echo cancellation, AGC2 gain, the noise suppressor of a build without RNNoise |
| RNNoise | `3rdPartyLibs/codecs/rnnoise` | denoise and the speech probability for the speech gate, on every platform (`USE_RNNOISE_DENOISER` in the jamfile) |
| SpeexDSP 1.2.0 | `3rdPartyLibs/codecs/speexdsp-1.2.0` | the resamplers: capture to output rate, source to mixer rate, mixer to capture rate |
| Opus 1.1 | `3rdPartyLibs/codecs/opus-1.1`, wrapped by `gameLibs/voiceChatCodecs/opus` | proximity voice encode and decode, concealment, inband FEC |
| Dear ImGui | `3rdPartyLibs/imgui`, `engine/imgui` | the metrics overlay, dev and dbg builds only |

The proximity voice frames go over daNet (enet) as `VoiceDataToClientMsgV3`, declared in
`proximityVoiceChatNet/voiceNetMsg.cpp`.

## How it works

### Threads

| Thread | Work |
| --- | --- |
| `VoiceCaptureThread` (`SharedVoiceCapture`) | reads the mic, runs `VoiceProcessor`, gives the result to each client |
| owner thread of a communicator (the voice worker) | `VoiceCommunicatorImpl::process`: frames for the sinks, frames from the sources into the playback rings |
| FMOD mixer thread | `VoiceSourceControl::readMix` plays the rings, `VoiceFeedback::processEchoSource` takes the echo reference |

The mixer thread shares data only through lock-free rings (`SpscRing`). The capture thread gives the processed
mic and the meter level to each client through queues that a `WinCritSec` guards, so a slow `process()` and a
capture step can wait for each other there. The tuning values are guarded too.

### Capture

There is one mic capture for the process. All communicators share it:

1. `VoiceRecorder` records the mic into a 500 ms ring at the capture rate.
2. `SharedVoiceCapture` reads the ring in 10 ms frames and runs `VoiceProcessor::process` on them.
3. Each communicator has a `VoiceCaptureClient` at the sample rate of its sink. The client keeps up to 500 ms of
   processed audio at that rate. A client that reads later than that loses the older audio.
4. `VoiceCommunicatorImpl::process` gives the audio to a `VoiceFramer` for each sink. The framer cuts it into
   the frame size of that sink. Raw sinks get the audio from before the processing.

Communicators can use different sample rates. The processor resamples once to each rate that a client uses, and
a rate that stays in use keeps its resampler state when clients join or leave.

### Processing chain

`VoiceProcessor::processFrames` does steps 1 to 5 on each 10 ms frame, and `VoiceProcessor::process` does step 6
on the whole processed span after that loop:

1. AEC3 echo cancellation (`echoAP`). Before the mic frame, the processor gives the ready frames of the echo
   reference to `ProcessReverseStream`.
2. Record volume.
3. RNNoise denoise, which also gives a speech probability. The jamfile sets `USE_RNNOISE_DENOISER` on every
   platform; a build without it uses the WebRTC noise suppressor and has no speech gate.
4. AGC2 adaptive digital gain (`gainAP`).
5. Speech detector and gate (`SpeechDetector`, `SpeechGate`). The closed gate opens after
   `vadOnsetFrames` frames in a row with a probability at or above `vadProbThreshold`. While it is open,
   the detector smooths the probability with `vadSmoothing` and keeps the gate open while the result is
   at or above `vadCloseThreshold`. The gate keeps a preroll of frames from before the speech start and
   sends them when it opens. It stays open for the hangover after the last speech frame. It fades in the first frame and fades out the
   last frame of each spurt.
6. Resample to each output rate.

`voice.preprocess_enabled 0` turns off steps 1 and 4. `voice.denoise_enabled 0` turns off steps 3 and 5.

### Echo reference

`VoiceFeedback` is a DSP at the tail of the master channel group. On the mixer thread it only downmixes what the
game plays into a ring, at the mixer rate. The capture thread drains the ring, resamples it to the capture rate
and takes it in 10 ms frames for AEC3. Reference that is older than 500 ms cannot line up with the mic, so it is dropped. When the capture
stops, `[VC] echo reference lost` tells how much reference was not used.

### Playback

Each `IVoiceSource` has a `VoiceSourceControl`:

- The voice worker takes the frames of the source, resamples them to the mixer rate, and puts them into a
  ring.
- An FMOD generator DSP plays the ring on the mixer thread. A new spurt waits for the start margin
  (`playbackStartMarginMs`) before it plays, so that a late frame does not make a gap. The worker keeps the
  ring `playbackLeadFrames` mixer blocks ahead.
- When the ring runs dry, the DSP fades the voice out and plays silence. A dry ring inside a spurt is an
  underrun. The control gives the underruns back to the source.
- A source that makes audio on demand (`makesAudioOnDemand`) has no start margin.

### Voice test (loopback)

`SoundSettings::voiceTest` or `voice.test 1` plays the processed mic back through a `VoiceLoopback` source.
The loopback keeps up to 500 ms. The console switch works with the first communicator that sees it on.
Use it to hear the result of the chain and of the tuning on a real mic.

### Proximity voice path

The send side is `ProximityVoiceChatSink`: it encodes 20 ms Opus frames at the 24 kHz network rate
(`PROXIMITY_VOICE_SAMPLE_RATE`), which lets Opus code super-wideband (up to 12 kHz). A peer of an older build at 16 kHz
decodes these packets, and its own, at its rate. The sink numbers each frame that it sends,
and flags the last frame of a spurt. It applies the Opus tuning (bitrate, complexity, inband FEC, the
planned loss) at the next `process()` after a tuning change. Each listener reports its measured loss of a
speaker every 2 s through the server, and the sink plans the worst report of the last 8 to 10 s, up to 50 percent;
the tuning value is the fallback while no report comes. With the inband FEC on, from `opusWidebandLossPercent` of
planned loss on, the sink caps Opus at wideband, so the FEC fits into the bitrate. Until the first report the fallback
planned loss caps it too, so a speaker sends super-wideband only once its listeners report a clean link.

The receive side is `VoiceChatSoundStream`, with a `VoicePlayoutBuffer` for each sender:

- `VoicePlayoutScheduler` knows no codec. It puts the frames in sample order, anchors each burst on its
  first frame, keeps a jitter estimate, and at the deadline of each slot gives the frame or the lost slot
  to the decoder stage. A frame that comes after its slot is late and is dropped.
- `OpusStage` decodes. For a lost slot it uses the FEC of the next frame when that frame is pending and
  carries FEC (LBRR), else Opus concealment. After 3 concealed slots in a row it plays silence, because
  Opus concealment is near silence after that. A slot recovered from FEC always plays.

The decoded audio goes through the room queue to `ProximityVoiceChatSource`, which is the `IVoiceSource` that
`VoiceSourceControl` plays. `proximityVoiceChat/readme.txt` shows who owns what.

## How to debug

### Console

| Command | Effect |
| --- | --- |
| `voice.stats_enabled 1` | turns on the periodic stats logs (see below) |
| `voice.toggle_metrics` | shows or hides the imgui metrics overlay; dev and dbg builds only |
| `voice.test 1` | plays the processed mic back |
| `voice.denoise_enabled 0` | turns off RNNoise and the speech gate |
| `voice.preprocess_enabled 0` | turns off AEC3 and AGC2 |
| `voice.reload_tuning [file.blk]` | loads the tuning again (see "How to tune") |
| `voice.dump_capture [seconds]` | dev and dbg builds: writes the next 1..60 s (default 10) of the raw mic, the echo reference as fed, a line `<frames fed> <record volume> <preroll dropped>` per capture frame and the output as `voice_dump_<n>_*` into the log directory (the current directory where the platform has none, as on consoles); a dump ends early where the record stops, so it never spans two runs of the chain, or where its output rate leaves the processor's set |

To find which step makes a fault, turn the steps off one at a time with `voice.denoise_enabled` and
`voice.preprocess_enabled`, and listen with `voice.test 1`.

### Logs

The lines below start with `[VC]`. With `voice.stats_enabled 1`, these lines come every 5 minutes, and also when
the object goes away:

- `capture stats of record device ...`: frames processed, record ring lags, audio dropped for late readers,
  frames dropped by the speech gate, frames without echo reference, APM errors, the AGC2 gain range, and the time of
  the chain per 10 ms frame (mean and max) with the mean of its RNNoise stage, which tells the CPU cost on a device.
- `sink stats ...`: frames sent to each sink.
- `receive stats of player ...`: frames received, late, and missing from the numbers; slots concealed,
  recovered from FEC, and lost and silent; bursts; the playout buffer and its target; blocks dropped by a
  full room queue; playback underruns.
- `worker stats ...`: iterations and the time of one pass of the proximity voice worker
  (`ProximityVoiceChatManager`), max and p99.

These lines come always:

- `tuning: ...`: the tuning values after each load.
- `record reader lags by ...`: the capture thread read the record ring late.
- `voice capture client read more than 500 ms late`: a communicator did not call `process` in time.
- `echo reference lost: ...`: reference audio that AEC3 did not get.
- `use RNNoise denoiser` or `use WebRTC denoiser`: the chain of this platform.

### Symptoms

| Symptom | Look at |
| --- | --- |
| echo of the game sound or of other players | "without echo reference" in the capture stats, `echo reference lost` |
| first syllable cut | gate drops in the capture stats; `vadPrerollFrames`, `vadOnsetFrames`, `vadProbThreshold` |
| noise is sent | the speech gate state in the overlay; `vadProbThreshold`, `vadOnsetFrames`, `vadCloseThreshold` |
| voice too quiet or noise pumps | the AGC2 gain range in the capture stats; `agc2MaxGainDb` |
| gaps on the receive side | late, missing and concealed counts in the receive stats; the playout keys |
| clicks or gaps with a full buffer | playback underruns, ring fill in the overlay; the playback keys |
| voice too loud in one ear, or jumps between the ears | `playbackPanLevel`, `playbackPanFadeMeters` |
| audio of a client stops | `voice capture client read more than 500 ms late`, the worker stats |

### Harnesses

- `unittests/voiceProcessing`: the regression set of the processing chain (CI lane
  `voice_processing_unit_tests`). The WAV mode runs the real chain on a file:
  `voiceProcessing-dev [-tuning:file.blk] [-feed:file.txt] [-denoise:0] [-preprocess:0] mic.wav out.wav [far.wav]`,
  mono PCM16 at the capture rate.
  `far.wav` is the echo reference. `-feed:voice_dump_<n>_feed.txt` replays the far end of a `voice.dump_capture`
  as it was fed. The replay starts AEC3 and AGC2 cold, while the dump started in a running session, so the first
  second or two of the output can differ from `voice_dump_<n>_out.wav`. The dump holds neither the
  tuning nor the two step switches: replay a session that ran with its own `voice{tuning{}}` with `-tuning:` and the
  same block, and one that ran with a step off with `-denoise:0` or `-preprocess:0`.
- `unittests/voicePlayback`: plays a positional source through `VoiceSourceControl` into an offline FMOD mixer
  and checks the level of each ear around the listener (CI lane `voice_playback_unit_tests`).
- `unittests/voiceNetwork`: the proximity send and receive path through a simulated network, scored with
  STOI (CI lane `voice_network_unit_tests`). `abCompare.py` compares revisions per scenario. See
  `unittests/voiceNetwork/README.md`.

## How to tune

### Where the values are

The values are in the `voice{tuning{}}` block of the game settings:

```
voice {
  tuning {
    vadProbThreshold:r=0.6
    vadHangoverFrames:i=50
    opusBitrateBps:i=24000
    playoutMinJitterMs:i=20
  }
}
```

A key that is not in the block uses its default. A value out of range is clamped. The first use loads the
block. `voice.reload_tuning` loads it again from the settings, and `voice.reload_tuning file.blk` loads
the `voice{tuning{}}` block of that file. Each load writes the `[VC] tuning:` line, so you can see the
values that are in use. A reload does not need a restart.

### Keys

Capture keys apply at the next capture frame. The `vad` keys act only with the RNNoise speech gate, which a build without `USE_RNNOISE_DENOISER` lacks.

| Key | Default | Range | Effect |
| --- | --- | --- | --- |
| `vadProbThreshold` | 0.6 | 0..1 | RNNoise speech probability that opens the gate; higher sends less noise and cuts more soft speech |
| `vadCloseThreshold` | `vadProbThreshold` | 0..`vadProbThreshold` | smoothed probability that keeps the open gate open; lower holds soft speech, and on noise keeps the gate open longer |
| `vadOnsetFrames` | 2 | 1..`vadPrerollFrames` + 1 | frames in a row at the open threshold that open the gate; more rejects short noise, and the preroll sends the onset frames |
| `vadSmoothing` | 0.5 | 0..0.9 | weight of the earlier frames in the smoothed probability while the gate is open; more holds through short dips |
| `vadPrerollFrames` | 5 | 0..50 | 10 ms frames sent from before the speech start; more keeps word starts, adds delay at the start |
| `vadHangoverFrames` | 50 | 1..500 | 10 ms frames the gate stays open after speech; more keeps word ends, sends more noise |
| `agc2InitialGainDb` | 6 | 0..50 | AGC2 gain at the start; a load that changes an AGC2 key restarts AGC2 at this gain |
| `agc2MaxGainDb` | 30 | initial gain (min 1)..50 | AGC2 gain limit; less boosts less noise on a quiet mic |

Opus keys apply at the next `process()` of the sink.

| Key | Default | Range | Effect |
| --- | --- | --- | --- |
| `opusBitrateBps` | 24000 | 6000..64000 | encoder bitrate |
| `opusComplexity` | 8 on PC, 4 on other platforms | 0..10 | quality against encoder CPU time |
| `opusInbandFec` | true | bool | adds FEC of the previous frame to each frame, so one lost frame can be recovered |
| `opusPacketLossPercent` | 10 | 0..50 | planned loss while no listener reports one; more makes the FEC stronger and takes bitrate from the main signal. Proximity voice plans the worst loss that its listeners reported in the last 8 to 10 s (each listener reports every 2 s), up to 50. Opus writes the inband FEC only with a planned loss above 0, so a clean link that reports 0 sends no FEC |
| `opusWidebandLossPercent` | 2 | 0..51 | with `opusInbandFec` on, from this planned loss on the encoder stays wideband (up to 8 kHz), since at 24 kbps a super-wideband frame leaves Opus no bits for the inband FEC; 51 never caps |

Playout keys apply at the start of the next burst of a sender.

| Key | Default | Range | Effect |
| --- | --- | --- | --- |
| `playoutMinJitterMs` | 20 | 0..500 | lowest playout buffer |
| `playoutMaxJitterMs` | 200 | min..1000 | highest playout buffer, and the limit on the delay a late frame can add |
| `playoutJitterMarginMs` | 10 | 0..200 | added to the jitter estimate |
| `playoutInitialJitterMs` | 30 | 0..1000 | jitter estimate before the first burst; a change resets the estimate |

Playback keys: the lead applies at the next push, the margin at the start of the next spurt, the pan keys at
the next 3D update.

| Key | Default | Range | Effect |
| --- | --- | --- | --- |
| `playbackStartMarginMs` | 96 | 0..200 | wait at the start of a spurt before it plays; more prevents underruns, adds delay |
| `playbackLeadFrames` | 2 | 2..6 | mixer blocks the worker keeps ahead in the ring |
| `playbackPanLevel` | 0.7 | 0..1 | FMOD 3D level of positional voice, the rest centered; 1 takes the whole voice to one ear at the side |
| `playbackPanFadeMeters` | 1.0 | 0..10 | closer than this, the 3D level falls linearly to centered, so a speaker next to the listener does not flip between the ears |

### Procedure

1. Write the values in a blk file with a `voice{tuning{}}` block.
2. For the capture keys, run the WAV mode of `unittests/voiceProcessing` with `-tuning:file.blk` on a
   recorded mic file and listen to the output. Then do `voice.reload_tuning file.blk` in the game and
   check with `voice.test 1`.
3. For the Opus and playout keys, put the values in the defaults of a worktree and run `abCompare.py`
   against master. In the game, compare the receive stats before and after the reload.
4. Change one key at a time. The latency and the gaps go in opposite directions for most keys.
5. When a value is good for all players, change the default in `voiceTuning.h` or `voiceCommunicator.h`,
   and update the tables here.
