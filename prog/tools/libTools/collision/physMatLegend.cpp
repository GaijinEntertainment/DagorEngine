// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <libTools/collision/physMatLegend.h>
#include <gameRes/dag_collisionResource.h>
#include <scene/dag_physMat.h>
#include <math/dag_mathBase.h>
#include <osApiWrappers/dag_localConv.h>
#include <util/dag_hash.h>
#include <EASTL/sort.h>

static void add_faces(dag::Vector<NodeMaterialFaces> &list, int mat_id, uint32_t faces)
{
  for (NodeMaterialFaces &m : list)
    if (m.matId == mat_id)
    {
      m.faces += faces;
      return;
    }
  list.push_back({mat_id, faces});
}

static bool name_first(const NodeMaterialFaces &a, const NodeMaterialFaces &b)
{
  const int order = dd_stricmp(PhysMatLegend::nameOf(a.matId), PhysMatLegend::nameOf(b.matId));
  return order != 0 ? order < 0 : a.matId < b.matId;
}

enum
{
  HUE_AMBER,
  HUE_YELLOW,
  HUE_OLIVE,
  HUE_LIME,
  HUE_GREEN,
  HUE_TEAL,
  HUE_SKY,
  HUE_BLUE,
  HUE_INDIGO,
  HUE_PURPLE,
  HUE_ORCHID,
  HUE_PINK,
  HUE_COUNT
};
static constexpr int SHADE_COUNT = 4;
static constexpr int PALETTE_COUNT = HUE_COUNT * SHADE_COUNT;

// Not authored: one hue per material cluster, four lightness steps inside it, picked offline in Oklch and
// scored under CIEDE2000. Odd hues run a ladder shifted half a step, so two neighbouring hues never meet at one
// lightness: that is what keeps 48 colors apart. Two hues are at least dE2000 11 and two shades of one hue 9,
// both well over the dE2000 2 a viewer can just notice, and none comes near the red of the no-material faces.
static const E3DCOLOR MAT_PALETTE[][SHADE_COUNT] = {
  {E3DCOLOR(255, 203, 169, 255), E3DCOLOR(255, 146, 58, 255), E3DCOLOR(212, 109, 0, 255), E3DCOLOR(162, 82, 0, 255)},
  {E3DCOLOR(244, 178, 0, 255), E3DCOLOR(197, 143, 0, 255), E3DCOLOR(153, 110, 0, 255), E3DCOLOR(111, 79, 0, 255)},
  {E3DCOLOR(228, 224, 1, 255), E3DCOLOR(188, 184, 0, 255), E3DCOLOR(150, 147, 0, 255), E3DCOLOR(113, 111, 0, 255)},
  {E3DCOLOR(134, 217, 78, 255), E3DCOLOR(97, 178, 29, 255), E3DCOLOR(71, 138, 0, 255), E3DCOLOR(49, 100, 0, 255)},
  {E3DCOLOR(30, 253, 175, 255), E3DCOLOR(0, 210, 143, 255), E3DCOLOR(0, 167, 114, 255), E3DCOLOR(0, 127, 85, 255)},
  {E3DCOLOR(0, 219, 208, 255), E3DCOLOR(0, 177, 168, 255), E3DCOLOR(0, 137, 130, 255), E3DCOLOR(0, 99, 93, 255)},
  {E3DCOLOR(136, 232, 255, 255), E3DCOLOR(0, 198, 231, 255), E3DCOLOR(0, 158, 184, 255), E3DCOLOR(0, 120, 140, 255)},
  {E3DCOLOR(124, 199, 255, 255), E3DCOLOR(0, 163, 244, 255), E3DCOLOR(0, 126, 189, 255), E3DCOLOR(0, 90, 138, 255)},
  {E3DCOLOR(199, 215, 255, 255), E3DCOLOR(145, 174, 255, 255), E3DCOLOR(93, 130, 255, 255), E3DCOLOR(62, 93, 214, 255)},
  {E3DCOLOR(196, 176, 255, 255), E3DCOLOR(164, 125, 255, 255), E3DCOLOR(130, 86, 216, 255), E3DCOLOR(97, 48, 176, 255)},
  {E3DCOLOR(247, 194, 255, 255), E3DCOLOR(232, 132, 249, 255), E3DCOLOR(192, 94, 209, 255), E3DCOLOR(154, 56, 170, 255)},
  {E3DCOLOR(255, 153, 206, 255), E3DCOLOR(234, 95, 174, 255), E3DCOLOR(193, 56, 137, 255), E3DCOLOR(152, 0, 102, 255)},
};
G_STATIC_ASSERT(countof(MAT_PALETTE) == HUE_COUNT); // a hue named in the enum needs a row of its own
static const E3DCOLOR NO_MATERIAL_COLOR(255, 0, 0, 255);

