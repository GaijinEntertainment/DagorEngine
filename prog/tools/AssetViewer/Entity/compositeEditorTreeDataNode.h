#pragma once
#include <dag/dag_vector.h>
#include <EASTL/unique_ptr.h>
#include <math/dag_TMatrix.h>
#include <ioSys/dag_dataBlock.h>

class CompositeEditorTreeDataNode
{
public:
  CompositeEditorTreeDataNode();

  void reset();
  const char *getAssetName(const char *defaultValue = "") const;
  const char *getAssetTypeName() const;
  const char *getName() const;
  float getWeight() const;
  bool getUseTransformationMatrix() const;
  TMatrix getTransformationMatrix() const;
  bool canTransform() const;
  bool tryGetPoint2Parameter(const char *name, Point2 &value) const;
  bool isEntBlock() const;
  bool hasEntBlock() const;
  bool hasNameParameter() const;
  bool canEditAssetName(bool isRootNode) const;
  bool canEditChildren() const;
  bool canEditRandomEntities(bool isRootNode) const;
  void insertEntBlock(int index);
  void insertNodeBlock(int index);
  void convertSingleRandomEntityNodeToRegularNode(bool isRootNode);

  DataBlock params;
  dag::Vector<eastl::unique_ptr<CompositeEditorTreeDataNode>> nodes;
  unsigned dataBlockId;
};
