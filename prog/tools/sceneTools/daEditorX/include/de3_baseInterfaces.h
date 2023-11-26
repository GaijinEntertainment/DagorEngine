//
// DaEditorX
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <generic/dag_tabFwd.h>
#include <de3_navmeshObstacle.h>
#include <de3_groundHole.h>
#include <de3_gameLadder.h>
#include <util/dag_oaHashNameMap.h>
#include <ska_hash_map/flat_hash_map2.hpp>

// forward declaration for external classes
class DagorAsset;
class DataBlock;
class IGenSave;
class ILogWriter;
class StaticGeometryContainer;

class LandMeshMap;

namespace ddsx
{
struct Buffer;
}
namespace mkbindump
{
class BinDumpSaveCB;
};
using mkbindump::BinDumpSaveCB;

typedef class PropertyContainerControlBase PropPanel2;

//! Textures numerator (works like namemap)
class ITextureNumerator
{
public:
  virtual int addTextureName(const char *fname) = 0;
  virtual int addDDSxTexture(const char *fname, ddsx::Buffer &b) = 0;
  virtual int getTextureOrdinal(const char *fname) const = 0;

  virtual int getTargetCode() const = 0;
};


//! Level binary data build/export interface
class IBinaryDataBuilder
{
public:
  static constexpr unsigned HUID = 0x0B2E4A51u; // IBinaryDataBuilder

  //! called first to check prequisites before build/export binary data
  virtual bool validateBuild(int target, ILogWriter &rep, PropPanel2 *params) = 0;
  //! called after validation pass done to gather all used textures
  virtual bool addUsedTextures(ITextureNumerator &tn) = 0;
  //! finally called to build and write binary data to output stream
  virtual bool buildAndWrite(BinDumpSaveCB &cwr, const ITextureNumerator &tn, PropPanel2 *pp) = 0;

  //! checks custom data metrics specified in datablock
  virtual bool checkMetrics(const DataBlock &metrics_blk) = 0;

  //! optional interface to provide export-time filled parameters
  virtual bool useExportParameters() const { return false; }
  virtual void fillExportPanel(PropPanel2 &params) {}
};


//! Level binary data build/export interface
class ILevelResListBuilder
{
public:
  static constexpr unsigned HUID = 0xFFC4810Du; // ILevelResListBuilder
  virtual void addUsedResNames(OAHashNameMap<true> &res_list) = 0;
};


//! Static geometry gather interface
class IGatherStaticGeometry
{
public:
  static constexpr unsigned HUID = 0x148BAFE8u; // IGatherStaticGeometry

  //! called to get static scene visual geometry
  virtual void gatherStaticVisualGeometry(StaticGeometryContainer &container) = 0;
  //! called to get static environment scene visual geometry
  virtual void gatherStaticEnviGeometry(StaticGeometryContainer &container) = 0;

  //! called to get static scene collsion geometry (for game)
  virtual void gatherStaticCollisionGeomGame(StaticGeometryContainer &container) = 0;
  //! called to get static scene collsion geometry (for editor)
  virtual void gatherStaticCollisionGeomEditor(StaticGeometryContainer &container) = 0;

  //! called to get static scene visual geometry
  virtual void gatherStaticVisualGeometryForStage(StaticGeometryContainer &container, int stage_idx) {}
};


//! On-lighting-change notification interface
class ILightingChangeClient
{
public:
  static constexpr unsigned HUID = 0x73E29D7Cu; // ILightingChangeClient

  //! called to notify about any lighting changes
  virtual void onLightingChanged() = 0;

  //! called to notify about lighting settings changes
  virtual void onLightingSettingsChanged() = 0;
};


//! Additional LTinput data export interface
class IWriteAddLtinputData
{
public:
  static constexpr unsigned HUID = 0xFDB7238Du; // IWriteAddLtinputData

  //! called when exporting LTinput fo daLight, just after all base data (geometry) was exported
  virtual void writeAddLtinputData(IGenSave &cwr) = 0;
};

//! on-export notification interface (can be used to prefetch streamed data)
class IOnExportNotify
{
public:
  static constexpr unsigned HUID = 0xB301BE83u; // IOnExportNotify

  //! called just before export operation for all plugins/services start
  virtual void onBeforeExport(unsigned target_code) = 0;
  //! called after export operation for all plugins/services done
  virtual void onAfterExport(unsigned target_code) = 0;
};


class IColor
{
public:
  static constexpr unsigned HUID = 0x7A16A8DDu; // IColor

  virtual void setColor(unsigned int in_color) = 0;
  virtual unsigned int getColor() = 0;
};

class IRendInstEntity
{
public:
  static constexpr unsigned HUID = 0xF03EE677u; // IRendInstEntity

  virtual DagorAsset *getAsset() = 0;
  virtual int getAssetNameId() = 0;
  virtual const char *getResourceName() = 0;
  virtual int getPoolIndex() = 0;

  // Gets the transformation matrix that corresponds to the aligned/compressed/quantized
  // matrix where the rendInst is actually drawn at.
  // The out_tm parameter is only set in case of success.
  // Returns with true in case of success.
  virtual bool getRendInstQuantizedTm(TMatrix &out_tm) const = 0;
};


class IExportToDag
{
public:
  static constexpr unsigned HUID = 0x8B862BABu; // IExportToDag

  virtual void gatherExportToDagGeometry(StaticGeometryContainer &container) = 0;
};

class IWriterToLandmesh
{
public:
  static constexpr unsigned HUID = 0xDB2A3BB1u; // IWriterToLandmesh

  virtual void writeToLandmesh(LandMeshMap &landmesh) = 0;
};

class IPluginAutoSave
{
public:
  static constexpr unsigned HUID = 0xF5D85BF6u; // IPluginAutoSave

  virtual void autoSaveObjects(DataBlock &local_data) = 0;
};


class IPostProcessGeometry
{
public:
  static constexpr unsigned HUID = 0xA1D88595u; // IPostProcessGeometry

  virtual void processGeometry(StaticGeometryContainer &container) = 0;
};

class IGatherCollision
{
public:
  static constexpr unsigned HUID = 0xA1D88598u;

  virtual void gatherCollision(const BBox3 &box, Tab<Point3> &vertices, Tab<int> &indices, Tab<IPoint2> &transparent,
    Tab<NavMeshObstacle> &obstacles, const Tab<BBox3> &exclude_boxes) = 0;
  virtual void getObstaclesFlags(const ska::flat_hash_map<uint32_t, uint32_t> *&obstacle_flags_by_res_name_hash) = 0;
};


class IGatherGroundHoles
{
public:
  static constexpr unsigned HUID = 0x0D508A8Eu; // IGatherGroundHoles

  virtual void gatherGroundHoles(Tab<GroundHole> &obstacles) = 0;
};

class IGatherRiNavmeshBlockers
{
public:
  static constexpr unsigned HUID = 0xA41DA302u; // IGatherRiNavmeshBlockers

  virtual void gatherRiNavmeshBlockers(Tab<BBox3> &blockers) = 0;
};

class IGatherGameLadders
{
public:
  static constexpr unsigned HUID = 0xB3E9280Cu; // IGatherGameLadders

  virtual void gatherGameLadders(Tab<GameLadder> &ladders) = 0;
};
