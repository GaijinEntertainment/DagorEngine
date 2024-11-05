// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

enum class CompositeEditorRefreshType
{
  Unset,
  Nothing,                  // No need to refresh anything.
  Entity,                   // Refresh the entity.
  EntityAndTransformation,  // Refresh the entity and the transformation properties on the composit panel.
  EntityAndCompositeEditor, // Refresh the entity, the composit tree and the composit panel.
};