// A cluster listed here owns its hue in every resource; add a line to name one more. The name is hashed, so a
// bf_class only matches it exactly, case included, the way physmat.blk spells it.
static constexpr struct
{
  uint32_t cluster;
  int hue;
} CLUSTER_HUE[] = {
  {"concrete"_h, HUE_PURPLE},
  {"wood"_h, HUE_YELLOW},
  {"steel"_h, HUE_BLUE},
  {"metal"_h, HUE_SKY},
};

// The hues no cluster owns, derived so the table above is the only one to edit.
static constexpr uint32_t owned_hues()
{
  uint32_t mask = 0;
  for (const auto &anchor : CLUSTER_HUE)
    mask |= 1u << anchor.hue;
  return mask;
}

static constexpr int free_hue_count()
{
  int count = 0;
  for (int hue = 0; hue < HUE_COUNT; ++hue)
    count += (owned_hues() & (1u << hue)) == 0;
  return count;
}

G_STATIC_ASSERT(free_hue_count() > 0); // a cluster the table does not name has to have a hue to hash into

// nth < free_hue_count(), so the walk always ends on one.
static int free_hue(uint32_t nth)
{
  for (int hue = 0;; ++hue)
    if ((owned_hues() & (1u << hue)) == 0 && nth-- == 0)
      return hue;
}

static bool has_palette_color(int mat_id) { return IsPhysMatID_Valid(mat_id) && mat_id != PHYSMAT_DEFAULT; }

// The cluster a material shares a hue with: its bf_class, or the head of its name when it declares none, which is
// all that groups ship_metal with ship_armor. It is the hash, not the name: two clusters that collide would share a
// hue, which the resource pass already handles for the clusters that merely hash to one hue.
static uint32_t cluster_of(int mat_id)
{
  const PhysMat::BFClassID bfid = PhysMat::getMaterial(mat_id).bfid;
  if (bfid > 0) // class 0 collects every material without a bf_class, so it groups nothing
  {
    const char *bfClass = PhysMat::getBFClassName(bfid);
    return mem_hash_fnv1(bfClass, strlen(bfClass));
  }
  const char *name = PhysMatLegend::nameOf(mat_id);
  const char *tail = strchr(name, '_');
  return mem_hash_fnv1(name, tail ? tail - name : strlen(name));
}

static int anchored_hue_of(uint32_t cluster)
{
  for (const auto &anchor : CLUSTER_HUE)
    if (anchor.cluster == cluster)
      return anchor.hue;
  return -1;
}

// The hue a cluster moves to when another one holds the hue it asked for: one no cluster owns, so the wood or
// concrete hue still names its own family, and an owned hue only when nothing else is left.
static int unclaimed_hue(uint32_t claimed, int preferred)
{
  for (int hue = 0; hue < HUE_COUNT; ++hue)
    if (!(claimed & (1u << hue)) && (owned_hues() & (1u << hue)) == 0)
      return hue;
  for (int hue = 0; hue < HUE_COUNT; ++hue)
    if (!(claimed & (1u << hue)))
      return hue;
  return preferred; // more clusters than hues: two of them share one, and the shade tells their materials apart
}

