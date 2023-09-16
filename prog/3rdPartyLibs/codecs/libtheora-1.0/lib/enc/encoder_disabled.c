/********************************************************************
 *                                                                  *
 * THIS FILE IS PART OF THE OggTheora SOFTWARE CODEC SOURCE CODE.   *
 * USE, DISTRIBUTION AND REPRODUCTION OF THIS LIBRARY SOURCE IS     *
 * GOVERNED BY A BSD-STYLE SOURCE LICENSE INCLUDED WITH THIS SOURCE *
 * IN 'COPYING'. PLEASE READ THESE TERMS BEFORE DISTRIBUTING.       *
 *                                                                  *
 * THE Theora SOURCE CODE IS COPYRIGHT (C) 2002-2007                *
 * by the Xiph.Org Foundation http://www.xiph.org/                  *
 *                                                                  *
 ********************************************************************

  function:
    encoder_disabled.c 13884 2007-09-22 08:38:10Z giles

 ********************************************************************/

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "toplevel_lookup.h"
#include "codec_internal.h"

int theora_encode_init(theora_state *th, theora_info *c){
  return OC_DISABLED;
}

int theora_encode_YUVin(theora_state *t, yuv_buffer *yuv){
  return OC_DISABLED;
}

int theora_encode_packetout( theora_state *t, int last_p, ogg_packet *op){
  return OC_DISABLED;
}

int theora_encode_header(theora_state *t, ogg_packet *op){
  return OC_DISABLED;
}

int theora_encode_comment(theora_comment *tc, ogg_packet *op){
  return OC_DISABLED;
}

int theora_encode_tables(theora_state *t, ogg_packet *op){
  return OC_DISABLED;
}

void theora_encoder_clear (CP_INSTANCE * cpi)
{
}
