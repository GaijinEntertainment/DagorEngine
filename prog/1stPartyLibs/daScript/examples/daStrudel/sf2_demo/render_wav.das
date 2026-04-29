options gen2
options indenting = 4

// Offline render of the SF2 demo pattern to WAV file.

require strudel/strudel
require strudel/strudel_player
require strudel/strudel_sf2
require strudel/strudel_sf2_voice
require audio/audio_wav
require audio/audio_boost
require audio
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

[export]
def main() {
    let sf2_path = find_sf2()
    if (empty(sf2_path)) {
        print("No SF2 found\n")
        return
    }
    let sf2 = sf2_load(sf2_path)
    if (sf2 == null) {
        print("Failed to load SF2\n")
        return
    }
    print("Loaded SF2: {sf2_path}\n")

    // build the pattern
    let pat <- stack([
        // drums — SF2 GM percussion (bank 128)
        note_pattern("c2 ~ [~ c2] ~, ~ eb2 ~ eb2, [~ fs2] [~ fs2] [~ fs2] [~ fs2]")
            |> set_sf2(0) |> set_sf2_bank(128) |> set_gain(0.7),
        // piano arpeggios
        note_pattern("<[a3 c4 e4 a4 e4 c4 a3 c4] [f3 a3 c4 f4 c4 a3 f3 a3] [c3 e3 g3 c4 g3 e3 c3 e3] [g3 b3 d4 g4 d4 b3 g3 b3]>")
            |> set_sf2("piano") |> set_gain(0.45),
        // bass
        note_pattern("<[a2 ~ ~ a2 ~ a2 ~ ~] [f2 ~ ~ f2 ~ f2 ~ ~] [c2 ~ ~ c2 ~ c2 ~ ~] [g2 ~ ~ g2 ~ g2 ~ ~]>")
            |> set_sf2("acoustic_bass") |> set_gain(0.6),
        // string pad
        note_pattern("<[a2,c3,e3] [f2,a2,c3] [c2,e2,g2] [g2,b2,d3]>")
            |> set_sf2("string_ensemble") |> set_gain(0.2)
    ])

    // set up scheduler
    var sched = Scheduler()
    sched.cps = 120.0lf / 240.0lf  // 120 BPM (default), 4 beats per cycle
    sched.sf2 = sf2
    init_reverb(sched)

    // render 22 seconds in 50ms chunks
    let duration = 22.0lf
    let chunk_sec = 0.05
    var wall_time = 0.0lf
    let bank = SampleBank()
    var output : array<float>
    let total_samples = int(duration * 48000.0lf) * 2
    output |> reserve(total_samples)

    print("Rendering {duration} seconds...\n")
    while (wall_time < duration) {
        var chunk <- tick(sched, pat, bank, wall_time, chunk_sec)
        for (s in chunk) {
            output |> push(s)
        }
        delete chunk
        wall_time += double(chunk_sec)
    }

    let wav_path = "E:/sf2_demo.wav"
    print("Writing {wav_path} ({length(output) / 2} frames)\n")
    write_wav(wav_path, output, [sample_rate = 48000u, channels = 2])
    print("Done!\n")
}
