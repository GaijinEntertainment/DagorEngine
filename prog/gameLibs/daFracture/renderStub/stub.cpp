// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <daFracture/render/renderList.h>
#include <daFracture/render/renderContext.h>


namespace frx
{

void init_render() { G_ASSERT(0); }
void shutdown_render() { G_ASSERT(0); }

DestrContextRenderMaterialsHolder::DestrContextRenderMaterialsHolder() = default;
DestrContextRenderMaterialsHolder::~DestrContextRenderMaterialsHolder() { G_ASSERT(renderMats.empty()); };

RenderMesh::~RenderMesh() { G_ASSERT(0); }

uint16_t add_render_material(DestrContext &, ShaderMaterial *, RenderMatElem &&, bool)
{
  G_ASSERT(0);
  return 0;
}
void add_destr_geometry(DestrContext &, dag::Span<ShaderGeomLoadRequest>) { G_ASSERT(0); }
void upload_mesh_batch(const DestrContext &, dag::Span<MeshUploadRequest>) { G_ASSERT(0); }

void MeshRenderList::add(const RenderMesh &, const TMatrix &, const TMatrix &) { G_ASSERT(0); }
void MeshRenderList::prepare() { G_ASSERT(0); }
void MeshRenderList::finalizeForRi() { G_ASSERT(0); }
void MeshRenderList::clear() {}

ShaderMatChannels ShaderMatChannels::create(ShaderMaterial *)
{
  G_ASSERT(0);
  return {};
}

} // namespace frx
