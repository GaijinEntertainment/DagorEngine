// C++ source-search scope for the daslang MCP server.
//
// These constants control which folders the cpp_* MCP tools (cpp_grep_usage,
// cpp_find_symbol, cpp_outline, cpp_goto_definition) and the with_cpp_source
// redirect on find_symbol / goto_definition scan for symbols.
//
// The MCP server reads these at index-build time. To pick up changes:
//   - Edit this file and restart the MCP server (`mcp__daslang__shutdown`),
//     OR
//   - Touch any tracked .cpp/.h/.hpp under CPP_SEARCH_DIRS — the staleness
//     signature includes this file's mtime, so the next lookup rebuilds.
//
// All paths are relative to the daslang repository root.
//
// Documentation: skills/mcp_tools.md (or install/skills/mcp_tools.md for SDK
// consumers) and doc/source/reference/utils/mcp.rst.

options gen2

// Folders to scan, recursively. `sg scan` walks each tree, so listing
// `modules` covers all `modules/*/...` subdirectories automatically.
let CPP_SEARCH_DIRS <- [
    "src",      // engine internals (.cpp, .h)
    "include",  // public headers (.h, .hpp)
    "modules"   // bundled native modules (recursive)
]

// Always-applied excludes — generated/vendored content under the search
// roots that should never be indexed.
let CPP_SEARCH_ALWAYS_EXCLUDE <- [
    "!**/build*/**",
    "!**/cmake-build-*/**",   // CLion default build dir
    "!**/CMakeFiles/**",      // cmake per-target dirs
    "!**/_deps/**",           // FetchContent / CPM destination
    "!**/3rdparty/**",
    "!**/.git/**"
]

// File extensions to scan. The daslang repo itself only uses .cpp/.h/.hpp,
// but `.cc` is included so consumers who embed the MCP server in a codebase
// that uses Google/Chromium-style `.cc` get coverage out of the box. Other
// extensions (.c/.cxx/.hxx/.inl/.ipp) don't appear in either tree audited.
let CPP_SEARCH_INCLUDE_GLOBS <- [
    "*.cpp",
    "*.cc",
    "*.h",
    "*.hpp"
]

// Auto-exclude policy: any folder containing a `.git` file or directory is
// treated as a separate repository (submodule, daspkg-installed package,
// FetchContent destination, ad-hoc clone) and excluded by default.
//
// Example: `modules/.daspkg_cache/index/.git` (daspkg's package index clone)
// is auto-excluded today.
//
// To force-include a folder despite it containing `.git`, list its
// repo-relative path here. Example:
//   let CPP_SEARCH_INCLUDE_OVERRIDES <- ["modules/myExternalCppLib"]
let CPP_SEARCH_INCLUDE_OVERRIDES : array<string>

// Result-count caps for the cpp_* MCP tools. Raise these for codebases where
// a common identifier (e.g. `Logger`, `Result`) legitimately has hundreds of
// declarations and the default cutoff hides the one you want. Lower them to
// keep tool output compact.
let CPP_MAX_FIND_RESULTS = 50      // cpp_find_symbol — declaration matches per query
let CPP_MAX_GOTO_CANDIDATES = 5    // cpp_goto_definition — ranked candidates shown
