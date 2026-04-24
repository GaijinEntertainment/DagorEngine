// Foreign Function Binding -- Calling Native DLL Functions
//
// The `[extern]` annotation lets you call C functions from shared
// libraries (DLLs on Windows, .so on Linux, .dylib on macOS)
// directly from daslang, without writing any C++ glue code.
//
// Annotation syntax:
//   [extern(convention, name="CName", library="LibName")]
//   def DasName(args) : ReturnType { return default_value; }
//
// Parameters:
//   convention -- calling convention: stdcall (aka WINAPI), cdecl, opengl
//   name       -- the exported symbol name in the library
//   library    -- DLL / shared library to load
//
// The function body is a stub (never executed) -- the annotation
// replaces it with a direct call to the native function at link time.
//
// Platform-specific libraries can be specified:
//   [extern(cdecl, name="foo",
//           windows_library="foo.dll",
//           macos_library="libfoo.dylib",
//           linux_library="libfoo.so")]
//
// Add `late` to defer symbol resolution until first call:
//   [extern(cdecl, late, name="foo", library="plugin.dll")]
//
// Limitations:
//   - Vector types (float2/float3/float4) are not supported as arguments
//   - Maximum ~13 arguments per function (platform-dependent)
//   - Requires `require dasbind` module
//
// This example calls standard C library functions available on all platforms.
// Run: daslang examples/dasbind/dasbind_example.das

options gen2
require dasbind

// --- C library bindings ---

// Try `non-late`:

// Returns the absolute value of an integer.
[extern(cdecl, name="abs", windows_library="msvcrt", linux_library="libc.so.6", macos_library="libSystem.B.dylib")]
def c_abs(x : int) : int { return 0; }

// Returns the current time as seconds since epoch.
[extern(cdecl, name="time", windows_library="msvcrt", linux_library="libc.so.6", macos_library="libSystem.B.dylib")]
def c_time(timer : void?) : int64 { return int64(0); }

// Try `late`:

// Computes the square root of a double.
[extern(cdecl, late, name="sqrt", windows_library="msvcrt", linux_library="libm.so.6", macos_library="libSystem.B.dylib")]
def c_sqrt(x : double) : double { return 0.0lf; }

// Returns the length of a C string.
[extern(cdecl, late, name="strlen", windows_library="msvcrt", linux_library="libc.so.6", macos_library="libSystem.B.dylib")]
def c_strlen(s : string) : int64 { return int64(0); }

// --- Main ---

[export]
def main() {
    let val = c_abs(-42)
    let now = c_time(null)
    let sq = c_sqrt(2.0lf)
    let slen = c_strlen("hello dasbind")
    print("abs(-42)      : {val}\n")
    print("epoch seconds : {now}\n")
    print("sqrt(2.0)     : {sq}\n")
    print("strlen(...)   : {slen}\n")
    print("done\n")
}
