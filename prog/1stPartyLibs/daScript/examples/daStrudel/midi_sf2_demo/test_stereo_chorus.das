options gen2
options indenting = 4

// Quick verification: stereo-linked samples + chorus
// Run: bin/Release/daslang.exe examples/daStrudel/midi_sf2_demo/test_stereo_chorus.das

require strudel/strudel_sf2
require strudel/strudel_sf2_voice
require strudel/strudel_midi_player
require audio
require daslib/fio
require math

let SF2_DIR = "{get_das_root()}/examples/daStrudel/midi_sf2_demo"

[export]
def main() {
    // ── Load SF2 ──
    var st : FStat
    let paths = fixed_array("{SF2_DIR}/FluidR3_GM.sf2", "{SF2_DIR}/Musyng Kite.sf2", "{SF2_DIR}/GeneralUser GS v1.471.sf2")
    var sf2_path = ""
    for (p in paths) {
        if (stat(p, st)) {
            sf2_path = p
            break
        }
    }
    if (empty(sf2_path)) {
        print("No SF2 found in {SF2_DIR}\n")
        return
    }
    var sf2 = sf2_load(sf2_path)
    if (sf2 == null) {
        print("Failed to load {sf2_path}\n")
        return
    }
    print("Loaded: {sf2.name}\n")

    // ── Count stereo-linked samples ──
    var mono_count = 0
    var left_count = 0
    var right_count = 0
    var link_ok = 0
    var adj_ok = 0
    for (i in range(length(sf2.samples))) {
        let sh = sf2.samples[i]
        if (sh.sample_type == SF2_SAMPLE_MONO) { mono_count++; }
        elif (sh.sample_type == SF2_SAMPLE_LEFT) {
            left_count++
            let link = int(sh.sample_link)
            if (link >= 0 && link < length(sf2.samples) && sf2.samples[link].sample_type == SF2_SAMPLE_RIGHT) {
                link_ok++
            } elif (i + 1 < length(sf2.samples) && sf2.samples[i + 1].sample_type == SF2_SAMPLE_RIGHT) {
                adj_ok++
            }
        } elif (sh.sample_type == SF2_SAMPLE_RIGHT) { right_count++; }
    }
    print("Samples: {length(sf2.samples)} total, {mono_count} mono, {left_count} left, {right_count} right\n")
    print("Stereo pairs resolved: {link_ok} via sample_link, {adj_ok} via adjacent fallback\n")

    // ── Find a stereo preset and render a note ──
    if (left_count > 0) {
        // scan presets to find one with a stereo-linked zone
        var found_stereo = false
        for (pi in range(length(sf2.presets))) {
            if (found_stereo) { break; }
            let zones = sf2_find_zones(*sf2, pi, 60, 100)
            for (z in zones) {
                let inst = sf2.instruments[z.x]
                let izone = inst.zones[z.y]
                let sid = get_generator(*izone, int(SF2GenOper.sampleID), -1)
                if (sid >= 0 && sid < length(sf2.samples) && sf2.samples[sid].sample_type == SF2_SAMPLE_LEFT) {
                    let preset = sf2.presets[pi]
                    print("\nFound stereo preset: bank {preset.bank} preset {preset.preset} \"{preset.name}\"\n")
                    print("  Left sample: \"{sf2.samples[sid].name}\"  ->  linked to #{sf2.samples[sid].sample_link}\n")
                    let link_idx = int(sf2.samples[sid].sample_link)
                    if (link_idx >= 0 && link_idx < length(sf2.samples)) {
                        print("  Right sample: \"{sf2.samples[link_idx].name}\"\n")
                    }
                    // create voice and check stereo flag
                    var pzone : SF2Zone const?
                    for (pz in preset.zones) {
                        if (get_generator(*pz, int(SF2GenOper.instrument), -1) == z.x) {
                            pzone = pz
                            break
                        }
                    }
                    var cv : ma_sf2_voice
                    if (sf2_create_c_voice(*sf2, z.x, z.y, pzone, preset.global_zone, 60, 100, 0, cv)) {
                        print("  Voice stereo flag: {cv.stereo}\n")
                        print("  L sample range: {cv.sample_start}..{cv.sample_end}\n")
                        print("  R sample range: {cv.sample_start_r}..{cv.sample_end_r}\n")
                        print("  pan: {cv.pan}  attenuation: {cv.attenuation}\n")
                        // render 480 frames (10ms at 48kHz) and compare L vs R
                        var buf : array<float>
                        buf |> resize(480 * 2)
                        for (s in buf) { s = 0.0; }
                        unsafe {
                            ma_sf2_voice_render(addr(cv), addr(sf2.sample_data[0]), length(sf2.sample_data), addr(buf[0]), 0, 480)
                        }
                        // also check raw sample data at R start
                        let r_start = cv.sample_start_r
                        var r_data_nonzero = 0
                        for (ri in range(min(100, cv.sample_end_r - r_start))) {
                            if (sf2.sample_data[r_start + ri] != int16(0)) {
                                r_data_nonzero++
                            }
                        }
                        print("  R raw samples non-zero (first 100): {r_data_nonzero}\n")
                        var sum_l = 0.0
                        var sum_r = 0.0
                        var diff_count = 0
                        for (i in range(480)) {
                            sum_l += abs(buf[i * 2])
                            sum_r += abs(buf[i * 2 + 1])
                            if (abs(buf[i * 2] - buf[i * 2 + 1]) > 1e-6) {
                                diff_count++
                            }
                        }
                        print("  L energy: {sum_l:.4f}  R energy: {sum_r:.4f}\n")
                        print("  Frames where L != R: {diff_count} / 480\n")
                        if (diff_count > 0) {
                            print("  STEREO CONFIRMED - left and right channels differ\n")
                        } else {
                            print("  MONO - channels are identical (stereo not working?)\n")
                        }
                    }
                    found_stereo = true
                    break
                }
            }
        }
    } else {
        print("No stereo-linked samples in this SF2\n")
    }

    // ── Chorus send check ──
    print("\n── Chorus send ──\n")
    var chorus_presets = 0
    for (pi in range(length(sf2.presets))) {
        let zones = sf2_find_zones(*sf2, pi, 60, 100)
        for (z in zones) {
            let preset = sf2.presets[pi]
            let csend = sf2_get_chorus_send(*sf2, z.x, z.y, null, preset.global_zone)
            if (csend > 0.0) {
                if (chorus_presets < 5) {
                    print("  bank {preset.bank} preset {preset.preset} \"{preset.name}\": chorus_send = {csend:.3f}\n")
                }
                chorus_presets++
            }
            break  // one zone per preset is enough
        }
    }
    print("Total presets with chorus send > 0: {chorus_presets}\n")
    print("\nDone. Run the MIDI demo to hear it live:\n  bin/Release/daslang.exe examples/daStrudel/midi_sf2_demo/main.das\n")
}
