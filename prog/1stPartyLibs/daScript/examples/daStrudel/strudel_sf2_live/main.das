options gen2
options indenting = 4

// SF2 SoundFont live demo — lo-fi chill beat with hot-reload support.
// SF2 data persists across reloads (no re-reading from disk).
//
// Run: daslang-live.exe examples/daStrudel/strudel_sf2_live/main.das
// Or:  daslang.exe examples/daStrudel/strudel_sf2_live/main.das

require strudel/strudel
require strudel/strudel_player
require strudel/strudel_live
require audio/audio_boost
require live/audio_live
require live/live_commands
require live/live_api
require live/live_watch_boost
require live_host
require daslib/fio
require daslib/json
require daslib/json_boost

def find_sf2() : string {
    let dir = "{get_das_root()}/examples/daStrudel/midi_sf2_demo"
    var st : FStat
    let musyng = "{dir}/Musyng Kite.sf2"
    if (stat(musyng, st)) {
        return musyng
    }
    let fluid = "{dir}/FluidR3_GM.sf2"
    if (stat(fluid, st)) {
        return fluid
    }
    let generaluser = "{dir}/GeneralUser GS v1.471.sf2"
    if (stat(generaluser, st)) {
        return generaluser
    }
    return ""
}

def strudel_main() {
    strudel_set_bpm(120.0lf)
    strudel_add_track(stack([
        // drums — SF2 GM percussion (bank 128): c2=kick, d2=snare, eb2=clap, fs2=closed hh
        note_pattern("c2 ~ [~ c2] ~, ~ eb2 ~ eb2, [~ fs2] [~ fs2] [~ fs2] [~ fs2]")
            |> set_sf2(0) |> set_sf2_bank(128) |> set_gain(0.7),
        // piano arpeggios — Am-F-C-G
        note_pattern("<[a3 c4 e4 a4 e4 c4 a3 c4] [f3 a3 c4 f4 c4 a3 f3 a3] [c3 e3 g3 c4 g3 e3 c3 e3] [g3 b3 d4 g4 d4 b3 g3 b3]>")
            |> set_sf2("piano") |> set_gain(0.45),
        // bass
        note_pattern("<[a2 ~ ~ a2 ~ a2 ~ ~] [f2 ~ ~ f2 ~ f2 ~ ~] [c2 ~ ~ c2 ~ c2 ~ ~] [g2 ~ ~ g2 ~ g2 ~ ~]>")
            |> set_sf2("acoustic_bass") |> set_gain(0.6),
        // string pad
        note_pattern("<[a2,c3,e3] [f2,a2,c3] [c2,e2,g2] [g2,b2,d3]>")
            |> set_sf2("string_ensemble") |> set_gain(0.2)
    ]))
    strudel_play()
}

// --- Live commands ---

struct CmdSetBpmArgs {
    bpm : double = 120.0lf
}

[live_command(description="Set tempo in BPM")]
def cmd_set_bpm(input : JsonValue?) : JsonValue? {
    let args = from_JV(input, type<CmdSetBpmArgs>)
    strudel_set_bpm(args.bpm)
    return JV("bpm set to {args.bpm}")
}

struct CmdSetVolumeArgs {
    volume : float = 1.0
    time : float = 0.0
}

[live_command(description="Set volume with optional fade time")]
def cmd_set_volume(input : JsonValue?) : JsonValue? {
    let args = from_JV(input, type<CmdSetVolumeArgs>)
    strudel_set_volume(args.volume, args.time)
    return JV("volume set to {args.volume} (fade {args.time}s)")
}

[live_command(description="Pause playback")]
def cmd_pause(input : JsonValue?) : JsonValue? {
    strudel_set_pause(true)
    return JV("paused")
}

[live_command(description="Unpause playback")]
def cmd_unpause(input : JsonValue?) : JsonValue? {
    strudel_set_pause(false)
    return JV("unpaused")
}

// --- daslang-live mode ---

[export]
def init() {
    if (get_audio_command_channel() == null) {
        audio_system_create()
    }
    if (!strudel_has_sf2()) {
        let sf2_path = find_sf2()
        if (!empty(sf2_path) && strudel_load_sf2(sf2_path)) {
            print("Loaded SF2: {sf2_path}\n")
        } else {
            print("No SF2 found — SF2 events will be silent\n")
        }
    } else {
        print("SF2 restored from live store\n")
    }
    strudel_init(@@strudel_main)
    print("strudel SF2 live — playing\n")
}

[export]
def update() {
    pass
}

[export]
def shutdown() {
    pass
}

// --- standalone mode (daslang.exe) ---

[export]
def main() {
    let sf2_path = find_sf2()
    with_audio_system() {
        if (!empty(sf2_path) && strudel_load_sf2(sf2_path)) {
            print("Loaded SF2: {sf2_path}\n")
        }
        print("strudel SF2 live (standalone) — playing 30 seconds\n")
        strudel_init(@@strudel_main)
        sleep(30000u)
        strudel_shutdown()
    }
    print("done\n")
}
