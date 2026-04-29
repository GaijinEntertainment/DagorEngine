options gen2
options indenting = 4

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
require daslib/strings_boost
require daslib/json
require daslib/json_boost

let MEDIA = "{get_das_root()}/examples/media"

// Track name -> index mapping (maintained by user code)
var g_track_names : table<string; int>

def strudel_main() {
    let drums = strudel_add_track(s("bd [hh hh] sd [hh cp]"))
    g_track_names["drums"] = drums
    strudel_set_bpm(120.0lf)
    strudel_play(@@strudel_on_command)
}

def strudel_on_command(cmd : string) {
    // commands: "play:name:pattern", "stop:name", "stop:name:fade_time", "fade:name:gain:time"
    let parts <- split(cmd, ":")
    if (length(parts) < 2) {
        return
    }
    let action = parts[0]
    let name = parts[1]
    if (action == "play" && length(parts) >= 3) {
        // remove old track with this name if it exists
        if (key_exists(g_track_names, name)) {
            strudel_remove_track(g_track_names[name])
        }
        let idx = strudel_add_track(s(parts[2]))
        g_track_names[name] = idx
    } elif (action == "stop") {
        if (key_exists(g_track_names, name)) {
            let fade_time = length(parts) >= 3 ? float(to_double(parts[2])) : 0.5
            strudel_fade_track(g_track_names[name], 0.0, fade_time)
        }
    } elif (action == "fade" && length(parts) >= 4) {
        if (key_exists(g_track_names, name)) {
            let gain = float(to_double(parts[2]))
            let time = float(to_double(parts[3]))
            strudel_fade_track(g_track_names[name], gain, time)
        }
    }
}

def load_samples() {
    strudel_load_sound("{MEDIA}/drums/bd", "bd")
    strudel_load_sound("{MEDIA}/drums/sd", "sd")
    strudel_load_sound("{MEDIA}/drums/hh", "hh")
    strudel_load_sound("{MEDIA}/drums/cp", "cp")
    strudel_load_sound("{MEDIA}/drums/ride", "ride")
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

struct CmdSetCpsArgs {
    cps : double = 0.5lf
}

[live_command(description="Set tempo in cycles per second")]
def cmd_set_cps(input : JsonValue?) : JsonValue? {
    let args = from_JV(input, type<CmdSetCpsArgs>)
    strudel_set_cps(args.cps)
    return JV("cps set to {args.cps}")
}

struct CmdSetVolumeArgs {
    volume : float = 1.0
    time : float = 0.0
}

[live_command(description="Set volume (0.0-1.0) with optional fade time in seconds")]
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

struct CmdStrudelArgs {
    cmd : string
}

[live_command(description="Send command to strudel thread (play:name:pattern, stop:name:fade, fade:name:gain:time)")]
def cmd_strudel(input : JsonValue?) : JsonValue? {
    let args = from_JV(input, type<CmdStrudelArgs>)
    strudel_command(args.cmd)
    return JV("sent: {args.cmd}")
}

// daslang-live mode
[export]
def init() {
    print("daStrudel live\n")
    if (get_audio_command_channel() == null) {
        audio_system_create()
    }
    load_samples()
    strudel_init(@@strudel_main)
    print("playing...\n")
}

[export]
def update() {
    pass
}

[export]
def shutdown() {
    pass
}

// standalone mode (daslang.exe)
[export]
def main() {
    print("daStrudel live (standalone)\n")
    with_audio_system() {
        load_samples()
        strudel_init(@@strudel_main)
        print("playing... press Ctrl+C to stop\n")
        while (true) {
            sleep(100u)
        }
    }
}