// Materials of one cluster take consecutive shades, so the cluster reads as one family. The start comes from what
// the hue left of the hash, or a one-material cluster would take the shade its hue already spent the hash on.
// It walks every phys material, so assignColors asks once per row.
static int shade_of(int mat_id, uint32_t cluster)
{
  const char *name = PhysMatLegend::nameOf(mat_id);
  uint32_t rank = 0;
  for (const PhysMat::MaterialData &m : PhysMat::getMaterials())
    if (dd_stricmp(m.name, name) < 0 && cluster_of(m.id) == cluster)
      ++rank;
  return (rank + cluster / free_hue_count()) % SHADE_COUNT;
}

void PhysMatLegend::build(const CollisionResource &res)
{
  clear();
  const int nodeCount = res.getAllNodes().size();
  perNodeMaterials.resize(nodeCount);
  for (int i = 0; i < nodeCount; ++i)
  {
    dag::Vector<NodeMaterialFaces> &list = perNodeMaterials[i];
    if (res.getNodeFaceCount(i) > 0)
      res.iterateNodeFacesWithLeafMaterial(i, [&](int, uint32_t, uint32_t, uint32_t, int mat) { add_faces(list, mat, 1); });
    else
      add_faces(list, res.getNodePhysMatId(i, 0), 0);
    eastl::sort(list.begin(), list.end(), name_first);
    for (const NodeMaterialFaces &m : list)
      add_faces(materialRows, m.matId, m.faces);
  }
  eastl::sort(materialRows.begin(), materialRows.end(), name_first);
  // One slot per id the material rows name (PHYSMAT_INVALID sits at 0); isVisible reads any other id as checked.
  int maxId = PHYSMAT_INVALID;
  for (const NodeMaterialFaces &m : materialRows)
    maxId = max(maxId, m.matId);
  matVisible.resize(maxId + 2);
  matVisible.set();
  assignColors();
}

static uint64_t entry_bit(int entry) { return uint64_t(1) << entry; }

static int first_free_entry(uint64_t busy, uint32_t skip_hues)
{
  for (int entry = 0; entry < PALETTE_COUNT; ++entry)
    if (!(busy & entry_bit(entry)) && !(skip_hues & (1u << (entry / SHADE_COUNT))))
      return entry;
  return -1;
}

// The entry a material moves to when its own is taken: another shade of its hue first, to keep the family, then a
// hue no other cluster of the resource holds, so two shades of a hue never mean two clusters; then one CLUSTER_HUE
// owns, which costs only the wood or concrete reading; a hue another cluster holds last. It never takes an entry
// another material holds or wants, or that material would lose a color it had no clash over. More materials than
// the palette holds leaves nothing free, and the last of them share a color.
static int free_palette_entry(int hue, uint64_t busy, uint32_t other_cluster_hues, int taken_entry)
{
  for (int shade = 0; shade < SHADE_COUNT; ++shade)
    if (!(busy & entry_bit(hue * SHADE_COUNT + shade)))
      return hue * SHADE_COUNT + shade;
  int entry = first_free_entry(busy, other_cluster_hues | owned_hues());
  if (entry < 0)
    entry = first_free_entry(busy, other_cluster_hues);
  if (entry < 0)
    entry = first_free_entry(busy, 0);
  return entry < 0 ? taken_entry : entry;
}

