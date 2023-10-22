#include "resampleImage.h"
#include <image/imageresampler/resampler.h>
#include <dag/dag_vector.h>
#include <math/dag_mathBase.h>

TexImage32 *resample_img(TexImage32 *img, int pic_w, int pic_h, bool keep_ar, IMemAlloc *mem)
{
  if (!pic_w && !pic_h)
    return img;
  if (!img)
    return nullptr;

  if (img->w > RESAMPLER_MAX_DIMENSION || img->h > RESAMPLER_MAX_DIMENSION)
  {
    logwarn("cannot resample %dx%d: image is too large!", img->w, img->h);
    return img;
  }

  // calculate target image size
  float w = img->w, h = img->h;
  float src_aspect = w / h;
  if (pic_w > 0 && pic_h > 0)
  {
    w = pic_w;
    h = pic_h;
    if (keep_ar) // special case dimensions are set, but we request keep aspect ratio
    {
      float w_from_h = h * src_aspect;
      float h_from_w = w / src_aspect;
      if (w_from_h < w)
        w = w_from_h;
      else if (h_from_w < h)
        h = h_from_w;
    }
  }
  else if (pic_w > 0)
    pic_set_dim(w, h, pic_w, 1.0f / src_aspect, keep_ar);
  else if (pic_h > 0)
    pic_set_dim(h, w, pic_h, src_aspect, keep_ar);

  if (pic_w < 0)
    pic_adjust_dim(w, h, -pic_w, 1.0f / src_aspect, keep_ar);
  if (pic_h < 0)
    pic_adjust_dim(h, w, -pic_h, src_aspect, keep_ar);
  int wi = (int)max(1.f, floorf(w + 0.5f)), hi = (int)max(1.0f, floorf(h + 0.5f));
  if (wi == img->w && hi == img->h)
    return img;

  TexImage32 *img_r = TexImage32::create(wi, hi, mem);
  if (!img_r)
  {
    logwarn("cannot resample %dx%d: not enought mem for %dx%d", img->w, img->h, wi, hi);
    return img;
  }

  const float filter_scale = 1.0f;  // Filter scale - values < 1.0 cause aliasing, but create sharper looking mips.
  const char *pFilter = "lanczos4"; // RESAMPLER_DEFAULT_FILTER;

  static constexpr int CHANNELS = 4;
  Resampler *resamplers[CHANNELS];
  dag::Vector<float> samples[CHANNELS];

  // Now create a Resampler instance for each component to process. The first instance will create
  // new contributor tables, which are shared by the resamplers used for the other components
  // (a memory and slight cache efficiency optimization).
  for (int i = 0; i < CHANNELS; i++)
  {
    resamplers[i] = new Resampler(img->w, img->h, wi, hi, Resampler::BOUNDARY_CLAMP, 0.0f, 1.0f, pFilter,
      i == 0 ? nullptr : resamplers[0]->get_clist_x(), i == 0 ? nullptr : resamplers[0]->get_clist_y(), filter_scale, filter_scale);
    samples[i].resize(img->w);
  }

  auto *pSrc_image = (const unsigned char *)img->getPixels();
  auto *dst_image = (unsigned char *)img_r->getPixels();
  const int src_pitch = img->w * CHANNELS;
  const int dst_pitch = wi * CHANNELS;
  int dst_y = 0;

  for (int src_y = 0; src_y < img->h; src_y++)
  {
    const unsigned char *pSrc = &pSrc_image[src_y * src_pitch];

    for (int x = 0; x < img->w; x++)
      for (int c = 0; c < CHANNELS; c++)
        samples[c][x] = *pSrc++ * (1.0f / 255.0f);

    for (int c = 0; c < CHANNELS; c++)
      if (!resamplers[c]->put_line(&samples[c][0]))
      {
        logerr("cannot resample %dx%d -> %dx%d: out of memory!", img->w, img->h, wi, hi);
        for (auto *r : resamplers)
          delete r;
        memfree(img_r, mem);
        return img;
      }

    for (;;)
    {
      int comp_index;
      for (comp_index = 0; comp_index < CHANNELS; comp_index++)
      {
        const float *pOutput_samples = resamplers[comp_index]->get_line();
        if (!pOutput_samples)
          break;

        G_ASSERT(dst_y < hi);
        unsigned char *pDst = &dst_image[dst_y * dst_pitch + comp_index];

        for (int x = 0; x < wi; x++)
        {
          *pDst = (unsigned char)clamp((int)(255.0f * pOutput_samples[x] + .5f), 0, 255);
          pDst += CHANNELS;
        }
      }
      if (comp_index < CHANNELS)
        break;

      dst_y++;
    }
  }

  // Delete the resamplers.
  for (auto *r : resamplers)
    delete r;

  // free original image and return resampled one
  memfree(img, mem);
  return img_r;
}
