Paddleboat is the Android Game Controller library of the Android Game Development Kit.

Upstream: https://android.googlesource.com/platform/frameworks/opt/gamesdk (Apache License 2.0, see LICENSE)
Vendored version: games-controller 2.0.2, gamesdk commit b832f08a5a2c2785f393ae99ad9af81922c2a337

Layout:
- src/main/cpp, src/main/java: games-controller/src/main of that commit, without the CMake and gradle files
- include/common/gamesdk_common.h: include/common of the same commit (paddleboat.h includes it)

The jamfile builds the paddleboat_static sources of games-controller/src/main/cpp/CMakeLists.txt.
The Java part goes to the APK through AndroidSources; and.proguard keeps it because the native code looks up
its classes and methods by name.

No local modifications.