// One hue names one cluster of the resource, so two shades of a hue always mean two materials of one family. Two
// clusters that ask for one hue are the common case, not two that ask for one entry, and a shade apart is the
// closest the palette gets: without this, glass and plastic came out one step of green apart.
void PhysMatLegend::assignColors()
{
  matColor.assign(matVisible.size(), NO_MATERIAL_COLOR); // a row the palette holds no color for keeps this

  struct RowColor
  {
    uint32_t cluster = 0;
    int hue = -1; // stays -1 where the palette holds no color for the row
    int shade = 0;
  };
  dag::Vector<RowColor> rows(materialRows.size());
  for (int row = 0; row < materialRows.size(); ++row)
  {
    const int matId = materialRows[row].matId;
    if (!has_palette_color(matId))
      continue;
    RowColor &r = rows[row];
    r.cluster = cluster_of(matId);
    r.shade = shade_of(matId, r.cluster);
  }

  // An anchored cluster claims before the hashed ones, or a hashed cluster earlier in name order takes its hue.
  uint32_t claimedHues = 0;
  for (int anchoredPass = 1; anchoredPass >= 0; --anchoredPass)
    for (int row = 0; row < materialRows.size(); ++row)
    {
      RowColor &r = rows[row];
      if (!has_palette_color(materialRows[row].matId) || r.hue >= 0)
        continue;
      const int anchored = anchored_hue_of(r.cluster);
      if ((anchored >= 0) != (anchoredPass == 1))
        continue;
      int hue = -1;
      for (const RowColor &other : rows)
        if (other.hue >= 0 && other.cluster == r.cluster)
          hue = other.hue;
      if (hue < 0)
      {
        hue = anchored >= 0 ? anchored : free_hue(r.cluster % free_hue_count());
        if (claimedHues & (1u << hue))
          hue = unclaimed_hue(claimedHues, hue);
      }
      r.hue = hue;
      claimedHues |= 1u << hue;
    }

  // A material the move sends to a free hue makes that hue its cluster's too, or the next cluster to overflow would
  // take the next shade of it and the two would read as one family.
  int hueOwnerRow[HUE_COUNT];
  for (int &owner : hueOwnerRow)
    owner = -1;
  for (int row = 0; row < materialRows.size(); ++row)
    if (rows[row].hue >= 0)
      hueOwnerRow[rows[row].hue] = row;

  uint64_t wantedEntries = 0, takenEntries = 0;
  G_STATIC_ASSERT(PALETTE_COUNT <= sizeof(takenEntries) * 8);
  for (const RowColor &r : rows)
    if (r.hue >= 0)
      wantedEntries |= entry_bit(r.hue * SHADE_COUNT + r.shade);
  for (int row = 0; row < materialRows.size(); ++row)
  {
    const RowColor &r = rows[row];
    if (r.hue < 0)
      continue;
    int entry = r.hue * SHADE_COUNT + r.shade;
    if (takenEntries & entry_bit(entry)) // two materials of one cluster, so their ranks are four apart
    {
      uint32_t otherClusterHues = 0;
      for (int hue = 0; hue < HUE_COUNT; ++hue)
        if (hueOwnerRow[hue] >= 0 && rows[hueOwnerRow[hue]].cluster != r.cluster)
          otherClusterHues |= 1u << hue;
      entry = free_palette_entry(r.hue, takenEntries | wantedEntries, otherClusterHues, entry);
    }
    takenEntries |= entry_bit(entry);
    if (hueOwnerRow[entry / SHADE_COUNT] < 0)
      hueOwnerRow[entry / SHADE_COUNT] = row;
    matColor[materialRows[row].matId + 1] = MAT_PALETTE[entry / SHADE_COUNT][entry % SHADE_COUNT];
  }
}

void PhysMatLegend::clear()
{
  perNodeMaterials.clear();
  materialRows.clear();
  matColor.clear();
  matVisible.clear();
}

dag::ConstSpan<NodeMaterialFaces> PhysMatLegend::nodeMaterials(int node_id) const
{
  return (unsigned)node_id < perNodeMaterials.size() ? make_span_const(perNodeMaterials[node_id])
                                                     : dag::ConstSpan<NodeMaterialFaces>();
}

bool PhysMatLegend::isVisible(int mat_id) const
{
  const unsigned at = (unsigned)(mat_id + 1);
  return at >= matVisible.size() || matVisible.get(at);
}

void PhysMatLegend::setRowVisible(int row, bool v) { matVisible.set(materialRows[row].matId + 1, v); }

void PhysMatLegend::setAllVisible(bool v) { v ? matVisible.set() : matVisible.reset(); }

bool PhysMatLegend::hasVisibleGeometry(int node_id) const
{
  for (const NodeMaterialFaces &m : nodeMaterials(node_id))
    if (isVisible(m.matId))
      return true;
  return false;
}

E3DCOLOR PhysMatLegend::colorOf(int mat_id) const
{
  const unsigned at = (unsigned)(mat_id + 1);
  return at < matColor.size() ? matColor[at] : NO_MATERIAL_COLOR;
}

const char *PhysMatLegend::nameOf(int mat_id) { return IsPhysMatID_Valid(mat_id) ? PhysMat::getMaterial(mat_id).name.str() : "none"; }
