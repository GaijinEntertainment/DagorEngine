# Significant changes per release

## 2.1.0

*  Add support for bind pose stripping
*  Add support for loop handling through looping policy
*  Add support for pre-processing
*  Add automatic compression level selection
*  Optimize compression through dominant shell computation
*  Add support for per sub-track rounding
*  Use error metric to detect constant/default sub-tracks
*  Tons of cleanup and minor improvements
*  Run regression tests with CI
*  Update to RTM 2.2.0
*  Update to sjson-cpp 0.9.0
*  Add support for GCC 12 and 13
*  Add support for clang 15
*  Add support for XCode 14
*  Add support for C++14, C++17, and C++20
*  Add support for MSYS2

## 2.0.6

*  Force macro expansion in version namespace identifier
*  Update to RTM v2.1.5
*  Update sjson-cpp to v0.8.3

## 2.0.5

*  Add support for clang 12, 13, and 14
*  Add support for GCC 11
*  Add support for XCode 12 and 13
*  Add support for Arm64 development on OS X and Linux
*  Misc CI improvements
*  Update to RTM v2.1.4
*  Update to Catch2 v2.13.7

## 2.0.4

*  Disable versioned namespace by default to avoid breaking ABI in patch release

## 2.0.3

*  Update sjson-cpp to v0.8.2
*  Update rtm to v2.1.3
*  Add versioned namespace to allow multiple versions to coexist within a binary
*  Fix database sampling interpolation when using a rounding mode other than `none`
*  Other minor fixes

## 2.0.2

*  Fix potential heap corruption when stripping a database that isn't split
*  Fix database tier bulk data size to include its padding

## 2.0.1

*  Fix incorrect seek offset when seeking past 0.0 in a single frame clip

## 2.0.0

*  Unified and cleaned up APIs
*  Cleaned up naming convention to match C++ stdlib, boost
*  Introduced streaming database support
*  Decompression profiling now uses Google Benchmark
*  Decompression has been heavily optimized
*  Compression has been heavily optimized
*  First release to support backwards compatibility going forward
*  Migrated all math to Realtime Math
*  Clips now support 4 billion samples/tracks
*  WebAssembly support added through emscripten
*  Many other improvements

## 1.3.5

*  Gracefully fail compression when we have more than 50000 samples
*  Update Catch2 to 2.13.1 to work with latest CMake

## 1.3.4

*  Avoid assert when using an additive base with a static pose/single frame

## 1.3.3

*  Fix single track decompression when scale is present with more than one segment
*  Gracefully fail compression when we have more than 65535 samples

## 1.3.2

*  Fix crash when compressing with an empty track array
*  Strip unused code when stat logging isn't required
*  Fix CompressedClip hash to be deterministic

## 1.3.1

*  Fix bug with scalar track decompression where garbage could be returned
*  Fix scalar track quantization to properly check the resulting error
*  Fix scalar track creation and properly copy the sample size
*  Other minor fixes and improvements

## 1.3.0

*  Added support for VS2019, GCC 9, clang7, and Xcode 11
*  Updated sjson-cpp and added a dependency on Realtime Math (RTM)
*  Optimized compression and decompression significantly
*  Added support for multiple root bones
*  Added support for scalar track compression
*  Many bug fixes and improvements

## 1.2.1

*  Silence SSE floating point exceptions during compression
*  Minor fixes

## 1.2.0

*  Added support for GCC 8, clang 6, Xcode 10, and Windows ARM64
*  Updated catch2 and sjson-cpp
*  Integrated SonarCloud
*  Added a compression level setting and changed default to *Medium*
*  Various bug fixes, minor optimizations, and cleanup

## 1.1.0

*  Added proper ARM NEON support
*  Properly detect SSE2 with MSVC if AVX isn't used
*  Lots of decompression performance optimizations
*  Minor fixes and cleanup

## 1.0.0

*  Minor additions to fully support UE4
*  Minor cleanup

## 0.8.0

*  Improved error handling
*  Added additive clip support
*  Added `acl_decompressor` tool to profile and test decompression
*  Increased warning levels to highest possible
*  Many more improvements and fixes

## 0.7.0

*  Added full support for Android and iOS
*  Added support for GCC6 and GCC7 on Linux
*  Downgraded C++ version to from 14 to 11
*  Added regression tests
*  Added lots of unit tests for core and math headers
*  Many more improvements and fixes

## 0.6.0

*  Hooked up continuous build integration
*  Added support for VS2017 on Windows
*  Added support for GCC5, Clang4, and Clang5 on Linux
*  Added support for Xcode 8.3 and Xcode 9.2 on OS X
*  Added support for x86 on Windows, Linux, and OS X
*  Better handle scale with built in error metrics
*  Many more improvements and fixes

## 0.5.0

*  Added support for 3D scale
*  Added partial support for Android (works in Unreal 4.15)
*  A fix to the variable bit rate optimization algorithm
*  Added a CLA system
*  Refactoring to support multiple algorithms better
*  More changes and additions to stat logging
*  Many more improvements and fixes

## 0.4.0

*  Lots of math performance, accuracy, and consistency improvements
*  Implemented segmenting support in uniformly sampled algorithm
*  Range reduction per segment support added
*  Minor fixes to fbx2acl
*  Optimized variable quantization considerably
*  Major changes to which stats are dumped and how they are processed
*  Many more improvements and fixes

## 0.3.0

*  Added CMake support
*  Improved error measuring and accuracy
*  Improved variable quantization
*  Convert most math to float32 to improve accuracy and performance
*  Many more improvements and fixes

## 0.2.0

*  Added clip_writer to create ACL files from a game integration
*  Added some unit tests and moved them into their own project
*  Added basic per track variable quantization
*  Added CMake support
*  Lots of cleanup, minor changes, and fixes

## 0.1.0

Initial release!

*  Uniformly sampled algorithm
*  Various rotation and vector formats
*  Clip range reduction
*  ACL SJSON file format
*  Custom allocator interface
*  Assert overrides
*  Custom math types and functions
*  Various tools to test the library
*  Visual Studio 2015 supported, x86 and x64
