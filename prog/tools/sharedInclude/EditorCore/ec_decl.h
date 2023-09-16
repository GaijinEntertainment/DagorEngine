#ifndef __GAIJIN_EDITORCORE_EC_DECL_H__
#define __GAIJIN_EDITORCORE_EC_DECL_H__
#pragma once

// forward declarations for classes used in DagorEditor

// general interfaces
class EditableObject;
class IGenViewportWnd;
class IGenEventHandler;
class IGenEditorPluginBase;
class IGenEditorPlugin; //< defined by app, must be inherited from IGenEditorPluginBase
class IEditorCoreEngine;


// other interfaces
class IObjectsList;
class ICustomCameras;


// interface windows
class GenericEditorAppWindow;
class ViewportWindow;
class UndoSystem;
class GizmoEventFilter;
class BrushEventFilter;


// helper classes
struct CameraConfig;

// Hashed-Unique-IDs for common interfaces
static constexpr unsigned HUID_IEditorCore = 0xB59C93E8u;              // IEditorCore
static constexpr unsigned HUID_IRenderingService = 0x80C30E29u;        // IRenderingService
static constexpr unsigned HUID_IHDRChangeSettingsClient = 0x006003ABu; // IHDRChangeSettingsClient

#endif
