options gen2
options rtti
options indenting = 4
options no_unused_block_arguments = false
options no_unused_function_arguments = false
options no_global_variables = false
options persistent_heap

module lockfile shared public

require daslib/fio
require strings
require daslib/json
require daslib/json_boost

struct PackageEntry {
    name : string
    source : string
    version : string           // requested version (e.g. "1.0" from install foo@1.0)
    tag : string               // resolved git tag (e.g. "v1.0" after resolve())
    branch : string            // resolved git branch (if branch-based)
    root : bool = true
    local : bool = false
    global : bool = false      // installed in das_root/modules/ (shared across projects)
}

struct LockFile {
    sdk_version : string
    packages : array<PackageEntry>
}

let LOCK_FILE_NAME = "daspkg.lock"
let GLOBAL_LOCK_FILE_NAME = ".daspkg_global.lock"

def lock_file_path(project_root : string) : string {
    return "{project_root}/{LOCK_FILE_NAME}"
}

def global_lock_file_path() : string {
    return "{get_das_root()}/modules/{GLOBAL_LOCK_FILE_NAME}"
}

// find index of package by name, -1 if not found
def find_package(lf : LockFile; name : string) : int {
    for (i in range(length(lf.packages))) {
        if (lf.packages[i].name == name) {
            return i
        }
    }
    return -1
}

def has_package(lf : LockFile; name : string) : bool {
    return find_package(lf, name) >= 0
}

def get_package(lf : LockFile; name : string) : PackageEntry {
    let idx = find_package(lf, name)
    if (idx >= 0) {
        return lf.packages[idx]
    }
    return PackageEntry()
}

// add or replace a package entry (appends if new, replaces in-place if existing)
def set_package(var lf : LockFile; entry : PackageEntry) {
    let idx = find_package(lf, entry.name)
    if (idx >= 0) {
        lf.packages[idx] = entry
    } else {
        lf.packages |> push_clone(entry)
    }
}

def remove_package(var lf : LockFile; name : string) {
    let idx = find_package(lf, name)
    if (idx >= 0) {
        lf.packages |> erase(idx)
    }
}

def read_lock_file_at(path : string; var lf : LockFile) : bool {
    var ok = false
    fopen(path, "rb") <| $(f) {
        if (f == null) {
            return
        }
        let text = fread(f)
        var error : string
        var parsed = read_json(text, error)
        if (parsed == null) {
            print("Error reading {path}: {error}\n")
            return
        }
        lf = from_JV(parsed, type<LockFile>)
        ok = true
    }
    return ok
}

def write_lock_file_at(path : string; lf : LockFile) : bool {
    let text = sprint_json(lf, true)
    var ok = false
    fopen(path, "wb") <| $(f) {
        if (f == null) {
            print("Error: cannot write {path}\n")
            return
        }
        fwrite(f, text)
        fwrite(f, "\n")
        ok = true
    }
    return ok
}

def read_lock_file(project_root : string; var lf : LockFile) : bool {
    return read_lock_file_at(lock_file_path(project_root), lf)
}

def write_lock_file(project_root : string; lf : LockFile) : bool {
    return write_lock_file_at(lock_file_path(project_root), lf)
}

def read_global_lock_file(var lf : LockFile) : bool {
    return read_lock_file_at(global_lock_file_path(), lf)
}

def write_global_lock_file(lf : LockFile) : bool {
    return write_lock_file_at(global_lock_file_path(), lf)
}
