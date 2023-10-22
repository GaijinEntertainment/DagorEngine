#include <de3_interface.h>
#include <de3_colorRangeService.h>
#include <generic/dag_tab.h>
#include <math/random/dag_random.h>
#include <debug/dag_debug.h>

class GenericColorRangeService : public IColorRangeService
{
public:
  struct Range
  {
    E3DCOLOR from, to;
  };
  Tab<Range> map;

  GenericColorRangeService() : map(midmem) {}

  virtual unsigned addColorRange(E3DCOLOR from, E3DCOLOR to)
  {
    if (from.u == 0xFFFFFFFFU && to.u == 0xFFFFFFFFU)
      return IDX_WHITE;
    if (from.u == 0x80808080U && to.u == 0x80808080U)
      return IDX_GRAY;

    for (int i = 0; i < map.size(); i++)
      if (map[i].from == from && map[i].to == to)
        return i;

    if (map.size() >= IDX_GRAY)
    {
      G_ASSERTF(map.size() < IDX_GRAY, "failed to add %08X,%08X, too many ranges created: %d", from, to, map.size());
      return IDX_GRAY;
    }

    int idx = append_items(map, 1);
    map[idx].from = from;
    map[idx].to = to;
    debug("add %08X,%08X as %d", from, to, idx);
    return idx;
  }

  virtual int getColorRangesNum() { return map.size(); }

  virtual E3DCOLOR getColorFrom(unsigned range_idx)
  {
    if (range_idx < map.size())
      return map[range_idx].from;
    if (range_idx == IDX_GRAY)
      return 0x80808080U;
    return 0xFFFFFFFFU;
  }
  virtual E3DCOLOR getColorTo(unsigned range_idx)
  {
    if (range_idx < map.size())
      return map[range_idx].to;
    if (range_idx == IDX_GRAY)
      return 0x80808080U;
    return 0xFFFFFFFFU;
  }

  virtual E3DCOLOR generateColor(unsigned range_idx, float seed_px, float seed_py, float seed_pz)
  {
    if (range_idx == IDX_GRAY)
      return 0xFF808080U;
    if (range_idx >= map.size())
      return 0xFFFFFFFFU;

    const Range &rng = map[range_idx];
    int seed = int(seed_px * 10) ^ (int(seed_py * 10) << 5) ^ (int(seed_pz * 10) << 10);
    float a, r, g, b;
    _rnd_fvec4(seed, a, r, g, b);

    a = (a * rng.from.a + (1.f - a) * rng.to.a) / 128.f;

    int ir = (int)((r * rng.from.r + (1.f - r) * rng.to.r) * a);
    if (ir > 255)
      ir = 255;

    int ig = (int)((g * rng.from.g + (1.f - g) * rng.to.g) * a);
    if (ig > 255)
      ig = 255;

    int ib = (int)((b * rng.from.b + (1.f - b) * rng.to.b) * a);
    if (ib > 255)
      ib = 255;

    // debug("%2d (%08X,%08X) p=(%.1f,%.1f,%.1f) -> %d,%d,%d", range_idx, rng.from, rng.to, seed_px, seed_py, seed_pz, ir, ig, ib);
    return E3DCOLOR_MAKE((unsigned char)ir, (unsigned char)ig, (unsigned char)ib, 255);
  }
};


static GenericColorRangeService srv;

void *get_generic_color_range_service() { return &srv; }
