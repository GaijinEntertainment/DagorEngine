options gen2
options indenting = 4

// MIDI file player demo — plays a .mid file with real piano + drum samples.
// Usage: daslang.exe midi_demo.das -- path_to.mid
// Default: examples/media/midi/fur_elise.mid

require strudel/strudel_midi_player
require audio/audio_boost
require daslib/fio
require daslib/strings_boost

[export]
def main() {
    let media = "{get_das_root()}/examples/media"
    var path = "{media}/midi/fur_elise.mid"
    let args <- get_command_line_arguments()
    for (arg in args) {
        if (ends_with(arg, ".mid") || ends_with(arg, ".MID")) {
            path = arg
            break
        }
    }
    print("MIDI player — {path}\n")
    with_audio_system() {
        midi_load_samples(media)
        midi_init()
        if (midi_play("music", path, [gain = 0.6, looping = true])) {
            print("playing 60 seconds (looping)...\n")
            sleep(60000u)
        } else {
            print("failed to load MIDI file\n")
        }
        midi_shutdown()
        print("done\n")
    }
}
