#include <3d/dag_drv3d.h>
#include <3d/dag_drv3dReset.h>
#include <math/dag_Point4.h>
#include <vecmath/dag_vecMath_const.h>
#include <daECS/core/entityManager.h>
#include <daECS/core/componentTypes.h>
#include <EASTL/algorithm.h>
#include <EASTL/numeric.h>

#include "decalMatrices/decalsMatrices.h"

struct MatrixManagerRegistry
{
  eastl::vector<DecalsMatrices *> managers;
};

static eastl::unique_ptr<MatrixManagerRegistry> registeredMatrixManagers;

static void register_matrix_manager(DecalsMatrices *mgr)
{
  if (!registeredMatrixManagers)
    registeredMatrixManagers = eastl::make_unique<MatrixManagerRegistry>();
  registeredMatrixManagers->managers.push_back(mgr);
}

static void release_matrix_manager(DecalsMatrices *mgr)
{
  G_ASSERT(registeredMatrixManagers);
  auto itr = eastl::find(registeredMatrixManagers->managers.begin(), registeredMatrixManagers->managers.end(), mgr);
  G_ASSERT(itr != registeredMatrixManagers->managers.end());
  if (itr != registeredMatrixManagers->managers.end())
    registeredMatrixManagers->managers.erase(itr);
  if (registeredMatrixManagers->managers.size() == 0)
    registeredMatrixManagers.reset();
}


DecalsMatrices::DecalsMatrices(size_t max_matrix_count, const char *matrix_buffer_name, uint32_t max_item_count) :
  DecalsMatrices(max_matrix_count, matrix_buffer_name, {nullptr, 0}, max_item_count)
{}

DecalsMatrices::DecalsMatrices(size_t max_matrix_count, const char *matrix_buffer_name, ecs::HashedConstString entity_component,
  uint32_t max_item_count) :
  matrices(max_matrix_count),
  removedMatrices(max_matrix_count, false),
  matricesGen(max_matrix_count),
  matricesReferences(max_matrix_count),
  maxItemCount(max_item_count),
  entityComponent(entity_component),
  itemMap(max_matrix_count, max_item_count)
{
  matricesCB = dag::buffers::create_persistent_cb(dag::buffers::cb_array_reg_count<DecalMatrix>(matrices.size()), matrix_buffer_name);
  matricesCB.setVar();

  if (entityComponent.str != nullptr)
    register_matrix_manager(this);
}

void DecalsMatrices::setMaxItems(uint32_t max_items)
{
  maxItemCount = max_items;
  itemMap.resize(max_items);
}

DecalsMatrices::~DecalsMatrices()
{
  if (entityComponent.str != nullptr)
    release_matrix_manager(this);
}

void DecalsMatrices::beforeRender()
{
  if (matricesInvalid)
  {
    if (idCount > 0)
      matricesCB.getBuf()->updateDataWithLock(0, sizeof(matrices[0]) * idCount, matrices.data(), VBLOCK_WRITEONLY | VBLOCK_DISCARD);
    matricesInvalid = false;
  }
}

void DecalsMatrices::defrag()
{
  while (idCount > 0 && !matricesReferences[idCount - 1])
    idCount--;
}

void DecalsMatrices::deleteMatrixId(uint32_t id)
{
  if (id == INVALID_MATRIX_ID)
    return;
  id--;
  if (id >= idCount)
  {
    logerr("attempt to delete wrong MatrixId %d out of %d", id, idCount);
    return;
  }
  if (matricesGen[id] == just_added)
    matricesGen[id] = never_used;
  matricesReferences[id] = max((int)matricesReferences[id] - 1, (int)0);
  if (!matricesReferences[id] && !removedMatrices[id])
  {
    removedMatrixCount++;
    removedMatrices[id] = true;
    itemMap.removeMatrix(id);
  }
  defrag();
}

