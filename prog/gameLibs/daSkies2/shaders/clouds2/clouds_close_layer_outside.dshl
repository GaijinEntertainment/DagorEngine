buffer clouds_close_layer_is_outside;

macro CLOSE_LAYER_EARLY_EXIT(code)
  ENABLE_ASSERT(code)
  if (clouds_close_layer_is_outside != NULL) {
    (code) {clouds_close_layer_is_outside@buf = clouds_close_layer_is_outside hlsl {StructuredBuffer<uint> clouds_close_layer_is_outside@buf;};}
    hlsl(code) {
      #define CLOUDS_HAS_EARLY_EXIT 1
      bool close_layer_should_early_exit()
      {
        return structuredBufferAt(clouds_close_layer_is_outside, 0) != 0;
      }
      bool clouds_non_empty_tile_count_lt(float count)
      {
        return float(structuredBufferAt(clouds_close_layer_is_outside, 1)) < count;
      }
      bool clouds_non_empty_tile_count_ge(float count)
      {
        return float(structuredBufferAt(clouds_close_layer_is_outside, 1)) >= count;
      }
    }
  } else
  {
    CLOSE_LAYER_EARLY_EXIT_STUB(code)
  }
endmacro
macro CLOSE_LAYER_EARLY_EXIT_STUB(code)
  hlsl(code) {
    #if !CLOUDS_HAS_EARLY_EXIT
      #define CLOUDS_HAS_EARLY_EXIT 1
      bool close_layer_should_early_exit(){ return false; }
      bool clouds_non_empty_tile_count_ge(float count){return false;}
      bool clouds_non_empty_tile_count_lt(float count){return true;}
    #endif
  }
endmacro

