//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

// NOTE: values for classids are derived by hash from string
//       WITHOUT suffix "GameResClassId", e.g., hash("Texture")


/// standard game resource classes
static constexpr unsigned DynModelGameResClassId = 0xB4B7D9C4u;     // DynModel
static constexpr unsigned RendInstGameResClassId = 0x77F8232Fu;     // RendInst
static constexpr unsigned ImpostorDataGameResClassId = 0x2AD457F0u; // ImpostorData
static constexpr unsigned RndGrassGameResClassId = 0xA7D3ED6Au;     // RndGrass
static constexpr unsigned GeomNodeTreeGameResClassId = 0x56F81B6Du; // GeomNodeTree
static constexpr unsigned CharacterGameResClassId = 0xA6F87A9Bu;    // Character
static constexpr unsigned FastPhysDataGameResClassId = 0x855A1BE6u; // FastPhysData
static constexpr unsigned Anim2DataGameResClassId = 0x40C586F9u;    // Anim2Data
static constexpr unsigned PhysSysGameResClassId = 0xDBBCB7D2u;      // PhysSys
static constexpr unsigned PhysObjGameResClassId = 0xD543E771u;      // PhysObj
static constexpr unsigned CollisionGameResClassId = 0xACE50000u;    // CollisionResource
static constexpr unsigned RagdollGameResClassId = 0xD403C21Cu;      // Ragdoll
static constexpr unsigned EffectGameResClassId = 0x88B7A117u;       // Effect
static constexpr unsigned IslLightGameResClassId = 0x64EA5C39u;     // IslLight
static constexpr unsigned CloudGameResClassId = 0xE3F1FCE6u;        // Cloud
static constexpr unsigned MaterialGameResClassId = 0x39b5b09eu;     // Material
static constexpr unsigned LShaderGameResClassId = 0xCD5B9736u;      // LShader
static constexpr unsigned AnimBnlGameResClassId = 0x8F2A7018u;      // AnimChar
static constexpr unsigned AnimCharGameResClassId = 0x8F2A701Au;     // AnimChar
