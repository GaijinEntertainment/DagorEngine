options gen2
options no_global_variables = false
options indenting = 4
options strict_smart_pointers = true

module daspkg shared public

// Package manager API for .das_package manifests.
//
// .das_package scripts `require daslib/daspkg` and call these functions
// to register package metadata, version resolution, dependencies, and build info.
// daspkg (utils/daspkg/main.das) compiles and runs the .das_package, then reads
// the accumulated state from the module globals.

// --- Package metadata result (used by daspkg tool and .das_package scripts) ---

struct PackageMeta {
    pkg_name : string
    author : string
    description : string
    source : string
    license : string
    tags : array<string>
    min_sdk : string
}

// --- Package metadata globals (set by `package()`) ---

var _pkg_meta : PackageMeta

def package_name(name : string) {
    _pkg_meta.pkg_name = name
}

def package_author(author : string) {
    _pkg_meta.author = author
}

def package_description(desc : string) {
    _pkg_meta.description = desc
}

def package_source(url : string) {
    _pkg_meta.source = url
}

def package_license(license : string) {
    _pkg_meta.license = license
}

def package_tag(tag : string) {
    _pkg_meta.tags |> push(tag)
}

def package_tags(tags : array<string>) {
    _pkg_meta.tags |> push_clone_from(tags)
}

def package_min_sdk(version : string) {
    _pkg_meta.min_sdk = version
}

// --- Version resolution (set by `resolve()`) ---

enum DownloadKind {
    unset
    tag
    branch
    redirect
}

var _download_kind : DownloadKind = DownloadKind.unset
var _download_ref : string
var _download_source : string

def download_tag(tag : string) {
    _download_kind = DownloadKind.tag
    _download_ref = tag
}

def download_branch(branch : string) {
    _download_kind = DownloadKind.branch
    _download_ref = branch
}

def download_redirect(source, ref : string) {
    _download_kind = DownloadKind.redirect
    _download_source = source
    _download_ref = ref
}

// --- Dependencies (accumulated by `dependencies()`) ---

struct Dependency {
    source : string
    version_constraint : string
}

var _dependencies : array<Dependency>

def require_package(source : string; version_constraint : string = "") {
    _dependencies |> emplace(Dependency(source = source, version_constraint = version_constraint))
}

// --- Build info (set by `build()`) ---

enum BuildKind {
    no_build
    cmake
    custom
}

var _build_kind : BuildKind = BuildKind.no_build
var _build_command : string

def cmake_build() {
    _build_kind = BuildKind.cmake
}

def no_build() {
    _build_kind = BuildKind.no_build
}

def custom_build(command : string) {
    _build_kind = BuildKind.custom
    _build_command = command
}

// --- Release info (set by `release()`) ---

struct ReleaseSpec {
    name : string                       //!< bundle name; defaults to package_name
    main_script : string                //!< path to project's main .das (relative to root)
    wasm_main_script : string           //!< entry for `daspkg release wasm` when it differs from main_script (e.g. a live_stub variant); falls back to main_script
    include_globs : array<string>       //!< asset globs to ship
    include_if_missing_globs : array<string> //!< mutable starter files to copy only when absent; never release-owned or overwritten
    exclude_globs : array<string>       //!< asset globs to skip
    forced_modules : array<string>      //!< force-include shared module by das_name (auto-detection misses media-only or runtime-loaded packages)
    native_dlls : array<string>         //!< native shared libraries to ship next to this package's .shared_module (e.g. "glfw3.dll" / "libglfw.so.3.4"); resolved against <das_root>/bin/ (Windows) or <das_root>/lib/ (POSIX)
    bundle_id : string                  //!< macOS .app CFBundleIdentifier (reverse-DNS); defaults to com.<author>.<name>
    emcc_args : array<string>           //!< extra emcc flags this module contributes to a `daspkg release wasm` link (e.g. "-sUSE_GLFW=3"); gathered from every referenced module
    embed_paths : array<string>         //!< "src@dst" embed pairs this module needs in the wasm MEMFS (e.g. a HUD font); gathered from every referenced module
    web_shell : string                  //!< optional emcc `--shell-file` HTML for `release wasm`; empty = daslang's minimal canvas shell
    wasm_build_command : string         //!< shell command that builds this EXTERNAL module's wasm archives (run with cwd = module dir); empty = in-tree module built by `daspkg build --wasm`
    wasm_archives : array<string>       //!< archives required by wasm link; build outputs are module-relative, SDK archives may be named from the staged wasm lib dir
    requires_jit : bool                 //!< app is JIT-only (per-box [tune]/[llvm_code] kernels); a baked -exe would run broken, so `daspkg release` refuses it
    include_symbols : bool              //!< ship debug symbols for every shipped binary into `<bundle>/symbols/`
    external_files : array<string>      //!< "src@dst" pairs shipped from outside the package; src is relative to <das_root>, dst to the bundle root
}

var _release_spec : ReleaseSpec

