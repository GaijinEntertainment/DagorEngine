options gen2
options indenting = 4

// SF2 SoundFont lo-fi chill beat — all sounds from SF2, including drums.
// Am - F - C - G chord progression with arpeggiated piano, walking bass, and pad.
// Place FluidR3_GM.sf2 or GeneralUser GS v1.471.sf2 in the midi_sf2_demo folder.
// Usage: daslang.exe main.das

require strudel/strudel
require strudel/strudel_player
require audio/audio_boost
require daslib/fio

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
        // piano arpeggios — broken chords, Am-F-C-G
        note_pattern("<[a3 c4 e4 a4 e4 c4 a3 c4] [f3 a3 c4 f4 c4 a3 f3 a3] [c3 e3 g3 c4 g3 e3 c3 e3] [g3 b3 d4 g4 d4 b3 g3 b3]>")
            |> set_sf2("piano") |> set_gain(0.45),
        // bass — root notes with a bit of movement
        note_pattern("<[a2 ~ ~ a2 ~ a2 ~ ~] [f2 ~ ~ f2 ~ f2 ~ ~] [c2 ~ ~ c2 ~ c2 ~ ~] [g2 ~ ~ g2 ~ g2 ~ ~]>")
            |> set_sf2("acoustic_bass") |> set_gain(0.6),
        // string pad — sustained chords
        note_pattern("<[a2,c3,e3] [f2,a2,c3] [c2,e2,g2] [g2,b2,d3]>")
            |> set_sf2("string_ensemble") |> set_gain(0.2)
    ]))
    strudel_play()
}

[export]
def main() {
    let sf2 = find_sf2()
    if (empty(sf2)) {
        print("No SF2 found — place FluidR3_GM.sf2 or GeneralUser GS in midi_sf2_demo/\n")
        print("Running anyway (SF2 events will be silent, drums will play)\n")
    }
    with_audio_system() {
        if (!empty(sf2)) {
            if (strudel_load_sf2(sf2)) {
                print("Loaded SF2: {sf2}\n")
            } else {
                print("Failed to load SF2: {sf2}\n")
            }
        }
        print("daStrudel SF2 demo — lo-fi chill (Am-F-C-G)\n")
        strudel_init(@@strudel_main)
        print("playing 30 seconds...\n")
        sleep(30000u)
        strudel_shutdown()
    }
    print("done\n")
}
