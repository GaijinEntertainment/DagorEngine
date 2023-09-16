#include <gameRes/dag_collisionResource.h>
#include <render/lruCollision.h>
#pragma once
struct LRUCollision
{
  dag::Vector<eastl::unique_ptr<CollisionResource>> collRes;
  dag::Vector<dag::Vector<mat43f>> instances;
  size_t addCollRes(const dag::Vector<Point3_vec4> &vertices, const dag::Vector<uint16_t> &indices)
  {
    eastl::unique_ptr<CollisionResource> coll(new CollisionResource());
    coll->numMeshNodes = 1;
    auto &n = coll->createNode();
    coll->meshNodesHead = &n;
    n.flags = n.IDENT;
    n.tm = TMatrix::IDENT;
    n.resetVertices(make_span(memalloc_typed<Point3_vec4>(vertices.size(), midmem), vertices.size()));
    mem_copy_from(n.vertices, vertices.data());
    n.resetIndices(make_span(memalloc_typed<uint16_t>(indices.size(), midmem), indices.size()));
    mem_copy_from(n.indices, indices.data());
    v_bbox3_init_empty(coll->vFullBBox);
    for (auto &v : vertices)
      v_bbox3_add_pt(coll->vFullBBox, v_ld(&v.x));
    collRes.push_back(eastl::move(coll));
    return collRes.size() - 1;
  }
  void load(IGenLoad &cb)
  {
    cb.readInt();
    struct Mesh
    {
      dag::Vector<uint16_t> indices;
      dag::Vector<Point3> vertices;
    };
    Mesh mesh;
    dag::Vector<Point3_vec4> vertices;
    for (int i = 0; cb.tell() < cb.getTargetDataSize(); ++i)
    {
      mesh.indices.resize(cb.readInt());
      cb.read(mesh.indices.begin(), mesh.indices.size() * sizeof(*mesh.indices.data()));
      mesh.vertices.resize(cb.readInt());
      cb.read(mesh.vertices.begin(), mesh.vertices.size() * sizeof(*mesh.vertices.data()));
      vertices.resize(mesh.vertices.size());
      for (int j = 0; j < mesh.vertices.size(); ++j)
        vertices[j] = mesh.vertices[j];
      addCollRes(vertices, mesh.indices);

      int instCount = cb.readInt();
      instances.push_back().resize(instCount);
      cb.read(instances.back().data(), instances.back().size() * sizeof(mat43f));
    }
  }

  eastl::unique_ptr<ShaderMaterial> renderCollisionMat;
  ShaderElement *renderCollisionElem = 0;
  eastl::unique_ptr<LRURendinstCollision> lruColl;
  ~LRUCollision()
  {
    if (renderCollisionElem && d3d::get_driver_desc().caps.hasWellSupportedIndirect)
      d3d::delete_vdecl(renderCollisionElem->getEffectiveVDecl());
  }

  void init()
  {
    lruColl.reset(new LRURendinstCollision);
    renderCollisionMat.reset(new_shader_material_by_name_optional("render_collision"));
    if (renderCollisionMat)
    {
      renderCollisionMat->addRef();
      renderCollisionElem = renderCollisionMat->make_elem();
    }
    else
      logerr("no render_collision shader");
    const bool multiDrawIndirectSupported = d3d::get_driver_desc().caps.hasWellSupportedIndirect;
    if (multiDrawIndirectSupported && renderCollisionElem)
    {
      VSDTYPE vsdInstancing[] = {VSD_STREAM_PER_VERTEX_DATA(0), VSD_REG(VSDR_POS, VSDT_HALF4), VSD_STREAM_PER_INSTANCE_DATA(1),
        VSD_REG(VSDR_TEXC0, VSDT_INT1), VSD_END};
      renderCollisionElem->replaceVdecl(d3d::create_vdecl(vsdInstancing));
    }
  }
  void rasterize(dag::Span<uint64_t> handles)
  {
    if (!lruColl)
      init();
    if (!renderCollisionElem)
      return;
    DA_PROFILE_GPU;
    lruColl->updateLRU(handles);
    lruColl->draw(handles, nullptr, nullptr, 1, *renderCollisionElem);
  }
  void rasterize(const dag::Vector<dag::Vector<mat43f>> &instances)
  {
    mat44f viewproj;
    d3d::getglobtm(viewproj);
    dag::Vector<uint64_t, framemem_allocator> handles;
    for (size_t i = 0, e = instances.size(); i != e; ++i)
    {
      vec3f bmin, bmax;
      for (size_t j = 0, je = instances[i].size(); j != je; ++j)
      {
        mat44f clip;
        v_mat44_mul43(clip, viewproj, instances[i][j]);
        if (v_is_visible_b_fast(bmin, bmax, clip))
          handles.push_back((uint64_t(i) << 32UL) | j);
      }
    }
    rasterize(dag::Span<uint64_t>(handles.data(), handles.size()));
  }
};