uint32_t DecalsMatrices::addNewMatrixId()
{
  defrag();
  uint32_t bestId = matrices.size();
  if (idCount == matrices.size())
  {
    uint64_t stillUsedGen = getStillUsedGeneration();
    uint64_t bestGen = eastl::numeric_limits<uint64_t>::max();
    for (uint32_t id = 0; id < idCount; ++id)
    {
      uint64_t cgen = (uint64_t(matricesReferences[id]) ? (1ULL << 32ULL) : 0ULL) + (uint64_t)matricesGen[id];
      if (bestGen > cgen) // we allow aliasing of matrices, if there is nothing else we can do! This is better than other visual bugs.
      {
        bestId = id;
        bestGen = cgen;
        if (bestGen < stillUsedGen) // it is good enough, there is no alive usage of that dead matrix!
          break;
      }
    }
  }
  else
  {
    bestId = idCount;
    matrices[idCount++] = DecalMatrix{V_C_UNIT_1000, V_C_UNIT_0100, V_C_UNIT_0010};
    matricesReferences[bestId] = 0;
    matricesGen[bestId] = just_added;
  }
  if (bestId == matrices.size())
  {
#if DAGOR_DBGLEVEL > 0
    logerr("%s limit of %d reached", __FUNCTION__, matrices.size());
#endif
    return INVALID_MATRIX_ID;
  }
  matricesReferences[bestId]++;
  if (removedMatrices[bestId])
  {
    removedMatrixCount--;
    removedMatrices[bestId] = false;
  }
  matricesInvalid = true;
  return bestId + 1;
}

void DecalsMatrices::setMatrix(uint32_t id, const DecalMatrix &mat)
{
  if (id == INVALID_MATRIX_ID)
    return;
  id--;
  if (id >= idCount || !matricesReferences[id])
  {
    G_ASSERTF(0, "matrix id=%d should be added with addNewMatrixId, currently there are %d added", id, idCount);
    return;
  }
  matrices[id] = mat;
  matricesInvalid = true;
}

const DecalsMatrices::DecalMatrix &DecalsMatrices::getMatrix(uint32_t id) const
{
  G_ASSERT(id != INVALID_MATRIX_ID);
  id--;
  G_ASSERTF(id < idCount, "matrix id=%d is not a valid id, number of matrices=%d", id, idCount);
  return matrices[id];
}

bool DecalsMatrices::isMatrixUsed(uint32_t id) const
{
  G_ASSERT(id != INVALID_MATRIX_ID);
  id--;
  if (id >= idCount)
    return false;
  return matricesReferences[id] > 0;
}

void DecalsMatrices::clearItems()
{
  clearDeletedMatrices();
  matricesInvalid = false;
  itemMap.clear();
}

void DecalsMatrices::clearAll()
{
  clearItems();
  idCount = 0;
  eastl::fill(matricesReferences.begin(), matricesReferences.end(), 0);
}

void DecalsMatrices::setMatrix(uint32_t matrix_id, const TMatrix &matrix)
{
  Point4 row0 = Point4::xyzV(matrix.getcol(0), matrix.getcol(3).x);
  Point4 row1 = Point4::xyzV(matrix.getcol(1), matrix.getcol(3).y);
  Point4 row2 = Point4::xyzV(matrix.getcol(2), matrix.getcol(3).z);

  DecalMatrix m43;
  m43.row0 = *(vec4f *)&row0;
  m43.row1 = *(vec4f *)&row1;
  m43.row2 = *(vec4f *)&row2;

  setMatrix(matrix_id, m43);
}

uint32_t DecalsMatrices::registerMatrix(ecs::EntityId eid, const TMatrix &tm)
{
  if (entityComponent.str == nullptr)
    return INVALID_MATRIX_ID;
  if (int *matrixId = g_entity_mgr->getNullableRW<int>(eid, entityComponent))
  {
    if (*matrixId == INVALID_MATRIX_ID)
      *matrixId = addNewMatrixId();
    setMatrix(*matrixId, tm);
    // These are never removed from the entity
    g_entity_mgr->set(eid, ECS_HASH("decals__useDecalMatrices"), true);
    return *matrixId;
  }
  return INVALID_MATRIX_ID;
}

uint32_t DecalsMatrices::getMatrixId(ecs::EntityId eid) const
{
  if (entityComponent.str == nullptr || !eid)
    return INVALID_MATRIX_ID;
  int ret = g_entity_mgr->getOr<int>(eid, entityComponent, INVALID_MATRIX_ID);
  G_ASSERT(ret >= 0);
  return ret;
}

// This function check if the entity supports decal matrices. Usually it means to have decaled_rendinst template.
// If the entity supports, return transform matrix of entity.
// If the entity doesn't support or have not transform matrix, function returns identity transform matrix.
const TMatrix &DecalsMatrices::getBasisMatrix(ecs::EntityId eid) const
{
  if (entityComponent.str == nullptr || !g_entity_mgr->has(eid, entityComponent))
    return TMatrix::IDENT;

  const TMatrix *tm = g_entity_mgr->getNullable<TMatrix>(eid, ECS_HASH("transform"));
  if (tm)
    return *tm;

  return TMatrix::IDENT;
}

