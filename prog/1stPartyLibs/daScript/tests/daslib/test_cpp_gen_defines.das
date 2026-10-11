options gen2
options no_aot

require daslib/cpp_gen
require daslib/fio
require dastest/testing_boost public

let HEADER_TEXT = ("#define TCG_HEX 0x10\n" +
    "#define TCG_DEC 42\n" +
    "#define TCG_U8 UINT8_C(0x7f)\n" +
    "#define TCG_U16 UINT16_C(0x1234)\n" +
    "#define TCG_U32 UINT32_C(0xdeadbeef)\n" +
    "#define TCG_U64 UINT64_C(0x1122334455667788)\n" +
    "#define TCG_DEC 43\n" +
    "#define TCG_TEXT \"not a constant\"\n")

def private write_header(t : T?; header : string) {
    fopen(header, "wb") $(fw) {
        if (fw == null) {
            t |> failure("cannot write {header}")
            return
        }
        fwrite(fw, HEADER_TEXT)
    }
}

[test]
def test_cpp_gen_defines(t : T?) {
    var tmp_err : string
    let tmp_dir = create_temp_directory("cpp_gen_defines", tmp_err)
    if (!empty(tmp_err)) {
        t |> failure("cannot create temp directory: {tmp_err}")
        return
    }
    let header = path_join(tmp_dir, "constants.h")
    write_header(t, header)

    let prefix = path_join(tmp_dir, "constants")
    var headers <- [ header ]
    genDefineConstants(headers, prefix)
    delete headers

    var generated = ""
    fopen("{prefix}.const_inc", "rb") $(fr) {
        if (fr != null) {
            generated = fread(fr)
        }
    }
    t |> equal(generated, ("addConstant<uint32_t>(*this,\"TCG_HEX\",uint32_t(0x10u));\n" +
        "addConstant<int32_t>(*this,\"TCG_DEC\",int32_t(42));\n" +
        "addConstant<uint8_t>(*this,\"TCG_U8\",uint8_t(0x7fu));\n" +
        "addConstant<uint16_t>(*this,\"TCG_U16\",uint16_t(0x1234u));\n" +
        "addConstant<uint32_t>(*this,\"TCG_U32\",uint32_t(0xdeadbeefu));\n" +
        "addConstant<uint64_t>(*this,\"TCG_U64\",uint64_t(0x1122334455667788ul));\n"))

    remove("{prefix}.const_inc")
    remove(header)
    rmdir(tmp_dir)
}
