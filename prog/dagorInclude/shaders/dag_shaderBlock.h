//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

namespace ShaderGlobal
{
enum
{
  LAYER_FRAME,
  LAYER_SCENE,
  LAYER_OBJECT,
  LAYER_GLOBAL_CONST
};

//! searches shader block by name and returns ID
//! param 'layer' is optional and can be specified to assert that block belongs to this level
int getBlockId(const char *block_name, int layer = -1);

const char *getBlockName(int block_id);

//! sets shader block by ID to proper layer; changes global state and executes corresponding stcode
//! param 'layer' is optional and can be specified to assert that block belongs to this level
//! param 'block_id' can be -1, this imply that NULL state is set to destination layer
void setBlock(int block_id, int layer, bool invalidate_cached_stblk = true);

//! gets currently set shader block ID for a layer
int getBlock(int layer);

//! checks whether shader block (specified by ID and optionaly with layer) is allowed to be set
//! in current circumstances (i.e., it supports currently set blocks)
bool checkBlockCompatible(int block_id, int layer = -1);

//! returns encoded state word that contains information about blocks set in each layer
unsigned getCurBlockStateWord();

//! returns ID of block set in specified layer of block_state_word;
//! can be rather slow since StateWord -> blockId resolution requires linear search
int decodeBlock(unsigned block_state_word, int layer);

//! enables/disables automatic state block changes; returns previous permission state
//! when automatic state block changes are disabled (default), attempt to draw anything with
//! shader that doesn't support currently set blocks produces fatal error
//! when enabled, drawing anything with shader that doesn't support currently set blocks causes
//! changing set blocks to first suitable block configuration (can be slower than default mode, but
//! definetely more safe to use in editors, etc.)
bool enableAutoBlockChange(bool enable);

//! sets state blocks to change current state word to specified;
//! designed primarily for internal use (in auto-block-change mode)
void changeStateWord(unsigned new_block_state_word);
}; // namespace ShaderGlobal

struct ShaderBlockSetter
{
  int oldBlock;
  int currentLayer;
  ShaderBlockSetter(int new_block, int layer) : oldBlock(ShaderGlobal::getBlock(layer)), currentLayer(layer)
  {
    ShaderGlobal::setBlock(new_block, currentLayer);
  }

  ~ShaderBlockSetter() { ShaderGlobal::setBlock(oldBlock, currentLayer); }
};

#define LAYER_GUARD_GLUE(a, b)               a##b
#define LAYER_GUARD_INTERNAL(x, line, layer) ShaderBlockSetter LAYER_GUARD_GLUE(layer_guard, line)(x, layer)
#define SCENE_LAYER_GUARD(x)                 LAYER_GUARD_INTERNAL(x, __LINE__, ShaderGlobal::LAYER_SCENE)
#define FRAME_LAYER_GUARD(x)                 LAYER_GUARD_INTERNAL(x, __LINE__, ShaderGlobal::LAYER_FRAME)

class ScopeResetShaderBlocks
{
public:
  ScopeResetShaderBlocks()
  {
    prevFrameBlockId = ShaderGlobal::getBlock(ShaderGlobal::LAYER_FRAME);
    prevSceneBlockId = ShaderGlobal::getBlock(ShaderGlobal::LAYER_SCENE);
    prevObjectBlockId = ShaderGlobal::getBlock(ShaderGlobal::LAYER_OBJECT);
    ShaderGlobal::setBlock(-1, ShaderGlobal::LAYER_FRAME);
    ShaderGlobal::setBlock(-1, ShaderGlobal::LAYER_SCENE);
    ShaderGlobal::setBlock(-1, ShaderGlobal::LAYER_OBJECT);
  }

  ~ScopeResetShaderBlocks()
  {
    ShaderGlobal::setBlock(prevFrameBlockId, ShaderGlobal::LAYER_FRAME);
    ShaderGlobal::setBlock(prevSceneBlockId, ShaderGlobal::LAYER_SCENE);
    ShaderGlobal::setBlock(prevObjectBlockId, ShaderGlobal::LAYER_OBJECT);
  }

protected:
  int prevFrameBlockId;
  int prevSceneBlockId;
  int prevObjectBlockId;
};

#define SCOPE_RESET_SHADER_BLOCKS ScopeResetShaderBlocks scopeResetShaderBlocks##__LINE__
