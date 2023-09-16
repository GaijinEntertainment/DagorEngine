include "land_block_inc.sh"
include "bc_compression_inc.sh"
include "bc6h_compression.sh"

texture src_tex;
float src_mip = 0;
float dst_mip = 0;

int src_face = -1;
interval src_face : src_single_face < 0, src_cube;


shader bc1_srgbwrite_compressor, bc1_compressor, bc3_compressor, bc3_srgbwrite_compressor, bc4_compressor, bc5_compressor, bc6h_compressor
{
  supports none;
  supports global_frame;
  supports land_mesh_prepare_clipmap;

  hlsl
  {
    #define BC_COMPRESSION_USE_MIPS
  }

  (vs) { src_dst_mip__src_face@f3 = (src_mip, dst_mip, src_face, 0); }

  if (src_face != src_single_face)
  {
    (ps) { src_tex@smpCube = src_tex; }
    hlsl {
      #define BC_COMPRESSION_FOR_CUBE 1
    }
  }
  else
  {
    (ps) { src_tex@smp2d = src_tex; }
  }

  INIT_BC_COMPRESSION()

  if ( shader == bc1_compressor || shader == bc1_srgbwrite_compressor)
  {
    USE_BC1_COMPRESSION(ps)
    // USE_R5G6B5_2BPP_COMPRESSION_DEBUG()
  }

  if ( shader == bc3_compressor || shader == bc3_srgbwrite_compressor )
  {
    USE_BC3_COMPRESSION(ps)
  }

  if ( shader == bc4_compressor )
  {
    USE_BC4_COMPRESSION(ps)
  }

  if ( shader == bc5_compressor )
  {
    USE_BC5_COMPRESSION(ps)
  }

  if ( shader == bc6h_compressor )
  {
    USE_BC6H_COMPRESSION(ps)
  }

  if ( shader == bc1_compressor || shader == bc4_compressor)
  {
    PS4_DEF_TARGET_FMT_32_GR()
  }
  else
  {
    PS4_DEF_TARGET_FMT_32_ABGR()
  }

  hlsl(ps)
  {
##if shader == bc3_srgbwrite_compressor || shader == bc1_srgbwrite_compressor
    #include <pixelPacking/ColorSpaceUtility.hlsl>
##endif

    uint4 bc_compressor_ps( VsOutput input ) : SV_Target
    {
      half4 texels[16];
      half4 min_color, max_color;

#ifdef BC_COMPRESSION_FOR_CUBE
      get_texels( input.tex, texels, input.src_mip, input.src_face );
#else
      get_texels( input.tex, texels, input.src_mip );
#endif

##if shader == bc3_srgbwrite_compressor || shader == bc1_srgbwrite_compressor
      for ( int i = 0; i < 16; ++i )
        texels[i].rgb = accurateLinearToSRGB( texels[i].rgb );
##endif

##if shader == bc6h_compressor
      //DXGI_FORMAT_BC6H_UF16 fmt is used, so limit negative values
      for ( int i = 0; i < 16; ++i )
        texels[i].rgb = max( 0., texels[i].rgb );
      return pack_bc6h_block(texels);
##endif
      find_base_colors( texels, min_color, max_color );

##if shader == bc1_compressor || shader == bc1_srgbwrite_compressor
      refine_rgb_base_colors( texels, min_color, max_color );
      return pack_bc1_block( texels, min_color, max_color );
      // return pack_r5g6b5_2bpp_psnr_block( texels, min_color, max_color );
##endif

##if shader == bc3_compressor || shader == bc3_srgbwrite_compressor
      refine_rgb_base_colors( texels, min_color, max_color );
      return pack_bc3_block( texels, min_color, max_color );
##endif

##if shader == bc4_compressor
      return pack_bc4_block( texels, min_color, max_color );
##endif

##if shader == bc5_compressor
      return pack_bc5_block( texels, min_color, max_color );
##endif
    }
  }

  compile( "target_vs", "bc_compressor_vs" );
  compile( "target_ps", "bc_compressor_ps" );
}
