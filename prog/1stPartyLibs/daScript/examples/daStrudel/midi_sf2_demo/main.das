options gen2
options indenting = 4

// MIDI player with SF2 SoundFont — plays a .mid file using a GM SoundFont.
// Place a GM SoundFont next to this file (see README.md for download links).
// Usage: daslang.exe main.das [-- path_to.mid]
// Default: examples/media/midi/take_five.mid

require strudel/strudel_midi_player
require audio/audio_boost
require daslib/fio
require daslib/strings_boost

def find_sf2() : string {
    let dir = "{get_das_root()}/examples/daStrudel/midi_sf2_demo"
    var st : FStat
    // prefer larger soundfonts — more samples, better quality
    let stolen = "{dir}/Stolen Soundfont v2.08.2 (rev.3).sf2"
    if (stat(stolen, st)) {
        return stolen
    }
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

[export]
def main() {
    let media = "{get_das_root()}/examples/media"
    var path = "{media}/midi/Bach_Air_on_G_string_BWV1068.mid"
    let args <- get_command_line_arguments()
    for (arg in args) {
        if (ends_with(arg, ".mid") || ends_with(arg, ".MID")) {
            path = arg
        }
    }
    let sf2 = find_sf2()
    print("MIDI SF2 player — {path}\n")
    with_audio_system() {
        if (empty(sf2)) {
            print("No SF2 found — falling back to oscillators (see README.md)\n")
        } elif (!midi_load_sf2(sf2)) {
            print("Failed to load {sf2} — falling back to oscillators\n")
        } else {
            print("Loaded {sf2}\n")
        }
        midi_init()
        if (midi_play("music", path, [gain = 1.0, looping = true])) {
            print("playing 60 seconds (looping)...\n")
            sleep(60000u)
        } else {
            print("failed to load MIDI file\n")
        }
        midi_shutdown()
        print("done\n")
    }
}
