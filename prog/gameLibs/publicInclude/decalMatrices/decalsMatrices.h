//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <drv/3d/dag_consts.h>
#include <3d/dag_resPtr.h>
#include <EASTL/vector.h>
#include <EASTL/unique_ptr.h>
#include <vecmath/dag_vecMathDecl.h>
#include <daECS/core/ecsHash.h>
#include <daECS/core/entityId.h>
#include <math/dag_TMatrix.h>
#include <EASTL/optional.h>

#include <decalMatrices/decal_matrices_const.hlsli>

class Sbuffer;

class DecalsMatrices
{
  struct DecalMatrix
  {
    vec4f row0, row1, row2;
  };

  UniqueBufHolder matricesCB;
  uint32_t idCount = 0;
  eastl::vector<DecalMatrix> matrices; // incompatible with mat43f from vecMath
  uint32_t removedMatrixCount = 0;
  eastl::vector<bool> removedMatrices;
  typedef uint32_t gen_type_t;
  static const gen_type_t never_used = eastl::numeric_limits<gen_type_t>::min();
  static const gen_type_t just_added = eastl::numeric_limits<gen_type_t>::max();
  eastl::vector<gen_type_t> matricesGen;
  eastl::vector<uint16_t> matricesReferences;
  bool matricesInvalid = false;
  uint32_t maxItemCount;
  uint32_t gen = 0;
  ecs::HashedConstString entityComponent = {nullptr, 0};
  void defrag();

  uint32_t getStillUsedGeneration() const { return gen; }
  uint32_t getCurrentGeneration() const { return gen + maxItemCount; }

  // UniqueMap stores associations between matrix ids and sets of item ids
  // The restriction is that item ids are unique, they can only be associated with a single matrix id
  // In other words all sets of item ids are pairwise disjoint
  class UniqueMap
  {
    using id_t = uint16_t;
    static constexpr int INVALID_ID = eastl::numeric_limits<id_t>::max();
    static constexpr int DEFAULT_VECTOR_SIZE = 20;

    eastl::vector<id_t> itemToMatrix;
    eastl::vector<eastl::vector<id_t>> matrixToItems;
    eastl::vector<id_t> removedItems; // these can be reused

  public:
    UniqueMap() = default;
    UniqueMap(uint32_t num_matrices, uint32_t max_num_items);

    void resize(uint32_t max_num_items);
    void clear();

    // associate item id with matrix id and remove previous association
    void set(uint32_t matrix, uint32_t id);

    // all associated items are marked for reuse
    void removeMatrix(uint32_t matrix);

    // return the matrix id, which is associated with the item id
    uint32_t matrixByItem(uint32_t id) const;

    // returns and resets an item, which is marked for reuse (associated with removedNode)
    eastl::optional<uint32_t> popReusableItem();

    // remove any associations with this item
    void resetItem(uint32_t item);

    // returns the number of items associated with the matrix
    int getMatrixItemCount(uint32_t matrix) const;
  } itemMap;

public:
  static constexpr int INVALID_MATRIX_ID = 0;
  DecalsMatrices() = default;
  // matrix_buffer_name: The buffer will be registered under this name and the this is also the shader var's name
  DecalsMatrices(size_t max_matrix_count, const char *matrix_buffer_name, uint32_t max_item_count);
  // matrix manager will be registered and connected to the ecs updater. The entity_component will store the matrix id on the entity
  DecalsMatrices(size_t max_matrix_count, const char *matrix_buffer_name, ecs::HashedConstString entity_component,
    uint32_t max_item_count);

  DecalsMatrices(const DecalsMatrices &) = delete;
  DecalsMatrices &operator=(const DecalsMatrices &) = delete;

  ~DecalsMatrices();

  void beforeRender();
  uint32_t addNewMatrixId();
  void deleteMatrixId(uint32_t id);
  void useMatrixId(uint32_t id);
  void useMatrixId(uint32_t id, uint32_t item_id);
  void setMatrix(uint32_t id, const DecalMatrix &mat);
  void setMatrix(uint32_t matrix_id, const TMatrix &matrix);
  const DecalMatrix &getMatrix(uint32_t id) const;
  bool isMatrixUsed(uint32_t id) const;

  // Return true when this matrix is used by 2 or more different decals which should have their
  // own matrices, but there was no empty space in the buffer with matrices.
  bool isMatrixAliased(uint32_t id) const;
  void reset() { matricesInvalid = true; }
  void clearItems();
  void clearAll();
  void setMaxItems(uint32_t max_items);
  void clearDeletedMatrices();
  bool isMatrixRemoved(uint32_t matrix_id) const;
  uint32_t numRemovedMatrices() const;
  eastl::optional<uint32_t> popReusableItem() { return itemMap.popReusableItem(); }
  void resetItem(uint32_t item_id) { itemMap.resetItem(item_id); }
  int getMatrixItemCount(uint32_t matrix_id) const;

  uint32_t registerMatrix(ecs::EntityId eid, const TMatrix &tm);
  uint32_t getMatrixId(ecs::EntityId eid) const;
  const TMatrix &getBasisMatrix(ecs::EntityId eid) const;

  // updates all registered matrices on this entity
  static void entity_update(ecs::EntityId eid, const TMatrix &tm);
  // deletes all registered matrices on this entity
  static void entity_delete(ecs::EntityId eid);
};
