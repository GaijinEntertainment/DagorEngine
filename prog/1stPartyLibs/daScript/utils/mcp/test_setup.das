options gen2
options no_aot
options no_unused_block_arguments = false

require dastest/testing_boost public
require daslib/fio
require daslib/json_boost
require setup

//! emit_mcp_config against a scratch root: the daslang entry written fresh, merged into a file
//! that carries other servers and its own defer_loading, refused on a file that is not a JSON
//! object, and left intact when the old file cannot be moved aside

def private make_root(t : T?) : string {
    let tmp = create_temp_directory_result("das_mcp_setup")
    if (!(tmp is value)) {
        t |> failure("could not create temp directory: {tmp as error}")
        return ""
    }
    return tmp as value
}

def private write_text(path, text : string) {
    fopen(path, "wb") $(f) {
        return if (f == null)
        fwrite(f, text)
    }
}

def private read_doc(path : string) : JsonValue? {
    var err : string
    return read_json(fread(path), err)
}

def private json_array_has(arr : JsonValue?; needle : string) : bool {
    return false if (arr == null || !(arr.value is _array))
    for (item in arr.value as _array) {
        if (item.value is _string && (item.value as _string) == needle) return true
    }
    return false
}

def private string_of(js : JsonValue?) : string {
    return js != null && js.value is _string ? js.value as _string : ""
}

def private no_leftovers(t : T?; root : string) {
    t |> success(!fexist(path_join(root, ".mcp.json.tmp")), "no .mcp.json.tmp is left behind")
    t |> success(!fexist(path_join(root, ".mcp.json.bak")), "no .mcp.json.bak is left behind")
}

let private SEEDED = "\{\"mcpServers\":\{\"github\":\{\"command\":\"gh\"\},\"daslang\":\{\"command\":\"python3\",\"args\":[\"x\"],\"defer_loading\":true\}\}\}\n"

[test]
def test_emit_dap_entry(t : T?) {
    t |> run("the DAP bridge gets its own entry over this root's binary") @(t : T?) {
        let root = make_root(t)
        return if (empty(root))
        t |> success(emit_mcp_config(root, "bin/daslang"), "emit_mcp_config reports success")
        var doc = read_doc(path_join(root, ".mcp.json"))
        let dap = doc?["mcpServers"]?["daslang-dap"]
        t |> success(dap != null, "the daslang-dap entry is written")
        t |> equal(string_of(dap?["command"]), path_join(root, "bin/daslang"), "no watchdog exe beside the binary: the interpreter host is the command")
        t |> success(json_array_has(dap?["args"], "--stdio"), "the front is asked for")
        t |> success(json_array_has(dap?["args"], "daslang-dap"), "named daslang-dap")
        t |> success(json_array_has(dap?["args"], "utils/dap/main.das"), "it runs the bridge")
        t |> success(json_array_has(dap?["args"], "--repo-root"), "rooted at the tree")
        t |> success(json_array_has(dap?["args"], path_join(root, "bin/daslang")), "--executable names this root's binary")
        unsafe {
            delete doc
        }
        no_leftovers(t, root)
    }
}

[test]
def test_emit_mcp_config(t : T?) {
    t |> run("a root with no .mcp.json gets the front over its own binary") @(t : T?) {
        let root = make_root(t)
        return if (empty(root))
        t |> success(emit_mcp_config(root, "bin/daslang"), "emit_mcp_config reports success")
        var doc = read_doc(path_join(root, ".mcp.json"))
        let entry = doc?["mcpServers"]?["daslang"]
        t |> success(entry != null, "the daslang entry is written")
        t |> equal(string_of(entry?["command"]), path_join(root, "bin/daslang"), "no watchdog exe beside the binary: the interpreter host is the command")
        t |> success(json_array_has(entry?["args"], "utils/watchdog/main.das"), "the interpreter host runs the watchdog's main.das")
        t |> success(json_array_has(entry?["args"], "--stdio"), "the front is asked for")
        t |> success(json_array_has(entry?["args"], root), "--cwd names the root")
        let herd = doc?["mcpServers"]?["dasherd"]
        t |> success(herd == null || herd.value is _null, "no dasherd entry without the das-herd shim")
        unsafe {
            delete doc
        }
        no_leftovers(t, root)
    }
    t |> run("an existing file keeps its other servers and the entry's defer_loading") @(t : T?) {
        let root = make_root(t)
        return if (empty(root))
        write_text(path_join(root, ".mcp.json"), SEEDED)
        t |> success(emit_mcp_config(root, "bin/daslang"), "emit_mcp_config reports success")
        var doc = read_doc(path_join(root, ".mcp.json"))
        t |> equal(string_of(doc?["mcpServers"]?["github"]?["command"]), "gh", "the other server is untouched")
        let entry = doc?["mcpServers"]?["daslang"]
        t |> equal(string_of(entry?["command"]), path_join(root, "bin/daslang"), "the daslang entry is replaced")
        let deferred = entry?["defer_loading"]
        t |> success(deferred != null && deferred.value is _bool && (deferred.value as _bool), "defer_loading survives the replacement")
        unsafe {
            delete doc
        }
        no_leftovers(t, root)
    }
    t |> run("the das-herd shim gets its own entry") @(t : T?) {
        let root = make_root(t)
        return if (empty(root))
        let shim = path_join(root, "utils/internal/das-herd/mcp_main.das")
        mkdir(path_join(root, "utils"))
        mkdir(path_join(root, "utils/internal"))
        mkdir(path_join(root, "utils/internal/das-herd"))
        write_text(shim, "options gen2\n")
        t |> success(emit_mcp_config(root, "bin/daslang"), "emit_mcp_config reports success")
        var doc = read_doc(path_join(root, ".mcp.json"))
        let entry = doc?["mcpServers"]?["dasherd"]
        t |> success(entry != null, "the dasherd entry is written")
        t |> success(json_array_has(entry?["args"], "utils/internal/das-herd/mcp_main.das"), "it runs the shim")
        unsafe {
            delete doc
        }
    }
    t |> run("a file that is not a JSON object is refused and left alone") @(t : T?) {
        let root = make_root(t)
        return if (empty(root))
        let path = path_join(root, ".mcp.json")
        write_text(path, "nope\n")
        t |> success(!emit_mcp_config(root, "bin/daslang"), "unparseable: refused")
        t |> equal(fread(path), "nope\n", "unparseable: the file is untouched")
        write_text(path, "\{\"mcpServers\":3\}\n")
        t |> success(!emit_mcp_config(root, "bin/daslang"), "non-object mcpServers: refused")
        t |> equal(fread(path), "\{\"mcpServers\":3\}\n", "non-object mcpServers: the file is untouched")
        no_leftovers(t, root)
    }
    t |> run("the old file stays when it cannot be moved aside") @(t : T?) {
        let root = make_root(t)
        return if (empty(root))
        let path = path_join(root, ".mcp.json")
        write_text(path, SEEDED)
        mkdir(path_join(root, ".mcp.json.bak"))
        write_text(path_join(root, ".mcp.json.bak/occupied"), "x")
        t |> success(!emit_mcp_config(root, "bin/daslang"), "a non-empty directory in the way of the move-aside: refused")
        t |> equal(fread(path), SEEDED, "the old configuration is intact")
        t |> success(fexist(path_join(root, ".mcp.json.tmp")), "the new configuration is left in .mcp.json.tmp")
    }
}
