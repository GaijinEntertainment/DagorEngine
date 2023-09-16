#include <image/dag_avif.h>
#include <image/dag_texPixel.h>
#include <ioSys/dag_fileIo.h>
#include <avif/avif.h>
#include <debug/dag_log.h>

TexImage32 *load_avif32(const char *fn, IMemAlloc *mem, bool *out_used_alpha)
{
  FullFileLoadCB crd(fn);
  if (crd.fileHandle)
    return load_avif32(crd, mem, out_used_alpha);
  return NULL;
}

struct AvifGenLoadReader
{
  avifIO io; // this must be the first member for easy casting to avifIO*
  avifRWData buffer;
  uint64_t curOffset;
  IGenLoad *crd;
};

static avifResult avif_read(struct avifIO *io, uint32_t readFlags, uint64_t offset, size_t size, avifROData *out)
{
  if (readFlags != 0) // Unsupported readFlags
    return AVIF_RESULT_IO_ERROR;

  AvifGenLoadReader *reader = (AvifGenLoadReader *)io;

  // Sanitize/clamp incoming request
  if (offset > reader->io.sizeHint)
    return AVIF_RESULT_IO_ERROR; // The offset is past the EOF.
  uint64_t availableSize = reader->io.sizeHint - offset;
  if (size > availableSize)
    size = (size_t)availableSize;

  if (size > 0)
  {
    if (reader->buffer.size < size)
      avifRWDataRealloc(&reader->buffer, size);

    if (reader->curOffset != offset)
    {
      int64_t seekRel = int64_t(offset) - int64_t(reader->curOffset);
      if (int(seekRel) != seekRel)
        return AVIF_RESULT_IO_ERROR;
      reader->crd->seekrel(int(seekRel));
      reader->curOffset = offset;
    }

    auto bytesRead = reader->crd->tryRead(reader->buffer.data, size); // todo: for 2Gb+ reads, implement loop
    if (bytesRead != size)
      return AVIF_RESULT_IO_ERROR;
    else
      reader->curOffset += size;
  }

  out->data = reader->buffer.data;
  out->size = size;
  return AVIF_RESULT_OK;
}

static void avif_destroy(struct avifIO *io)
{
  AvifGenLoadReader *reader = (AvifGenLoadReader *)io;
  avifRWDataFree(&reader->buffer);
}

TexImage32 *load_avif32(IGenLoad &crd, IMemAlloc *mem, bool *out_used_alpha)
{
  TexImage32 *ret = nullptr;
  avifRGBImage rgb;
  memset(&rgb, 0, sizeof(rgb));

  avifDecoder *decoder = avifDecoderCreate();
  AvifGenLoadReader avifIO;
  memset(&avifIO, 0, sizeof(avifIO));
  avifIO.crd = &crd;
  avifIO.io.read = &avif_read;
  avifIO.io.destroy = &avif_destroy;
  avifIO.io.sizeHint = (uint64_t)crd.getTargetDataSize();
  avifIO.io.persistent = AVIF_FALSE;
  avifRWDataRealloc(&avifIO.buffer, 1024);
  // Override decoder defaults here (codecChoice, requestedSource, ignoreExif, ignoreXMP, etc)

  avifDecoderSetIO(decoder, &avifIO.io);

  auto result = avifDecoderParse(decoder);
  if (result != AVIF_RESULT_OK)
  {
    logerr("Failed to decode image: %s", avifResultToString(result));
    goto cleanup;
  }
  result = avifDecoderNextImage(decoder);
  if (result == AVIF_RESULT_OK) // only first frame of animation!
  {
    // Now available (for this frame):
    // * All decoder->image YUV pixel data (yuvFormat, yuvPlanes, yuvRange, yuvChromaSamplePosition, yuvRowBytes)
    // * decoder->image alpha data (alphaPlane, alphaRowBytes)
    // * this frame's sequence timing

    avifRGBImageSetDefaults(&rgb, decoder->image);
    if (rgb.depth > 8)
    {
      logerr("16 bit images are currently not supported");
      goto cleanup;
    }
    if (out_used_alpha)
      *out_used_alpha = decoder->image->imageOwnsAlphaPlane;
    rgb.format = AVIF_RGB_FORMAT_BGRA;
    rgb.avoidLibYUV = AVIF_TRUE; // build without libuv (right now, slower!)
    // Override YUV(A)->RGB(A) defaults here:
    //   depth, format, chromaUpsampling, avoidLibYUV, ignoreAlpha, alphaPremultiplied, etc.

    // Alternative: set rgb.pixels and rgb.rowBytes yourself, which should match your chosen rgb.format
    // Be sure to use uint16_t* instead of uint8_t* for rgb.pixels/rgb.rowBytes if (rgb.depth > 8)
    TexImage32 *im = TexImage32::create(rgb.width, rgb.height, mem);
    rgb.pixels = (uint8_t *)im->getPixels();
    rgb.rowBytes = rgb.width * sizeof(TexPixel32);

    if (avifImageYUVToRGB(decoder->image, &rgb) != AVIF_RESULT_OK)
    {
      logerr("Conversion from YUV failed");
      mem->free(im);
    }
    else
      ret = im;
  }
  else
  {
    logerr("Failed to load image: %s", avifResultToString(result));
  }


cleanup:
  avifDecoderDestroy(decoder);
  return ret;
}