bool DecalsMatrices::isMatrixRemoved(uint32_t matrix_id) const
{
  if (matrix_id == INVALID_MATRIX_ID)
    return false;
  matrix_id--;
  G_ASSERT_RETURN(matrix_id < removedMatrices.size(), false);
  return removedMatrices[matrix_id];
}

uint32_t DecalsMatrices::numRemovedMatrices() const { return removedMatrixCount; }

void DecalsMatrices::clearDeletedMatrices()
{
  if (removedMatrixCount > 0)
  {
    eastl::fill(removedMatrices.begin(), removedMatrices.end(), false);
    removedMatrixCount = 0;
  }
}

void DecalsMatrices::useMatrixId(uint32_t id)
{
  if (id == INVALID_MATRIX_ID)
    return;
  gen++;
  matricesGen[id - 1] = getCurrentGeneration();
}

void DecalsMatrices::useMatrixId(uint32_t id, uint32_t item_id)
{
  if (id == INVALID_MATRIX_ID)
  {
    itemMap.resetItem(item_id);
  }
  else
  {
    useMatrixId(id);
    itemMap.set(id - 1, item_id);
  }
}

void DecalsMatrices::entity_update(ecs::EntityId eid, const TMatrix &tm)
{
  if (!registeredMatrixManagers)
    return;
  for (DecalsMatrices *mgr : registeredMatrixManagers->managers)
  {
    uint32_t id = mgr->getMatrixId(eid);
    if (id != INVALID_MATRIX_ID)
      mgr->setMatrix(id, tm);
  }
}

void DecalsMatrices::entity_delete(ecs::EntityId eid)
{
  if (!registeredMatrixManagers)
    return;
  for (DecalsMatrices *mgr : registeredMatrixManagers->managers)
  {
    uint32_t id = mgr->getMatrixId(eid);
    if (id != INVALID_MATRIX_ID)
      mgr->deleteMatrixId(id);
  }
}

DecalsMatrices::UniqueMap::UniqueMap(uint32_t num_matrices, uint32_t max_num_items) : matrixToItems(num_matrices)
{
  // if this fails, use uint32_t for id_t
  G_ASSERT(num_matrices <= INVALID_ID && max_num_items <= INVALID_ID);
  if (max_num_items > 0)
    resize(max_num_items);
  removedItems.reserve(DEFAULT_VECTOR_SIZE);
}

void DecalsMatrices::UniqueMap::resize(uint32_t max_num_items)
{
  // if this fails, use uint32_t for id_t
  G_ASSERT(max_num_items <= INVALID_ID);
  itemToMatrix.resize(max_num_items, INVALID_ID);
  clear();
}

void DecalsMatrices::UniqueMap::clear()
{
  for (auto &ids : matrixToItems)
  {
    ids.clear();
    ids.reserve(DEFAULT_VECTOR_SIZE);
  }
  eastl::fill(itemToMatrix.begin(), itemToMatrix.end(), INVALID_ID);
  removedItems.clear();
  removedItems.reserve(DEFAULT_VECTOR_SIZE);
}

void DecalsMatrices::UniqueMap::resetItem(uint32_t id)
{
  id_t &matrix = itemToMatrix[id];
  if (matrix == INVALID_ID)
    return;
  auto itr = eastl::find(matrixToItems[matrix].begin(), matrixToItems[matrix].end(), id);
  G_ASSERT(itr != matrixToItems[matrix].end());
  std::swap(*itr, *matrixToItems[matrix].rbegin());
  matrixToItems[matrix].pop_back();
  matrix = INVALID_ID;
}

void DecalsMatrices::UniqueMap::set(uint32_t matrix, uint32_t id)
{
  resetItem(id);
  itemToMatrix[id] = matrix;
  matrixToItems[matrix].push_back(id);
}

void DecalsMatrices::UniqueMap::removeMatrix(uint32_t matrix)
{
  while (matrixToItems[matrix].size() > 0)
  {
    const id_t id = matrixToItems[matrix].back();
    matrixToItems[matrix].pop_back();
    removedItems.push_back(id);
    itemToMatrix[id] = INVALID_ID;
  }
  matrixToItems[matrix].clear();
  matrixToItems[matrix].reserve(DEFAULT_VECTOR_SIZE);
}

uint32_t DecalsMatrices::UniqueMap::matrixByItem(uint32_t id) const { return itemToMatrix[id]; }


eastl::optional<uint32_t> DecalsMatrices::UniqueMap::popReusableItem()
{
  if (removedItems.size() > 0)
  {
    const uint32_t id = removedItems.back();
    removedItems.pop_back();
    return {id};
  }
  else
    return {};
}
