options gen2
options indenting = 4

require audio/audio_boost
require audio/audio_wav
require strudel/strudel
require strudel/strudel_player
require daslib/fio

[export]
def main() {
    print("daStrudel player\n")
    let media = "{get_das_root()}/examples/media"
    with_audio_system() {
        strudel_load_sound("{media}/drums/bd", "bd")
        strudel_load_sound("{media}/drums/sd", "sd")
        strudel_load_sound("{media}/drums/hh", "hh")
        strudel_load_sound("{media}/drums/cp", "cp")
        strudel_init(@@strudel_main)
        print("playing 8 seconds...\n")
        sleep(8000u)
        print("shutting down...\n")
        strudel_shutdown()
    }
    print("done\n")
}

def strudel_main() {
    strudel_add_track(s("bd sd [hh hh] cp"))
    strudel_set_bpm(120.0lf)
    strudel_play()
}
