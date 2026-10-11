//
// DaEditorX
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

static constexpr unsigned HUID_AfterProjectLoad = 0xA5A31695u;   // AfterProjectLoad
static constexpr unsigned HUID_ReloadBuiltScene = 0x094987F6u;   // ReloadBuiltScene
static constexpr unsigned HUID_UseVisibilityOpt = 0xABD39979u;   // UseVisibilityOpt
static constexpr unsigned HUID_SunPropertyChanged = 0xCDEAF48Eu; // SunPropertyChanged
static constexpr unsigned HUID_DumpEntityStat = 0x7D718498u;     // DumpEntityStat
static constexpr unsigned HUID_DumpRiEntityStat = 0xB309699Au;   // DumpRiEntityStat
static constexpr unsigned HUID_InvalidateClipmap = 0x2ABA45ADu;  // InvalidateClipmap
static constexpr unsigned HUID_AfterD3DReset = 0x2322B8F8u;      // AfterD3DReset
static constexpr unsigned HUID_AutoSaveDueToCrash = 0x998B4A22u; // AutoSaveDueToCrash
static constexpr unsigned HUID_PostRenderObjects = 0x38092EF8u;  // PostRenderObjects
static constexpr unsigned HUID_BeforeMainLoop = 0xC12CC2E4u;     // BeforeMainLoop
static constexpr unsigned HUID_ZoomAndCenter = 0xB0E3A360u;      // ZoomAndCenter
// Asked with the undo op owner token as user_data; a plugin that stamps a token of its own answers true.
static constexpr unsigned HUID_ClaimUndoOpOwner = 0x36CF5C84u; // ClaimUndoOpOwner
// Asked before the editor closes a project or itself, so a plugin holding unsaved work of its own can
// put it to the user. One that answers true stops the close.
static constexpr unsigned HUID_KeepEditorOpen = 0x9714412Du; // KeepEditorOpen

static constexpr unsigned INVALIDATE_CLIPMAP_FORCE_REDRAW = 1;
static constexpr unsigned INVALIDATE_CLIPMAP_SKIP_LAST_CLIP = 1 << 1;
