options gen2
options no_aot

require dastest/testing_boost
require daslib/ast_boost
require daslib/fio
require _llvm_cache_common

def private parse_round(client, blob : string; var res : SplitRound) : bool {
    var wrote = false
    compile_for_jit(client, false) $(var program) {
        var ser = create_ast_serializer()
        if (serialize_program(ser, program)) {
            ast_serializer_get_data(ser) $(data) {
                fopen(blob, "wb") $(bf) {
                    if (bf != null) {
                        bf |> fwrite(data)
                        wrote = true
                    }
                }
            }
        }
        delete_ast_serializer(ser)
        res = split_round(program)
    }
    return wrote
}

def private deser_round(blob : string; var res : SplitRound) : bool {
    var read_back = false
    ast_gc_guard() {
        var data : array<uint8>
        fopen(blob, "rb") $(f) {
            if (f != null) {
                data |> resize(int64(stat(blob).size))
                fread(f, data)
            }
        }
        if (empty(data)) {
            return
        }
        var ser = create_ast_deserializer(data)
        var inscope access <- make_file_access("")
        access |> add_extra_module("llvm_jit_finalizer",
            "{get_das_root()}/modules/dasLLVM/daslib/llvm_jit_finalizer.das")
        using() $(var mg : ModuleGroup) {
            deserialize_program(ser, access, unsafe(addr(mg))) $(ok, program, _error) {
                if (!ok || program == null) {
                    return
                }
                read_back = true
                res = split_round(program)
            }
        }
        delete_ast_serializer(ser)
    }
    return read_back
}

[test]
def test_jit_ast_roundtrip(t : T?) {
    if (!jit_enabled() || !das_is_dll_build()) {
        t |> skip("needs -jit and a DLL daslang build")
        return
    }
    let client = stage_split_client(t, "das_astrt")
    if (empty(client)) return
    let blob = "{dir_name(client)}/ast.bin"
    var parsed : SplitRound
    t |> success(parse_round(client, blob, parsed), "round 1 parsed: the program serialized")
    t |> success(parsed.ok, "round 1 parsed: the backend ran")
    t |> equal(parsed.partitions, 3, "round 1 parsed: one partition per das module")
    t |> equal(parsed.cached, 0, "round 1 parsed: nothing was cached to link")
    t |> equal(parsed.acc, 340, "round 1 parsed: the cross-partition answer")
    var deser : SplitRound
    t |> success(deser_round(blob, deser), "round 2 deserialized: the blob read back")
    t |> success(deser.ok, "round 2 deserialized: the backend ran")
    t |> equal(deser.partitions, 0, "round 2 deserialized: nothing re-emitted")
    t |> strictEqual(deser.output_path, parsed.output_path,
        "round 2 deserialized: every function hash survived the round trip")
    t |> equal(deser.acc, 340, "round 2 deserialized: the cross-partition answer")
    var err : string
    rmdir_rec(dir_name(parsed.output_path), err)
    rmdir_rec(dir_name(client), err)
}
