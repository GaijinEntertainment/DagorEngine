options gen2
options indenting = 4

// "Ode to Joy" (Beethoven) — piano samples pitched via seq() bar notation.

require strudel/strudel
require strudel/strudel_player
require audio/audio_boost
require daslib/fio
require math

let MEDIA = "{get_das_root()}/examples/media"

// Sampled MIDI notes available in examples/media/piano/
let PIANO_NOTES = fixed_array(40, 45, 50, 55, 59, 62, 65, 71, 76)

// Map a MIDI note to the closest piano sample + pitch-shift speed
def piano_map(e : Event) : Event {
    var best = 62
    var bestDist = 100
    let midi = int(e.note)
    for (n in PIANO_NOTES) {
        let dist = abs(midi - n)
        if (dist < bestDist) {
            bestDist = dist
            best = n
        }
    }
    var r = e
    r.s = "note_{best}"
    r.speed = pow(2.0, float(midi - best) / 12.0)
    return r
}

def strudel_main() {
    // Ode to Joy — 8 bars, 4 notes per bar (4/4 time)
    let melody <- seq(
        "e4 e4 f4 g4 | g4 f4 e4 d4 | c4 c4 d4 e4 | e4 d4 d4 ~ |
         e4 e4 f4 g4 | g4 f4 e4 d4 | c4 c4 d4 e4 | d4 c4 c4 ~"
    )
    // map note events to piano samples with pitch-shifting
    var piano <- melody |> set_gain(0.6) |> fmap(@(e) => piano_map(e))
    var drums <- s("bd ~ sd ~") |> set_gain(0.5)
    strudel_add_track(stack([piano, drums]))
    strudel_set_bpm(100.0lf)
    strudel_play()
}

[export]
def main() {
    print("daStrudel piano demo — Ode to Joy (piano samples)\n")
    with_audio_system() {
        strudel_load_sound("{MEDIA}/drums/bd", "bd")
        strudel_load_sound("{MEDIA}/drums/sd", "sd")
        for (n in PIANO_NOTES) {
            strudel_load_sound("{MEDIA}/piano/note_{n}", "note_{n}")
        }
        strudel_init(@@strudel_main)
        print("playing 20 seconds...\n")
        sleep(20000u)
        strudel_shutdown()
    }
    print("done\n")
}
