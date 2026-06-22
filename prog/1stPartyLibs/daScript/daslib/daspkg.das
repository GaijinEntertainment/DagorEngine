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
    _pkg_meta.tags |> push_clone(tag)
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
    include_globs : array<string>       //!< asset globs to ship
    exclude_globs : array<string>       //!< asset globs to skip
    forced_modules : array<string>      //!< force-include shared module by das_name (auto-detection misses media-only or runtime-loaded packages)
    native_dlls : array<string>         //!< native shared libraries to ship next to this package's .shared_module (e.g. "glfw3.dll" / "libglfw.so.3.4"); resolved against <das_root>/bin/ (Windows) or <das_root>/lib/ (POSIX)
    bundle_id : string                  //!< macOS .app CFBundleIdentifier (reverse-DNS); defaults to com.<author>.<name>
}

var _release_spec : ReleaseSpec

def release_name(name : string) {
    _release_spec.name = name
}

def release_main(script : string) {
    _release_spec.main_script = script
}

def release_include(pattern : string) {
    _release_spec.include_globs |> push(pattern)
}

def release_exclude(pattern : string) {
    _release_spec.exclude_globs |> push(pattern)
}

def release_shared_module(name : string) {
    _release_spec.forced_modules |> push(name)
}

//! Declare a native shared library that must be shipped next to this package's
//! `.shared_module` in a release bundle. `name` is the platform-specific filename
//! (e.g. `"glfw3.dll"` on Windows, `"libglfw.so.3.4"` on Linux,
//! `"libglfw.3.4.dylib"` on macOS). daspkg searches `<das_root>/bin/` on Windows
//! and `<das_root>/lib/` on POSIX, then copies the file into
//! `<bundle>/modules/<package>/`. Errors loudly if the file is not found —
//! authors should guard per-platform with `if (get_platform_name() == "windows")`.
def release_include_dll(name : string) {
    _release_spec.native_dlls |> push(name)
}

def release_bundle_id(id : string) {
    _release_spec.bundle_id = id
}

