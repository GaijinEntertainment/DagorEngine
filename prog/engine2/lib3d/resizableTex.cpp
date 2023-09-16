
#include <3d/dag_resizableTex.h>
#include <math/dag_adjpow2.h>
#include <debug/dag_debug.h>
#include <EASTL/string.h>
#include <EASTL/algorithm.h>

namespace resptr_detail
{

static key_t make_key(uint32_t width, uint32_t height) { return (width << 16) | height; }

void ResizableManagedTex::resize(int width, int height)
{
  G_ASSERT(this->mResource);
  if (!this->mResource)
    return;

  TextureInfo tex_info;
  this->mResource->getinfo(tex_info);
  key_t my_key = make_key(tex_info.w, tex_info.h);
  key_t new_key = make_key(width, height);

  if (my_key == new_key)
    return;

  const char *texture_name = this->mResource->getTexName();
  eastl::string managed_name({}, "%s-%ix%i", texture_name, width, height);

  UniqueTex this_tex;
  this->ManagedTex::swap(this_tex);

  bool inserted = mAliases.emplace(my_key, eastl::move(this_tex)).second;
  G_ASSERT(inserted);
  G_UNUSED(inserted);

  const UniqueTex &bigest_texture = mAliases.rbegin()->second;
  const int max_mips = eastl::min(get_log2w(width), get_log2w(height)) + 1;
  int mip_levels = min(bigest_texture->level_count(), max_mips);

  auto found = mAliases.lower_bound(new_key);
  if (found == mAliases.end())
  {
    debug("Resizing %s to larger size than it has. This texture will be recreated", texture_name);
    mAliases.clear();
    TexPtr tex = dag::create_tex(nullptr, width, height, tex_info.cflg, mip_levels, texture_name);
    this_tex = UniqueTex(eastl::move(tex), managed_name.c_str());
  }
  else if (found->first == new_key)
  {
    this_tex = eastl::move(found->second);
    mAliases.erase(found->first);
  }
  else
  {
    TexPtr tex = dag::alias_tex(found->second.getTex2D(), nullptr, width, height, tex_info.cflg, mip_levels, texture_name);
    if (tex)
    {
      d3d::resource_barrier({{found->second.getTex2D(), tex.get()}, {RB_ALIAS_FROM, RB_ALIAS_TO_AND_DISCARD}, {0, 0}, {0, 0}});
      this_tex = UniqueTex(eastl::move(tex), managed_name.c_str());
    }
    else
    {
      logerr("d3d::alias_tex() not supported");
      this_tex = eastl::move(found->second);
      mAliases.erase(found->first);
    }
  }

  this->ManagedTex::swap(this_tex);
  return;
}

} // namespace resptr_detail