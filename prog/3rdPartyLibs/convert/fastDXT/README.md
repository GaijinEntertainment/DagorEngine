# fastDXT

Fast CPU DXT1/DXT3/DXT5/BC4/BC5 texture compressor, based on J.M.P. van Waveren's
"Real-Time DXT Compression" (id Software). Includes rygDXT.cpp, which is
stb_dxt.h (a DXT1/DXT5 compressor by Fabian "ryg" Giesen, ported to C by Sean
Barrett), and goofy_tc.h, a realtime BC1/ETC1 encoder by Sergey Makeev.

Upstream: stb_dxt.h from https://github.com/nothings/stb (v1.04), public
domain / MIT dual license per LICENSE.
