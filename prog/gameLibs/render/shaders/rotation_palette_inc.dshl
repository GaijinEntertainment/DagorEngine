macro USE_ROTATION_PALETTE_ENCODING()
    hlsl(vs) {
      #include <rendInst/rotation_palette_consts.hlsli>

      // scale__palette_id: 8.5 scale (high bits) and 3bit palette (low bits)

      float unpack_scale(float scale__palette_id) {
        return floor(scale__palette_id*PALETTE_SCALE_MULTIPLIER)/PALETTE_SCALE_MULTIPLIER;
      }

      uint unpack_palette_id(float scale__palette_id) {
        return frac(scale__palette_id*PALETTE_SCALE_MULTIPLIER)*PALETTE_ID_MULTIPLIER;
      }
    }
endmacro