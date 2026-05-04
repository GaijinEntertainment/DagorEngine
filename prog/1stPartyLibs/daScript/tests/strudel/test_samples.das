options gen2
options indenting = 4
options no_unused_function_arguments = false
options no_unused_block_arguments = false

require dastest/testing_boost
require strudel/strudel_samples
require audio/audio_boost
require daslib/fio
require math

let MEDIA = "{get_das_root()}/examples/media"

def read_file_bytes(path : string) : array<uint8> {
    var result : array<uint8>
    fopen(path, "rb") <| $(f) {
        if (f != null) {
            let st = fstat(f)
            if (st.is_valid) {
                result |> resize(int(st.size))
                fread(f, result)
            }
        }
    }
    return <- result
}

[test]
def test_load_audio_memory(t : T?) {
    t |> run("load OGG from memory") <| @(t : T?) {
        let bytes <- read_file_bytes("{MEDIA}/drums/bd/kick_0.ogg")
        t |> success(length(bytes) > 0, "file read ok")
        let sample <- load_audio_memory(bytes)
        t |> success(length(sample.data) > 0, "decoded audio has samples")
        t |> success(sample.channels > 0, "has channel info")
    }
}

[test]
def test_round_trip(t : T?) {
    t |> run("file vs memory loading identical") <| @(t : T?) {
        let path = "{MEDIA}/drums/bd/kick_0.ogg"
        let from_file <- load_audio_file(path)
        let bytes <- read_file_bytes(path)
        let from_mem <- load_audio_memory(bytes)
        t |> equal(length(from_file.data), length(from_mem.data), "same sample count")
        t |> equal(from_file.channels, from_mem.channels, "same channels")
        if (length(from_file.data) > 0 && length(from_mem.data) > 0) {
            // spot-check first and last samples
            t |> success(abs(from_file.data[0] - from_mem.data[0]) < 0.001, "first sample matches")
            let last = length(from_file.data) - 1
            t |> success(abs(from_file.data[last] - from_mem.data[last]) < 0.001, "last sample matches")
        }
    }
}

[test]
def test_load_sound_memory_bank(t : T?) {
    t |> run("sound added to bank") <| @(t : T?) {
        let bytes <- read_file_bytes("{MEDIA}/drums/bd/kick_0.ogg")
        var bank : SampleBank
        load_sound_memory(bytes, "TestKick", bank)
        t |> success(key_exists(bank.sounds, "testkick"), "key exists (lowercased)")
    }
}

[test]
def test_empty_data(t : T?) {
    t |> run("empty array returns empty sample") <| @(t : T?) {
        let empty : array<uint8>
        let sample <- load_audio_memory(empty)
        t |> equal(length(sample.data), 0)
    }
}

[test]
def test_invalid_data(t : T?) {
    t |> run("garbage bytes returns empty sample") <| @(t : T?) {
        var garbage : array<uint8>
        garbage |> resize(100)
        for (i in range(100)) {
            garbage[i] = uint8(i)
        }
        let sample <- load_audio_memory(garbage)
        t |> equal(length(sample.data), 0)
    }
}