def release_name(name : string) {
    _release_spec.name = name
}

def release_main(script : string) {
    _release_spec.main_script = script
}

//! Mark the app JIT-only: its kernels are per-box JIT-compiled/tuned ([llvm_code]/[tune]) and a
//! baked standalone -exe would run broken (or refuse to start). `daspkg release` then fails with
//! a clear message instead of producing a dead binary. Such an app deploys as `daslang -jit <main>`.
def release_requires_jit() {
    _release_spec.requires_jit = true
}

//! Ship debug symbols for every shipped binary into `<bundle>/symbols/`, so a minidump from the
//! deployed tree resolves (`cdb -y <bundle>\symbols`). Opt-in: symbols dwarf the payload (daslang's
//! runtime PDBs are ~320 MB). Windows `.pdb` only; see `skills/daspkg.md`.
def release_include_symbols() {
    _release_spec.include_symbols = true
}

//! Entry script for `daspkg release wasm` (the wasm64 build) when it differs from the native release_main() — e.g. a live_stub variant that drops the desktop live-reload stack (which can't cross-compile). Falls back to release_main() when unset.
def release_wasm_main(script : string) {
    _release_spec.wasm_main_script = script
}

def release_include(pattern : string) {
    _release_spec.include_globs |> push(pattern)
}

//! Ship starter content only when the destination does not already exist.
def release_include_if_missing(pattern : string) {
    _release_spec.include_if_missing_globs |> push(pattern)
}

//! Ship a file that lives OUTSIDE this package — `release_include` only globs downward from the package root, so shared tooling (e.g. `utils/watchdog/watchdog.py`) is unreachable to it. `source` is relative to `<das_root>`; `dest` is relative to the bundle root and defaults to `source`'s file name. Errors loudly if the source is missing, so a moved file fails the release instead of silently shipping nothing.
def release_include_from(source, dest : string) {
    _release_spec.external_files |> push("{source}@{dest}")
}

def release_include_from(source : string) {
    // Empty dest = "use the source file name"; resolved by the shipper, which has fio. This
    // module is requires-free by design, so base_name() is not available here.
    _release_spec.external_files |> push("{source}@")
}

def release_exclude(pattern : string) {
    _release_spec.exclude_globs |> push(pattern)
}

def release_shared_module(name : string) {
    _release_spec.forced_modules |> push(name)
}

//! Declare a native shared library that must be shipped next to this package's `.shared_module` in a release bundle. `name` is the platform-specific filename (e.g. `"glfw3.dll"` on Windows, `"libglfw.so.3.4"` on Linux, `"libglfw.3.4.dylib"` on macOS). daspkg searches `<das_root>/bin/` on Windows and `<das_root>/lib/` on POSIX, then copies the file into `<bundle>/modules/<package>/`. Errors loudly if the file is not found — authors should guard per-platform with `if (get_platform_name() == "windows")`.
def release_include_dll(name : string) {
    _release_spec.native_dlls |> push(name)
}

def release_bundle_id(id : string) {
    _release_spec.bundle_id = id
}

//! Contribute an extra emcc flag to a `daspkg release wasm` link. Declared in a module's `.das_package` so the flag is added whenever a released app links that module (e.g. dasGlfw declares `release_emcc_arg("-sUSE_GLFW=3")`).
def release_emcc_arg(arg : string) {
    _release_spec.emcc_args |> push(arg)
}

//! Declare a file a module needs embedded in the wasm MEMFS for `release wasm` (emcc `--embed-file src@dst`). `src` is relative to the module dir (this .das_package's location, like release_main / release_wasm_archive), so it resolves for an external module under <project>/modules/<dep> too; `dst` is the absolute MEMFS path the runtime opens at (e.g. dasStbImage embeds its HUD font at `/modules/dasStbImage/fonts/droidsansmono.ttf`).
def release_embed_file(src, dst : string) {
    _release_spec.embed_paths |> push("{src}@{dst}")
}

//! Choose the emcc `--shell-file` HTML wrapper for `release wasm`. Empty (the
//! default) ships daslang's minimal canvas-only shell. A site can point this at
//! its own shell (e.g. the forge canvas frame) for in-page embedding.
def release_web_shell(path : string) {
    _release_spec.web_shell = path
}

//! Declare how to build this module's wasm archives for `daspkg release wasm`, used when the module is an EXTERNAL package (not part of daslang's in-tree web build, so `daspkg build --wasm` does not produce its archives). The command runs with the module directory as the working directory and an emscripten SDK already on PATH;
def release_wasm_build(command : string) {
    _release_spec.wasm_build_command = command
}

//! Declare a wasm archive needed by this module's `release wasm` link. Paths produced by release_wasm_build() are relative to the module directory; a bare filename may name an archive already staged by `daspkg build --wasm`. Every declared archive is linked in declaration order, so repeat this for the module archive and its private static dependencies.
def release_wasm_archive(path : string) {
    _release_spec.wasm_archives |> push(path)
}

