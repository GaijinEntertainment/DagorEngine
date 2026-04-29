// this file is generated via Daslang automatic C++ binder
// all user modifications will be lost after this file is re-generated

// from imgui/imgui.h:372:8
struct ImTextureRef_GeneratedAnnotation : ManagedStructureAnnotation<ImTextureRef> {
	ImTextureRef_GeneratedAnnotation(ModuleLibrary & ml) : ManagedStructureAnnotation ("ImTextureRef", ml, "ImTextureRef") {
	}
	void init () {
		addField<DAS_BIND_MANAGED_FIELD(_TexData)>("_TexData","_TexData");
		addField<DAS_BIND_MANAGED_FIELD(_TexID)>("_TexID","_TexID");
	}
};

// from imgui/imgui.h:2229:8
struct ImGuiTableSortSpecs_GeneratedAnnotation : ManagedStructureAnnotation<ImGuiTableSortSpecs> {
	ImGuiTableSortSpecs_GeneratedAnnotation(ModuleLibrary & ml) : ManagedStructureAnnotation ("ImGuiTableSortSpecs", ml, "ImGuiTableSortSpecs") {
	}
	void init () {
		addField<DAS_BIND_MANAGED_FIELD(Specs)>("Specs","Specs");
		addField<DAS_BIND_MANAGED_FIELD(SpecsCount)>("SpecsCount","SpecsCount");
		addField<DAS_BIND_MANAGED_FIELD(SpecsDirty)>("SpecsDirty","SpecsDirty");
	}
};

// from imgui/imgui.h:2239:8
struct ImGuiTableColumnSortSpecs_GeneratedAnnotation : ManagedStructureAnnotation<ImGuiTableColumnSortSpecs> {
	virtual bool isLocal() const override { return true; }
	virtual bool canBePlacedInContainer() const override { return true; }
	ImGuiTableColumnSortSpecs_GeneratedAnnotation(ModuleLibrary & ml) : ManagedStructureAnnotation ("ImGuiTableColumnSortSpecs", ml, "ImGuiTableColumnSortSpecs") {
	}
	void init () {
		addField<DAS_BIND_MANAGED_FIELD(ColumnUserID)>("ColumnUserID","ColumnUserID");
		addField<DAS_BIND_MANAGED_FIELD(ColumnIndex)>("ColumnIndex","ColumnIndex");
		addField<DAS_BIND_MANAGED_FIELD(SortOrder)>("SortOrder","SortOrder");
		addField<ImGuiSortDirection,offsetof(ManagedType,SortDirection)>("SortDirection","SortDirection")
			.adjustAot("das_reinterpret<ImGuiSortDirection_>::pass(",")");
	}
};

// from imgui/imgui.h:2269:8
struct ImNewWrapper_GeneratedAnnotation : ManagedStructureAnnotation<ImNewWrapper> {
	ImNewWrapper_GeneratedAnnotation(ModuleLibrary & ml) : ManagedStructureAnnotation ("ImNewWrapper", ml, "ImNewWrapper") {
	}
	void init () {
	}
};

// from imgui/imgui.h:2366:8
struct ImGuiStyle_GeneratedAnnotation : ManagedStructureAnnotation<ImGuiStyle> {
	ImGuiStyle_GeneratedAnnotation(ModuleLibrary & ml) : ManagedStructureAnnotation ("ImGuiStyle", ml, "ImGuiStyle") {
	}
	void init () {
		addField<DAS_BIND_MANAGED_FIELD(FontSizeBase)>("FontSizeBase","FontSizeBase");
		addField<DAS_BIND_MANAGED_FIELD(FontScaleMain)>("FontScaleMain","FontScaleMain");
		addField<DAS_BIND_MANAGED_FIELD(FontScaleDpi)>("FontScaleDpi","FontScaleDpi");
		addField<DAS_BIND_MANAGED_FIELD(Alpha)>("Alpha","Alpha");
		addField<DAS_BIND_MANAGED_FIELD(DisabledAlpha)>("DisabledAlpha","DisabledAlpha");
		addField<DAS_BIND_MANAGED_FIELD(WindowPadding)>("WindowPadding","WindowPadding");
		addField<DAS_BIND_MANAGED_FIELD(WindowRounding)>("WindowRounding","WindowRounding");
		addField<DAS_BIND_MANAGED_FIELD(WindowBorderSize)>("WindowBorderSize","WindowBorderSize");
		addField<DAS_BIND_MANAGED_FIELD(WindowBorderHoverPadding)>("WindowBorderHoverPadding","WindowBorderHoverPadding");
		addField<DAS_BIND_MANAGED_FIELD(WindowMinSize)>("WindowMinSize","WindowMinSize");
		addField<DAS_BIND_MANAGED_FIELD(WindowTitleAlign)>("WindowTitleAlign","WindowTitleAlign");
		addField<ImGuiDir,offsetof(ManagedType,WindowMenuButtonPosition)>("WindowMenuButtonPosition","WindowMenuButtonPosition")
			.adjustAot("das_reinterpret<ImGuiDir_>::pass(",")");
		addField<DAS_BIND_MANAGED_FIELD(ChildRounding)>("ChildRounding","ChildRounding");
		addField<DAS_BIND_MANAGED_FIELD(ChildBorderSize)>("ChildBorderSize","ChildBorderSize");
		addField<DAS_BIND_MANAGED_FIELD(PopupRounding)>("PopupRounding","PopupRounding");
		addField<DAS_BIND_MANAGED_FIELD(PopupBorderSize)>("PopupBorderSize","PopupBorderSize");
		addField<DAS_BIND_MANAGED_FIELD(FramePadding)>("FramePadding","FramePadding");
		addField<DAS_BIND_MANAGED_FIELD(FrameRounding)>("FrameRounding","FrameRounding");
		addField<DAS_BIND_MANAGED_FIELD(FrameBorderSize)>("FrameBorderSize","FrameBorderSize");
		addField<DAS_BIND_MANAGED_FIELD(ItemSpacing)>("ItemSpacing","ItemSpacing");
		addField<DAS_BIND_MANAGED_FIELD(ItemInnerSpacing)>("ItemInnerSpacing","ItemInnerSpacing");
		addField<DAS_BIND_MANAGED_FIELD(CellPadding)>("CellPadding","CellPadding");
		addField<DAS_BIND_MANAGED_FIELD(TouchExtraPadding)>("TouchExtraPadding","TouchExtraPadding");
		addField<DAS_BIND_MANAGED_FIELD(IndentSpacing)>("IndentSpacing","IndentSpacing");
		addField<DAS_BIND_MANAGED_FIELD(ColumnsMinSpacing)>("ColumnsMinSpacing","ColumnsMinSpacing");
		addField<DAS_BIND_MANAGED_FIELD(ScrollbarSize)>("ScrollbarSize","ScrollbarSize");
		addField<DAS_BIND_MANAGED_FIELD(ScrollbarRounding)>("ScrollbarRounding","ScrollbarRounding");
		addField<DAS_BIND_MANAGED_FIELD(ScrollbarPadding)>("ScrollbarPadding","ScrollbarPadding");
		addField<DAS_BIND_MANAGED_FIELD(GrabMinSize)>("GrabMinSize","GrabMinSize");
		addField<DAS_BIND_MANAGED_FIELD(GrabRounding)>("GrabRounding","GrabRounding");
		addField<DAS_BIND_MANAGED_FIELD(LogSliderDeadzone)>("LogSliderDeadzone","LogSliderDeadzone");
		addField<DAS_BIND_MANAGED_FIELD(ImageRounding)>("ImageRounding","ImageRounding");
		addField<DAS_BIND_MANAGED_FIELD(ImageBorderSize)>("ImageBorderSize","ImageBorderSize");
		addField<DAS_BIND_MANAGED_FIELD(TabRounding)>("TabRounding","TabRounding");
		addField<DAS_BIND_MANAGED_FIELD(TabBorderSize)>("TabBorderSize","TabBorderSize");
		addField<DAS_BIND_MANAGED_FIELD(TabMinWidthBase)>("TabMinWidthBase","TabMinWidthBase");
		addField<DAS_BIND_MANAGED_FIELD(TabMinWidthShrink)>("TabMinWidthShrink","TabMinWidthShrink");
		addField<DAS_BIND_MANAGED_FIELD(TabCloseButtonMinWidthSelected)>("TabCloseButtonMinWidthSelected","TabCloseButtonMinWidthSelected");
		addField<DAS_BIND_MANAGED_FIELD(TabCloseButtonMinWidthUnselected)>("TabCloseButtonMinWidthUnselected","TabCloseButtonMinWidthUnselected");
		addField<DAS_BIND_MANAGED_FIELD(TabBarBorderSize)>("TabBarBorderSize","TabBarBorderSize");
		addField<DAS_BIND_MANAGED_FIELD(TabBarOverlineSize)>("TabBarOverlineSize","TabBarOverlineSize");
		addField<DAS_BIND_MANAGED_FIELD(TableAngledHeadersAngle)>("TableAngledHeadersAngle","TableAngledHeadersAngle");
		addField<DAS_BIND_MANAGED_FIELD(TableAngledHeadersTextAlign)>("TableAngledHeadersTextAlign","TableAngledHeadersTextAlign");
		addField<ImGuiTreeNodeFlags_,offsetof(ManagedType,TreeLinesFlags)>("TreeLinesFlags","TreeLinesFlags")
			.adjustAot("das_reinterpret<ImGuiTreeNodeFlags_>::pass(",")");
		addField<DAS_BIND_MANAGED_FIELD(TreeLinesSize)>("TreeLinesSize","TreeLinesSize");
		addField<DAS_BIND_MANAGED_FIELD(TreeLinesRounding)>("TreeLinesRounding","TreeLinesRounding");
		addField<DAS_BIND_MANAGED_FIELD(DragDropTargetRounding)>("DragDropTargetRounding","DragDropTargetRounding");
		addField<DAS_BIND_MANAGED_FIELD(DragDropTargetBorderSize)>("DragDropTargetBorderSize","DragDropTargetBorderSize");
		addField<DAS_BIND_MANAGED_FIELD(DragDropTargetPadding)>("DragDropTargetPadding","DragDropTargetPadding");
		addField<DAS_BIND_MANAGED_FIELD(ColorMarkerSize)>("ColorMarkerSize","ColorMarkerSize");
		addField<ImGuiDir,offsetof(ManagedType,ColorButtonPosition)>("ColorButtonPosition","ColorButtonPosition")
			.adjustAot("das_reinterpret<ImGuiDir_>::pass(",")");
		addField<DAS_BIND_MANAGED_FIELD(ButtonTextAlign)>("ButtonTextAlign","ButtonTextAlign");
		addField<DAS_BIND_MANAGED_FIELD(SelectableTextAlign)>("SelectableTextAlign","SelectableTextAlign");
		addField<DAS_BIND_MANAGED_FIELD(SeparatorTextBorderSize)>("SeparatorTextBorderSize","SeparatorTextBorderSize");
		addField<DAS_BIND_MANAGED_FIELD(SeparatorTextAlign)>("SeparatorTextAlign","SeparatorTextAlign");
		addField<DAS_BIND_MANAGED_FIELD(SeparatorTextPadding)>("SeparatorTextPadding","SeparatorTextPadding");
		addField<DAS_BIND_MANAGED_FIELD(DisplayWindowPadding)>("DisplayWindowPadding","DisplayWindowPadding");
		addField<DAS_BIND_MANAGED_FIELD(DisplaySafeAreaPadding)>("DisplaySafeAreaPadding","DisplaySafeAreaPadding");
		addField<DAS_BIND_MANAGED_FIELD(DockingNodeHasCloseButton)>("DockingNodeHasCloseButton","DockingNodeHasCloseButton");
		addField<DAS_BIND_MANAGED_FIELD(DockingSeparatorSize)>("DockingSeparatorSize","DockingSeparatorSize");
		addField<DAS_BIND_MANAGED_FIELD(MouseCursorScale)>("MouseCursorScale","MouseCursorScale");
		addField<DAS_BIND_MANAGED_FIELD(AntiAliasedLines)>("AntiAliasedLines","AntiAliasedLines");
		addField<DAS_BIND_MANAGED_FIELD(AntiAliasedLinesUseTex)>("AntiAliasedLinesUseTex","AntiAliasedLinesUseTex");
		addField<DAS_BIND_MANAGED_FIELD(AntiAliasedFill)>("AntiAliasedFill","AntiAliasedFill");
		addField<DAS_BIND_MANAGED_FIELD(CurveTessellationTol)>("CurveTessellationTol","CurveTessellationTol");
		addField<DAS_BIND_MANAGED_FIELD(CircleTessellationMaxError)>("CircleTessellationMaxError","CircleTessellationMaxError");
		addField<DAS_BIND_MANAGED_FIELD(Colors)>("Colors","Colors");
		addField<DAS_BIND_MANAGED_FIELD(HoverStationaryDelay)>("HoverStationaryDelay","HoverStationaryDelay");
		addField<DAS_BIND_MANAGED_FIELD(HoverDelayShort)>("HoverDelayShort","HoverDelayShort");
		addField<DAS_BIND_MANAGED_FIELD(HoverDelayNormal)>("HoverDelayNormal","HoverDelayNormal");
		addField<ImGuiHoveredFlags_,offsetof(ManagedType,HoverFlagsForTooltipMouse)>("HoverFlagsForTooltipMouse","HoverFlagsForTooltipMouse")
			.adjustAot("das_reinterpret<ImGuiHoveredFlags_>::pass(",")");
		addField<ImGuiHoveredFlags_,offsetof(ManagedType,HoverFlagsForTooltipNav)>("HoverFlagsForTooltipNav","HoverFlagsForTooltipNav")
			.adjustAot("das_reinterpret<ImGuiHoveredFlags_>::pass(",")");
		addField<DAS_BIND_MANAGED_FIELD(_MainScale)>("_MainScale","_MainScale");
		addField<DAS_BIND_MANAGED_FIELD(_NextFrameFontSizeBase)>("_NextFrameFontSizeBase","_NextFrameFontSizeBase");
	}
};

// from imgui/imgui.h:2477:8
struct ImGuiKeyData_GeneratedAnnotation : ManagedStructureAnnotation<ImGuiKeyData> {
	ImGuiKeyData_GeneratedAnnotation(ModuleLibrary & ml) : ManagedStructureAnnotation ("ImGuiKeyData", ml, "ImGuiKeyData") {
	}
	void init () {
		addField<DAS_BIND_MANAGED_FIELD(Down)>("Down","Down");
		addField<DAS_BIND_MANAGED_FIELD(DownDuration)>("DownDuration","DownDuration");
		addField<DAS_BIND_MANAGED_FIELD(DownDurationPrev)>("DownDurationPrev","DownDurationPrev");
		addField<DAS_BIND_MANAGED_FIELD(AnalogValue)>("AnalogValue","AnalogValue");
	}
};

// from imgui/imgui.h:2485:8
struct ImGuiIO_GeneratedAnnotation : ManagedStructureAnnotation<ImGuiIO> {
	ImGuiIO_GeneratedAnnotation(ModuleLibrary & ml) : ManagedStructureAnnotation ("ImGuiIO", ml, "ImGuiIO") {
	}
	void init () {
		addField<ImGuiConfigFlags_,offsetof(ManagedType,ConfigFlags)>("ConfigFlags","ConfigFlags")
			.adjustAot("das_reinterpret<ImGuiConfigFlags_>::pass(",")");
		addField<ImGuiBackendFlags_,offsetof(ManagedType,BackendFlags)>("BackendFlags","BackendFlags")
			.adjustAot("das_reinterpret<ImGuiBackendFlags_>::pass(",")");
		addField<DAS_BIND_MANAGED_FIELD(DisplaySize)>("DisplaySize","DisplaySize");
		addField<DAS_BIND_MANAGED_FIELD(DisplayFramebufferScale)>("DisplayFramebufferScale","DisplayFramebufferScale");
		addField<DAS_BIND_MANAGED_FIELD(DeltaTime)>("DeltaTime","DeltaTime");
		addField<DAS_BIND_MANAGED_FIELD(IniSavingRate)>("IniSavingRate","IniSavingRate");
		addField<DAS_BIND_MANAGED_FIELD(IniFilename)>("IniFilename","IniFilename");
		addField<DAS_BIND_MANAGED_FIELD(LogFilename)>("LogFilename","LogFilename");
		addField<DAS_BIND_MANAGED_FIELD(UserData)>("UserData","UserData");
		addField<DAS_BIND_MANAGED_FIELD(Fonts)>("Fonts","Fonts");
		addField<DAS_BIND_MANAGED_FIELD(FontDefault)>("FontDefault","FontDefault");
		addField<DAS_BIND_MANAGED_FIELD(FontAllowUserScaling)>("FontAllowUserScaling","FontAllowUserScaling");
		addField<DAS_BIND_MANAGED_FIELD(ConfigNavSwapGamepadButtons)>("ConfigNavSwapGamepadButtons","ConfigNavSwapGamepadButtons");
		addField<DAS_BIND_MANAGED_FIELD(ConfigNavMoveSetMousePos)>("ConfigNavMoveSetMousePos","ConfigNavMoveSetMousePos");
		addField<DAS_BIND_MANAGED_FIELD(ConfigNavCaptureKeyboard)>("ConfigNavCaptureKeyboard","ConfigNavCaptureKeyboard");
		addField<DAS_BIND_MANAGED_FIELD(ConfigNavEscapeClearFocusItem)>("ConfigNavEscapeClearFocusItem","ConfigNavEscapeClearFocusItem");
		addField<DAS_BIND_MANAGED_FIELD(ConfigNavEscapeClearFocusWindow)>("ConfigNavEscapeClearFocusWindow","ConfigNavEscapeClearFocusWindow");
		addField<DAS_BIND_MANAGED_FIELD(ConfigNavCursorVisibleAuto)>("ConfigNavCursorVisibleAuto","ConfigNavCursorVisibleAuto");
		addField<DAS_BIND_MANAGED_FIELD(ConfigNavCursorVisibleAlways)>("ConfigNavCursorVisibleAlways","ConfigNavCursorVisibleAlways");
		addField<DAS_BIND_MANAGED_FIELD(ConfigDockingNoSplit)>("ConfigDockingNoSplit","ConfigDockingNoSplit");
		addField<DAS_BIND_MANAGED_FIELD(ConfigDockingNoDockingOver)>("ConfigDockingNoDockingOver","ConfigDockingNoDockingOver");
		addField<DAS_BIND_MANAGED_FIELD(ConfigDockingWithShift)>("ConfigDockingWithShift","ConfigDockingWithShift");
		addField<DAS_BIND_MANAGED_FIELD(ConfigDockingAlwaysTabBar)>("ConfigDockingAlwaysTabBar","ConfigDockingAlwaysTabBar");
		addField<DAS_BIND_MANAGED_FIELD(ConfigDockingTransparentPayload)>("ConfigDockingTransparentPayload","ConfigDockingTransparentPayload");
		addField<DAS_BIND_MANAGED_FIELD(ConfigViewportsNoAutoMerge)>("ConfigViewportsNoAutoMerge","ConfigViewportsNoAutoMerge");
		addField<DAS_BIND_MANAGED_FIELD(ConfigViewportsNoTaskBarIcon)>("ConfigViewportsNoTaskBarIcon","ConfigViewportsNoTaskBarIcon");
		addField<DAS_BIND_MANAGED_FIELD(ConfigViewportsNoDecoration)>("ConfigViewportsNoDecoration","ConfigViewportsNoDecoration");
		addField<DAS_BIND_MANAGED_FIELD(ConfigViewportsNoDefaultParent)>("ConfigViewportsNoDefaultParent","ConfigViewportsNoDefaultParent");
		addField<DAS_BIND_MANAGED_FIELD(ConfigViewportsPlatformFocusSetsImGuiFocus)>("ConfigViewportsPlatformFocusSetsImGuiFocus","ConfigViewportsPlatformFocusSetsImGuiFocus");
		addField<DAS_BIND_MANAGED_FIELD(ConfigDpiScaleFonts)>("ConfigDpiScaleFonts","ConfigDpiScaleFonts");
		addField<DAS_BIND_MANAGED_FIELD(ConfigDpiScaleViewports)>("ConfigDpiScaleViewports","ConfigDpiScaleViewports");
		addField<DAS_BIND_MANAGED_FIELD(MouseDrawCursor)>("MouseDrawCursor","MouseDrawCursor");
		addField<DAS_BIND_MANAGED_FIELD(ConfigMacOSXBehaviors)>("ConfigMacOSXBehaviors","ConfigMacOSXBehaviors");
		addField<DAS_BIND_MANAGED_FIELD(ConfigInputTrickleEventQueue)>("ConfigInputTrickleEventQueue","ConfigInputTrickleEventQueue");
		addField<DAS_BIND_MANAGED_FIELD(ConfigInputTextCursorBlink)>("ConfigInputTextCursorBlink","ConfigInputTextCursorBlink");
		addField<DAS_BIND_MANAGED_FIELD(ConfigInputTextEnterKeepActive)>("ConfigInputTextEnterKeepActive","ConfigInputTextEnterKeepActive");
		addField<DAS_BIND_MANAGED_FIELD(ConfigDragClickToInputText)>("ConfigDragClickToInputText","ConfigDragClickToInputText");
		addField<DAS_BIND_MANAGED_FIELD(ConfigWindowsResizeFromEdges)>("ConfigWindowsResizeFromEdges","ConfigWindowsResizeFromEdges");
		addField<DAS_BIND_MANAGED_FIELD(ConfigWindowsMoveFromTitleBarOnly)>("ConfigWindowsMoveFromTitleBarOnly","ConfigWindowsMoveFromTitleBarOnly");
		addField<DAS_BIND_MANAGED_FIELD(ConfigWindowsCopyContentsWithCtrlC)>("ConfigWindowsCopyContentsWithCtrlC","ConfigWindowsCopyContentsWithCtrlC");
		addField<DAS_BIND_MANAGED_FIELD(ConfigScrollbarScrollByPage)>("ConfigScrollbarScrollByPage","ConfigScrollbarScrollByPage");
		addField<DAS_BIND_MANAGED_FIELD(ConfigMemoryCompactTimer)>("ConfigMemoryCompactTimer","ConfigMemoryCompactTimer");
		addField<DAS_BIND_MANAGED_FIELD(MouseDoubleClickTime)>("MouseDoubleClickTime","MouseDoubleClickTime");
		addField<DAS_BIND_MANAGED_FIELD(MouseDoubleClickMaxDist)>("MouseDoubleClickMaxDist","MouseDoubleClickMaxDist");
		addField<DAS_BIND_MANAGED_FIELD(MouseDragThreshold)>("MouseDragThreshold","MouseDragThreshold");
		addField<DAS_BIND_MANAGED_FIELD(KeyRepeatDelay)>("KeyRepeatDelay","KeyRepeatDelay");
		addField<DAS_BIND_MANAGED_FIELD(KeyRepeatRate)>("KeyRepeatRate","KeyRepeatRate");
		addField<DAS_BIND_MANAGED_FIELD(ConfigErrorRecovery)>("ConfigErrorRecovery","ConfigErrorRecovery");
		addField<DAS_BIND_MANAGED_FIELD(ConfigErrorRecoveryEnableAssert)>("ConfigErrorRecoveryEnableAssert","ConfigErrorRecoveryEnableAssert");
		addField<DAS_BIND_MANAGED_FIELD(ConfigErrorRecoveryEnableDebugLog)>("ConfigErrorRecoveryEnableDebugLog","ConfigErrorRecoveryEnableDebugLog");
		addField<DAS_BIND_MANAGED_FIELD(ConfigErrorRecoveryEnableTooltip)>("ConfigErrorRecoveryEnableTooltip","ConfigErrorRecoveryEnableTooltip");
		addField<DAS_BIND_MANAGED_FIELD(ConfigDebugIsDebuggerPresent)>("ConfigDebugIsDebuggerPresent","ConfigDebugIsDebuggerPresent");
		addField<DAS_BIND_MANAGED_FIELD(ConfigDebugHighlightIdConflicts)>("ConfigDebugHighlightIdConflicts","ConfigDebugHighlightIdConflicts");
		addField<DAS_BIND_MANAGED_FIELD(ConfigDebugHighlightIdConflictsShowItemPicker)>("ConfigDebugHighlightIdConflictsShowItemPicker","ConfigDebugHighlightIdConflictsShowItemPicker");
		addField<DAS_BIND_MANAGED_FIELD(ConfigDebugBeginReturnValueOnce)>("ConfigDebugBeginReturnValueOnce","ConfigDebugBeginReturnValueOnce");
		addField<DAS_BIND_MANAGED_FIELD(ConfigDebugBeginReturnValueLoop)>("ConfigDebugBeginReturnValueLoop","ConfigDebugBeginReturnValueLoop");
		addField<DAS_BIND_MANAGED_FIELD(ConfigDebugIgnoreFocusLoss)>("ConfigDebugIgnoreFocusLoss","ConfigDebugIgnoreFocusLoss");
		addField<DAS_BIND_MANAGED_FIELD(ConfigDebugIniSettings)>("ConfigDebugIniSettings","ConfigDebugIniSettings");
		addField<DAS_BIND_MANAGED_FIELD(BackendPlatformName)>("BackendPlatformName","BackendPlatformName");
		addField<DAS_BIND_MANAGED_FIELD(BackendRendererName)>("BackendRendererName","BackendRendererName");
		addField<DAS_BIND_MANAGED_FIELD(BackendPlatformUserData)>("BackendPlatformUserData","BackendPlatformUserData");
		addField<DAS_BIND_MANAGED_FIELD(BackendRendererUserData)>("BackendRendererUserData","BackendRendererUserData");
		addField<DAS_BIND_MANAGED_FIELD(BackendLanguageUserData)>("BackendLanguageUserData","BackendLanguageUserData");
		addField<DAS_BIND_MANAGED_FIELD(WantCaptureMouse)>("WantCaptureMouse","WantCaptureMouse");
		addField<DAS_BIND_MANAGED_FIELD(WantCaptureKeyboard)>("WantCaptureKeyboard","WantCaptureKeyboard");
		addField<DAS_BIND_MANAGED_FIELD(WantTextInput)>("WantTextInput","WantTextInput");
		addField<DAS_BIND_MANAGED_FIELD(WantSetMousePos)>("WantSetMousePos","WantSetMousePos");
		addField<DAS_BIND_MANAGED_FIELD(WantSaveIniSettings)>("WantSaveIniSettings","WantSaveIniSettings");
		addField<DAS_BIND_MANAGED_FIELD(NavActive)>("NavActive","NavActive");
		addField<DAS_BIND_MANAGED_FIELD(NavVisible)>("NavVisible","NavVisible");
		addField<DAS_BIND_MANAGED_FIELD(Framerate)>("Framerate","Framerate");
		addField<DAS_BIND_MANAGED_FIELD(MetricsRenderVertices)>("MetricsRenderVertices","MetricsRenderVertices");
		addField<DAS_BIND_MANAGED_FIELD(MetricsRenderIndices)>("MetricsRenderIndices","MetricsRenderIndices");
		addField<DAS_BIND_MANAGED_FIELD(MetricsRenderWindows)>("MetricsRenderWindows","MetricsRenderWindows");
		addField<DAS_BIND_MANAGED_FIELD(MetricsActiveWindows)>("MetricsActiveWindows","MetricsActiveWindows");
		addField<DAS_BIND_MANAGED_FIELD(MouseDelta)>("MouseDelta","MouseDelta");
		addField<DAS_BIND_MANAGED_FIELD(Ctx)>("Ctx","Ctx");
		addField<DAS_BIND_MANAGED_FIELD(MousePos)>("MousePos","MousePos");
		addField<DAS_BIND_MANAGED_FIELD(MouseDown)>("MouseDown","MouseDown");
		addField<DAS_BIND_MANAGED_FIELD(MouseWheel)>("MouseWheel","MouseWheel");
		addField<DAS_BIND_MANAGED_FIELD(MouseWheelH)>("MouseWheelH","MouseWheelH");
		addField<DAS_BIND_MANAGED_FIELD(MouseSource)>("MouseSource","MouseSource");
		addField<DAS_BIND_MANAGED_FIELD(MouseHoveredViewport)>("MouseHoveredViewport","MouseHoveredViewport");
		addField<DAS_BIND_MANAGED_FIELD(KeyCtrl)>("KeyCtrl","KeyCtrl");
		addField<DAS_BIND_MANAGED_FIELD(KeyShift)>("KeyShift","KeyShift");
		addField<DAS_BIND_MANAGED_FIELD(KeyAlt)>("KeyAlt","KeyAlt");
		addField<DAS_BIND_MANAGED_FIELD(KeySuper)>("KeySuper","KeySuper");
		addField<DAS_BIND_MANAGED_FIELD(KeyMods)>("KeyMods","KeyMods");
		addField<DAS_BIND_MANAGED_FIELD(KeysData)>("KeysData","KeysData");
		addField<DAS_BIND_MANAGED_FIELD(WantCaptureMouseUnlessPopupClose)>("WantCaptureMouseUnlessPopupClose","WantCaptureMouseUnlessPopupClose");
		addField<DAS_BIND_MANAGED_FIELD(MousePosPrev)>("MousePosPrev","MousePosPrev");
		addField<DAS_BIND_MANAGED_FIELD(MouseClickedPos)>("MouseClickedPos","MouseClickedPos");
		addField<DAS_BIND_MANAGED_FIELD(MouseClickedTime)>("MouseClickedTime","MouseClickedTime");
		addField<DAS_BIND_MANAGED_FIELD(MouseClicked)>("MouseClicked","MouseClicked");
		addField<DAS_BIND_MANAGED_FIELD(MouseDoubleClicked)>("MouseDoubleClicked","MouseDoubleClicked");
		addField<DAS_BIND_MANAGED_FIELD(MouseClickedCount)>("MouseClickedCount","MouseClickedCount");
		addField<DAS_BIND_MANAGED_FIELD(MouseClickedLastCount)>("MouseClickedLastCount","MouseClickedLastCount");
		addField<DAS_BIND_MANAGED_FIELD(MouseReleased)>("MouseReleased","MouseReleased");
		addField<DAS_BIND_MANAGED_FIELD(MouseReleasedTime)>("MouseReleasedTime","MouseReleasedTime");
		addField<DAS_BIND_MANAGED_FIELD(MouseDownOwned)>("MouseDownOwned","MouseDownOwned");
		addField<DAS_BIND_MANAGED_FIELD(MouseDownOwnedUnlessPopupClose)>("MouseDownOwnedUnlessPopupClose","MouseDownOwnedUnlessPopupClose");
		addField<DAS_BIND_MANAGED_FIELD(MouseWheelRequestAxisSwap)>("MouseWheelRequestAxisSwap","MouseWheelRequestAxisSwap");
		addField<DAS_BIND_MANAGED_FIELD(MouseCtrlLeftAsRightClick)>("MouseCtrlLeftAsRightClick","MouseCtrlLeftAsRightClick");
		addField<DAS_BIND_MANAGED_FIELD(MouseDownDuration)>("MouseDownDuration","MouseDownDuration");
		addField<DAS_BIND_MANAGED_FIELD(MouseDownDurationPrev)>("MouseDownDurationPrev","MouseDownDurationPrev");
		addField<DAS_BIND_MANAGED_FIELD(MouseDragMaxDistanceAbs)>("MouseDragMaxDistanceAbs","MouseDragMaxDistanceAbs");
		addField<DAS_BIND_MANAGED_FIELD(MouseDragMaxDistanceSqr)>("MouseDragMaxDistanceSqr","MouseDragMaxDistanceSqr");
		addField<DAS_BIND_MANAGED_FIELD(PenPressure)>("PenPressure","PenPressure");
		addField<DAS_BIND_MANAGED_FIELD(AppFocusLost)>("AppFocusLost","AppFocusLost");
		addField<DAS_BIND_MANAGED_FIELD(AppAcceptingEvents)>("AppAcceptingEvents","AppAcceptingEvents");
		addField<DAS_BIND_MANAGED_FIELD(InputQueueSurrogate)>("InputQueueSurrogate","InputQueueSurrogate");
		addField<DAS_BIND_MANAGED_FIELD(InputQueueCharacters)>("InputQueueCharacters","InputQueueCharacters");
	}
};

// from imgui/imgui.h:2748:8
struct ImGuiInputTextCallbackData_GeneratedAnnotation : ManagedStructureAnnotation<ImGuiInputTextCallbackData> {
	ImGuiInputTextCallbackData_GeneratedAnnotation(ModuleLibrary & ml) : ManagedStructureAnnotation ("ImGuiInputTextCallbackData", ml, "ImGuiInputTextCallbackData") {
	}
	void init () {
		addField<DAS_BIND_MANAGED_FIELD(Ctx)>("Ctx","Ctx");
		addField<ImGuiInputTextFlags_,offsetof(ManagedType,EventFlag)>("EventFlag","EventFlag")
			.adjustAot("das_reinterpret<ImGuiInputTextFlags_>::pass(",")");
		addField<ImGuiInputTextFlags_,offsetof(ManagedType,Flags)>("Flags","Flags")
			.adjustAot("das_reinterpret<ImGuiInputTextFlags_>::pass(",")");
		addField<DAS_BIND_MANAGED_FIELD(UserData)>("UserData","UserData");
		addField<DAS_BIND_MANAGED_FIELD(ID)>("ID","ID");
		addField<DAS_BIND_MANAGED_FIELD(EventKey)>("EventKey","EventKey");
		addField<DAS_BIND_MANAGED_FIELD(EventChar)>("EventChar","EventChar");
		addField<DAS_BIND_MANAGED_FIELD(EventActivated)>("EventActivated","EventActivated");
		addField<DAS_BIND_MANAGED_FIELD(BufDirty)>("BufDirty","BufDirty");
		addField<DAS_BIND_MANAGED_FIELD(Buf)>("Buf","Buf");
		addField<DAS_BIND_MANAGED_FIELD(BufTextLen)>("BufTextLen","BufTextLen");
		addField<DAS_BIND_MANAGED_FIELD(BufSize)>("BufSize","BufSize");
		addField<DAS_BIND_MANAGED_FIELD(CursorPos)>("CursorPos","CursorPos");
		addField<DAS_BIND_MANAGED_FIELD(SelectionStart)>("SelectionStart","SelectionStart");
		addField<DAS_BIND_MANAGED_FIELD(SelectionEnd)>("SelectionEnd","SelectionEnd");
	}
};

// from imgui/imgui.h:2785:8
struct ImGuiSizeCallbackData_GeneratedAnnotation : ManagedStructureAnnotation<ImGuiSizeCallbackData> {
	ImGuiSizeCallbackData_GeneratedAnnotation(ModuleLibrary & ml) : ManagedStructureAnnotation ("ImGuiSizeCallbackData", ml, "ImGuiSizeCallbackData") {
	}
	void init () {
		addField<DAS_BIND_MANAGED_FIELD(UserData)>("UserData","UserData");
		addField<DAS_BIND_MANAGED_FIELD(Pos)>("Pos","Pos");
		addField<DAS_BIND_MANAGED_FIELD(CurrentSize)>("CurrentSize","CurrentSize");
		addField<DAS_BIND_MANAGED_FIELD(DesiredSize)>("DesiredSize","DesiredSize");
	}
};

// from imgui/imgui.h:2800:8
struct ImGuiWindowClass_GeneratedAnnotation : ManagedStructureAnnotation<ImGuiWindowClass> {
	ImGuiWindowClass_GeneratedAnnotation(ModuleLibrary & ml) : ManagedStructureAnnotation ("ImGuiWindowClass", ml, "ImGuiWindowClass") {
	}
	void init () {
		addField<DAS_BIND_MANAGED_FIELD(ClassId)>("ClassId","ClassId");
		addField<DAS_BIND_MANAGED_FIELD(ParentViewportId)>("ParentViewportId","ParentViewportId");
		addField<DAS_BIND_MANAGED_FIELD(FocusRouteParentWindowId)>("FocusRouteParentWindowId","FocusRouteParentWindowId");
		addField<ImGuiViewportFlags_,offsetof(ManagedType,ViewportFlagsOverrideSet)>("ViewportFlagsOverrideSet","ViewportFlagsOverrideSet")
			.adjustAot("das_reinterpret<ImGuiViewportFlags_>::pass(",")");
		addField<ImGuiViewportFlags_,offsetof(ManagedType,ViewportFlagsOverrideClear)>("ViewportFlagsOverrideClear","ViewportFlagsOverrideClear")
			.adjustAot("das_reinterpret<ImGuiViewportFlags_>::pass(",")");
		addField<ImGuiTabItemFlags_,offsetof(ManagedType,TabItemFlagsOverrideSet)>("TabItemFlagsOverrideSet","TabItemFlagsOverrideSet")
			.adjustAot("das_reinterpret<ImGuiTabItemFlags_>::pass(",")");
		addField<DAS_BIND_MANAGED_FIELD(DockNodeFlagsOverrideSet)>("DockNodeFlagsOverrideSet","DockNodeFlagsOverrideSet");
		addField<DAS_BIND_MANAGED_FIELD(DockingAlwaysTabBar)>("DockingAlwaysTabBar","DockingAlwaysTabBar");
		addField<DAS_BIND_MANAGED_FIELD(DockingAllowUnclassed)>("DockingAllowUnclassed","DockingAllowUnclassed");
	}
};

// from imgui/imgui.h:2816:8
struct ImGuiPayload_GeneratedAnnotation : ManagedStructureAnnotation<ImGuiPayload> {
	ImGuiPayload_GeneratedAnnotation(ModuleLibrary & ml) : ManagedStructureAnnotation ("ImGuiPayload", ml, "ImGuiPayload") {
	}
	void init () {
		addField<DAS_BIND_MANAGED_FIELD(Data)>("Data","Data");
		addField<DAS_BIND_MANAGED_FIELD(DataSize)>("DataSize","DataSize");
		addField<DAS_BIND_MANAGED_FIELD(SourceId)>("SourceId","SourceId");
		addField<DAS_BIND_MANAGED_FIELD(SourceParentId)>("SourceParentId","SourceParentId");
		addField<DAS_BIND_MANAGED_FIELD(DataFrameCount)>("DataFrameCount","DataFrameCount");
		addField<DAS_BIND_MANAGED_FIELD(DataType)>("DataType","DataType");
		addField<DAS_BIND_MANAGED_FIELD(Preview)>("Preview","Preview");
		addField<DAS_BIND_MANAGED_FIELD(Delivery)>("Delivery","Delivery");
	}
};

// from imgui/imgui.h:2851:8
struct ImGuiOnceUponAFrame_GeneratedAnnotation : ManagedStructureAnnotation<ImGuiOnceUponAFrame> {
	ImGuiOnceUponAFrame_GeneratedAnnotation(ModuleLibrary & ml) : ManagedStructureAnnotation ("ImGuiOnceUponAFrame", ml, "ImGuiOnceUponAFrame") {
	}
	void init () {
		addField<DAS_BIND_MANAGED_FIELD(RefFrame)>("RefFrame","RefFrame");
	}
};

// from imgui/imgui.h:2869:12
struct ImGuiTextFilter_ImGuiTextRange_GeneratedAnnotation : ManagedStructureAnnotation<ImGuiTextFilter::ImGuiTextRange> {
	ImGuiTextFilter_ImGuiTextRange_GeneratedAnnotation(ModuleLibrary & ml) : ManagedStructureAnnotation ("ImGuiTextRange", ml, "ImGuiTextFilter::ImGuiTextRange") {
	}
	void init () {
		addField<DAS_BIND_MANAGED_FIELD(b)>("b","b");
		addField<DAS_BIND_MANAGED_FIELD(e)>("e","e");
	}
};

// from imgui/imgui.h:2859:8
struct ImGuiTextFilter_GeneratedAnnotation : ManagedStructureAnnotation<ImGuiTextFilter> {
	ImGuiTextFilter_GeneratedAnnotation(ModuleLibrary & ml) : ManagedStructureAnnotation ("ImGuiTextFilter", ml, "ImGuiTextFilter") {
	}
	void init () {
		addField<DAS_BIND_MANAGED_FIELD(InputBuf)>("InputBuf","InputBuf");
		addField<DAS_BIND_MANAGED_FIELD(Filters)>("Filters","Filters");
		addField<DAS_BIND_MANAGED_FIELD(CountGrep)>("CountGrep","CountGrep");
	}
};

// from imgui/imgui.h:2886:8
struct ImGuiTextBuffer_GeneratedAnnotation : ManagedStructureAnnotation<ImGuiTextBuffer> {
	ImGuiTextBuffer_GeneratedAnnotation(ModuleLibrary & ml) : ManagedStructureAnnotation ("ImGuiTextBuffer", ml, "ImGuiTextBuffer") {
	}
	void init () {
		addField<DAS_BIND_MANAGED_FIELD(Buf)>("Buf","Buf");
	}
};

// from imgui/imgui.h:2907:8
struct ImGuiStoragePair_GeneratedAnnotation : ManagedStructureAnnotation<ImGuiStoragePair> {
	ImGuiStoragePair_GeneratedAnnotation(ModuleLibrary & ml) : ManagedStructureAnnotation ("ImGuiStoragePair", ml, "ImGuiStoragePair") {
	}
	void init () {
		addField<DAS_BIND_MANAGED_FIELD(key)>("key","key");
	}
};

// from imgui/imgui.h:2924:8
struct ImGuiStorage_GeneratedAnnotation : ManagedStructureAnnotation<ImGuiStorage> {
	ImGuiStorage_GeneratedAnnotation(ModuleLibrary & ml) : ManagedStructureAnnotation ("ImGuiStorage", ml, "ImGuiStorage") {
	}
	void init () {
		addField<DAS_BIND_MANAGED_FIELD(Data)>("Data","Data");
	}
};

// from imgui/imgui.h:2988:8
struct ImGuiListClipper_GeneratedAnnotation : ManagedStructureAnnotation<ImGuiListClipper> {
	ImGuiListClipper_GeneratedAnnotation(ModuleLibrary & ml) : ManagedStructureAnnotation ("ImGuiListClipper", ml, "ImGuiListClipper") {
	}
	void init () {
		addField<DAS_BIND_MANAGED_FIELD(Ctx)>("Ctx","Ctx");
		addField<DAS_BIND_MANAGED_FIELD(DisplayStart)>("DisplayStart","DisplayStart");
		addField<DAS_BIND_MANAGED_FIELD(DisplayEnd)>("DisplayEnd","DisplayEnd");
		addField<DAS_BIND_MANAGED_FIELD(ItemsCount)>("ItemsCount","ItemsCount");
		addField<DAS_BIND_MANAGED_FIELD(ItemsHeight)>("ItemsHeight","ItemsHeight");
		addField<DAS_BIND_MANAGED_FIELD(StartPosY)>("StartPosY","StartPosY");
		addField<DAS_BIND_MANAGED_FIELD(StartSeekOffsetY)>("StartSeekOffsetY","StartSeekOffsetY");
		addField<DAS_BIND_MANAGED_FIELD(TempData)>("TempData","TempData");
		addField<DAS_BIND_MANAGED_FIELD(Flags)>("Flags","Flags");
	}
};

// from imgui/imgui.h:3175:8
struct ImGuiMultiSelectIO_GeneratedAnnotation : ManagedStructureAnnotation<ImGuiMultiSelectIO> {
	ImGuiMultiSelectIO_GeneratedAnnotation(ModuleLibrary & ml) : ManagedStructureAnnotation ("ImGuiMultiSelectIO", ml, "ImGuiMultiSelectIO") {
	}
	void init () {
		addField<DAS_BIND_MANAGED_FIELD(Requests)>("Requests","Requests");
		addField<DAS_BIND_MANAGED_FIELD(RangeSrcItem)>("RangeSrcItem","RangeSrcItem");
		addField<DAS_BIND_MANAGED_FIELD(NavIdItem)>("NavIdItem","NavIdItem");
		addField<DAS_BIND_MANAGED_FIELD(NavIdSelected)>("NavIdSelected","NavIdSelected");
		addField<DAS_BIND_MANAGED_FIELD(RangeSrcReset)>("RangeSrcReset","RangeSrcReset");
		addField<DAS_BIND_MANAGED_FIELD(ItemsCount)>("ItemsCount","ItemsCount");
	}
};

// from imgui/imgui.h:3195:8
struct ImGuiSelectionRequest_GeneratedAnnotation : ManagedStructureAnnotation<ImGuiSelectionRequest> {
	ImGuiSelectionRequest_GeneratedAnnotation(ModuleLibrary & ml) : ManagedStructureAnnotation ("ImGuiSelectionRequest", ml, "ImGuiSelectionRequest") {
	}
	void init () {
		addField<DAS_BIND_MANAGED_FIELD(Type)>("Type","Type");
		addField<DAS_BIND_MANAGED_FIELD(Selected)>("Selected","Selected");
		addField<DAS_BIND_MANAGED_FIELD(RangeDirection)>("RangeDirection","RangeDirection");
		addField<DAS_BIND_MANAGED_FIELD(RangeFirstItem)>("RangeFirstItem","RangeFirstItem");
		addField<DAS_BIND_MANAGED_FIELD(RangeLastItem)>("RangeLastItem","RangeLastItem");
	}
};

// from imgui/imgui.h:3221:8
struct ImGuiSelectionBasicStorage_GeneratedAnnotation : ManagedStructureAnnotation<ImGuiSelectionBasicStorage> {
	ImGuiSelectionBasicStorage_GeneratedAnnotation(ModuleLibrary & ml) : ManagedStructureAnnotation ("ImGuiSelectionBasicStorage", ml, "ImGuiSelectionBasicStorage") {
	}
	void init () {
		addField<DAS_BIND_MANAGED_FIELD(Size)>("Size","Size");
		addField<DAS_BIND_MANAGED_FIELD(PreserveOrder)>("PreserveOrder","PreserveOrder");
		addField<DAS_BIND_MANAGED_FIELD(UserData)>("UserData","UserData");
		addField<DAS_BIND_MANAGED_FIELD(_SelectionOrder)>("_SelectionOrder","_SelectionOrder");
		addField<DAS_BIND_MANAGED_FIELD(_Storage)>("_Storage","_Storage");
	}
};

// from imgui/imgui.h:3244:8
struct ImGuiSelectionExternalStorage_GeneratedAnnotation : ManagedStructureAnnotation<ImGuiSelectionExternalStorage> {
	ImGuiSelectionExternalStorage_GeneratedAnnotation(ModuleLibrary & ml) : ManagedStructureAnnotation ("ImGuiSelectionExternalStorage", ml, "ImGuiSelectionExternalStorage") {
	}
	void init () {
		addField<DAS_BIND_MANAGED_FIELD(UserData)>("UserData","UserData");
	}
};

// from imgui/imgui.h:3294:8
struct ImDrawCmd_GeneratedAnnotation : ManagedStructureAnnotation<ImDrawCmd> {
	ImDrawCmd_GeneratedAnnotation(ModuleLibrary & ml) : ManagedStructureAnnotation ("ImDrawCmd", ml, "ImDrawCmd") {
	}
	void init () {
		addField<DAS_BIND_MANAGED_FIELD(ClipRect)>("ClipRect","ClipRect");
		addField<DAS_BIND_MANAGED_FIELD(TexRef)>("TexRef","TexRef");
		addField<DAS_BIND_MANAGED_FIELD(VtxOffset)>("VtxOffset","VtxOffset");
		addField<DAS_BIND_MANAGED_FIELD(IdxOffset)>("IdxOffset","IdxOffset");
		addField<DAS_BIND_MANAGED_FIELD(ElemCount)>("ElemCount","ElemCount");
		addField<DAS_BIND_MANAGED_FIELD(UserCallbackData)>("UserCallbackData","UserCallbackData");
		addField<DAS_BIND_MANAGED_FIELD(UserCallbackDataSize)>("UserCallbackDataSize","UserCallbackDataSize");
		addField<DAS_BIND_MANAGED_FIELD(UserCallbackDataOffset)>("UserCallbackDataOffset","UserCallbackDataOffset");
	}
};

// from imgui/imgui.h:3315:8
struct ImDrawVert_GeneratedAnnotation : ManagedStructureAnnotation<ImDrawVert> {
	ImDrawVert_GeneratedAnnotation(ModuleLibrary & ml) : ManagedStructureAnnotation ("ImDrawVert", ml, "ImDrawVert") {
	}
	void init () {
		addField<DAS_BIND_MANAGED_FIELD(pos)>("pos","pos");
		addField<DAS_BIND_MANAGED_FIELD(uv)>("uv","uv");
		addField<DAS_BIND_MANAGED_FIELD(col)>("col","col");
	}
};

// from imgui/imgui.h:3330:8
struct ImDrawCmdHeader_GeneratedAnnotation : ManagedStructureAnnotation<ImDrawCmdHeader> {
	ImDrawCmdHeader_GeneratedAnnotation(ModuleLibrary & ml) : ManagedStructureAnnotation ("ImDrawCmdHeader", ml, "ImDrawCmdHeader") {
	}
	void init () {
		addField<DAS_BIND_MANAGED_FIELD(ClipRect)>("ClipRect","ClipRect");
		addField<DAS_BIND_MANAGED_FIELD(TexRef)>("TexRef","TexRef");
		addField<DAS_BIND_MANAGED_FIELD(VtxOffset)>("VtxOffset","VtxOffset");
	}
};

// from imgui/imgui.h:3338:8
struct ImDrawChannel_GeneratedAnnotation : ManagedStructureAnnotation<ImDrawChannel> {
	ImDrawChannel_GeneratedAnnotation(ModuleLibrary & ml) : ManagedStructureAnnotation ("ImDrawChannel", ml, "ImDrawChannel") {
	}
	void init () {
		addField<DAS_BIND_MANAGED_FIELD(_CmdBuffer)>("_CmdBuffer","_CmdBuffer");
		addField<DAS_BIND_MANAGED_FIELD(_IdxBuffer)>("_IdxBuffer","_IdxBuffer");
	}
};

// from imgui/imgui.h:3346:8
struct ImDrawListSplitter_GeneratedAnnotation : ManagedStructureAnnotation<ImDrawListSplitter> {
	ImDrawListSplitter_GeneratedAnnotation(ModuleLibrary & ml) : ManagedStructureAnnotation ("ImDrawListSplitter", ml, "ImDrawListSplitter") {
	}
	void init () {
		addField<DAS_BIND_MANAGED_FIELD(_Current)>("_Current","_Current");
		addField<DAS_BIND_MANAGED_FIELD(_Count)>("_Count","_Count");
		addField<DAS_BIND_MANAGED_FIELD(_Channels)>("_Channels","_Channels");
	}
};

// from imgui/imgui.h:3401:8
struct ImDrawList_GeneratedAnnotation : ManagedStructureAnnotation<ImDrawList> {
	ImDrawList_GeneratedAnnotation(ModuleLibrary & ml) : ManagedStructureAnnotation ("ImDrawList", ml, "ImDrawList") {
	}
	void init () {
		addField<DAS_BIND_MANAGED_FIELD(CmdBuffer)>("CmdBuffer","CmdBuffer");
		addField<DAS_BIND_MANAGED_FIELD(IdxBuffer)>("IdxBuffer","IdxBuffer");
		addField<DAS_BIND_MANAGED_FIELD(VtxBuffer)>("VtxBuffer","VtxBuffer");
		addField<ImDrawListFlags_,offsetof(ManagedType,Flags)>("Flags","Flags")
			.adjustAot("das_reinterpret<ImDrawListFlags_>::pass(",")");
		addField<DAS_BIND_MANAGED_FIELD(_VtxCurrentIdx)>("_VtxCurrentIdx","_VtxCurrentIdx");
		addField<DAS_BIND_MANAGED_FIELD(_Data)>("_Data","_Data");
		addField<DAS_BIND_MANAGED_FIELD(_VtxWritePtr)>("_VtxWritePtr","_VtxWritePtr");
		addField<DAS_BIND_MANAGED_FIELD(_IdxWritePtr)>("_IdxWritePtr","_IdxWritePtr");
		addField<DAS_BIND_MANAGED_FIELD(_Path)>("_Path","_Path");
		addField<DAS_BIND_MANAGED_FIELD(_CmdHeader)>("_CmdHeader","_CmdHeader");
		addField<DAS_BIND_MANAGED_FIELD(_Splitter)>("_Splitter","_Splitter");
		addField<DAS_BIND_MANAGED_FIELD(_ClipRectStack)>("_ClipRectStack","_ClipRectStack");
		addField<DAS_BIND_MANAGED_FIELD(_TextureStack)>("_TextureStack","_TextureStack");
		addField<DAS_BIND_MANAGED_FIELD(_CallbacksDataBuf)>("_CallbacksDataBuf","_CallbacksDataBuf");
		addField<DAS_BIND_MANAGED_FIELD(_FringeScale)>("_FringeScale","_FringeScale");
		addField<DAS_BIND_MANAGED_FIELD(_OwnerName)>("_OwnerName","_OwnerName");
	}
};

// from imgui/imgui.h:3559:8
struct ImDrawData_GeneratedAnnotation : ManagedStructureAnnotation<ImDrawData> {
	ImDrawData_GeneratedAnnotation(ModuleLibrary & ml) : ManagedStructureAnnotation ("ImDrawData", ml, "ImDrawData") {
	}
	void init () {
		addField<DAS_BIND_MANAGED_FIELD(Valid)>("Valid","Valid");
		addField<DAS_BIND_MANAGED_FIELD(CmdListsCount)>("CmdListsCount","CmdListsCount");
		addField<DAS_BIND_MANAGED_FIELD(TotalIdxCount)>("TotalIdxCount","TotalIdxCount");
		addField<DAS_BIND_MANAGED_FIELD(TotalVtxCount)>("TotalVtxCount","TotalVtxCount");
		addField<DAS_BIND_MANAGED_FIELD(CmdLists)>("CmdLists","CmdLists");
		addField<DAS_BIND_MANAGED_FIELD(DisplayPos)>("DisplayPos","DisplayPos");
		addField<DAS_BIND_MANAGED_FIELD(DisplaySize)>("DisplaySize","DisplaySize");
		addField<DAS_BIND_MANAGED_FIELD(FramebufferScale)>("FramebufferScale","FramebufferScale");
		addField<DAS_BIND_MANAGED_FIELD(OwnerViewport)>("OwnerViewport","OwnerViewport");
		addField<DAS_BIND_MANAGED_FIELD(Textures)>("Textures","Textures");
	}
};

// from imgui/imgui.h:3611:8
struct ImTextureRect_GeneratedAnnotation : ManagedStructureAnnotation<ImTextureRect> {
	ImTextureRect_GeneratedAnnotation(ModuleLibrary & ml) : ManagedStructureAnnotation ("ImTextureRect", ml, "ImTextureRect") {
	}
	void init () {
		addField<DAS_BIND_MANAGED_FIELD(x)>("x","x");
		addField<DAS_BIND_MANAGED_FIELD(y)>("y","y");
		addField<DAS_BIND_MANAGED_FIELD(w)>("w","w");
		addField<DAS_BIND_MANAGED_FIELD(h)>("h","h");
	}
};

// from imgui/imgui.h:3624:8
struct ImTextureData_GeneratedAnnotation : ManagedStructureAnnotation<ImTextureData> {
	ImTextureData_GeneratedAnnotation(ModuleLibrary & ml) : ManagedStructureAnnotation ("ImTextureData", ml, "ImTextureData") {
	}
	void init () {
		addField<DAS_BIND_MANAGED_FIELD(UniqueID)>("UniqueID","UniqueID");
		addField<DAS_BIND_MANAGED_FIELD(Status)>("Status","Status");
		addField<DAS_BIND_MANAGED_FIELD(BackendUserData)>("BackendUserData","BackendUserData");
		addField<DAS_BIND_MANAGED_FIELD(TexID)>("TexID","TexID");
		addField<DAS_BIND_MANAGED_FIELD(Format)>("Format","Format");
		addField<DAS_BIND_MANAGED_FIELD(Width)>("Width","Width");
		addField<DAS_BIND_MANAGED_FIELD(Height)>("Height","Height");
		addField<DAS_BIND_MANAGED_FIELD(BytesPerPixel)>("BytesPerPixel","BytesPerPixel");
		addField<DAS_BIND_MANAGED_FIELD(Pixels)>("Pixels","Pixels");
		addField<DAS_BIND_MANAGED_FIELD(UsedRect)>("UsedRect","UsedRect");
		addField<DAS_BIND_MANAGED_FIELD(UpdateRect)>("UpdateRect","UpdateRect");
		addField<DAS_BIND_MANAGED_FIELD(Updates)>("Updates","Updates");
		addField<DAS_BIND_MANAGED_FIELD(UnusedFrames)>("UnusedFrames","UnusedFrames");
		addField<DAS_BIND_MANAGED_FIELD(RefCount)>("RefCount","RefCount");
		addField<DAS_BIND_MANAGED_FIELD(UseColors)>("UseColors","UseColors");
		addField<DAS_BIND_MANAGED_FIELD(WantDestroyNextFrame)>("WantDestroyNextFrame","WantDestroyNextFrame");
	}
};

// from imgui/imgui.h:3668:8
struct ImFontConfig_GeneratedAnnotation : ManagedStructureAnnotation<ImFontConfig> {
	ImFontConfig_GeneratedAnnotation(ModuleLibrary & ml) : ManagedStructureAnnotation ("ImFontConfig", ml, "ImFontConfig") {
	}
	void init () {
		addField<DAS_BIND_MANAGED_FIELD(Name)>("Name","Name");
		addField<DAS_BIND_MANAGED_FIELD(FontData)>("FontData","FontData");
		addField<DAS_BIND_MANAGED_FIELD(FontDataSize)>("FontDataSize","FontDataSize");
		addField<DAS_BIND_MANAGED_FIELD(FontDataOwnedByAtlas)>("FontDataOwnedByAtlas","FontDataOwnedByAtlas");
		addField<DAS_BIND_MANAGED_FIELD(MergeMode)>("MergeMode","MergeMode");
		addField<DAS_BIND_MANAGED_FIELD(PixelSnapH)>("PixelSnapH","PixelSnapH");
		addField<DAS_BIND_MANAGED_FIELD(OversampleH)>("OversampleH","OversampleH");
		addField<DAS_BIND_MANAGED_FIELD(OversampleV)>("OversampleV","OversampleV");
		addField<DAS_BIND_MANAGED_FIELD(EllipsisChar)>("EllipsisChar","EllipsisChar");
		addField<DAS_BIND_MANAGED_FIELD(SizePixels)>("SizePixels","SizePixels");
		addField<DAS_BIND_MANAGED_FIELD(GlyphRanges)>("GlyphRanges","GlyphRanges");
		addField<DAS_BIND_MANAGED_FIELD(GlyphExcludeRanges)>("GlyphExcludeRanges","GlyphExcludeRanges");
		addField<DAS_BIND_MANAGED_FIELD(GlyphOffset)>("GlyphOffset","GlyphOffset");
		addField<DAS_BIND_MANAGED_FIELD(GlyphMinAdvanceX)>("GlyphMinAdvanceX","GlyphMinAdvanceX");
		addField<DAS_BIND_MANAGED_FIELD(GlyphMaxAdvanceX)>("GlyphMaxAdvanceX","GlyphMaxAdvanceX");
		addField<DAS_BIND_MANAGED_FIELD(GlyphExtraAdvanceX)>("GlyphExtraAdvanceX","GlyphExtraAdvanceX");
		addField<DAS_BIND_MANAGED_FIELD(FontNo)>("FontNo","FontNo");
		addField<DAS_BIND_MANAGED_FIELD(FontLoaderFlags)>("FontLoaderFlags","FontLoaderFlags");
		addField<DAS_BIND_MANAGED_FIELD(RasterizerMultiply)>("RasterizerMultiply","RasterizerMultiply");
		addField<DAS_BIND_MANAGED_FIELD(RasterizerDensity)>("RasterizerDensity","RasterizerDensity");
		addField<DAS_BIND_MANAGED_FIELD(ExtraSizeScale)>("ExtraSizeScale","ExtraSizeScale");
		addField<DAS_BIND_MANAGED_FIELD(Flags)>("Flags","Flags");
		addField<DAS_BIND_MANAGED_FIELD(DstFont)>("DstFont","DstFont");
		addField<DAS_BIND_MANAGED_FIELD(FontLoader)>("FontLoader","FontLoader");
		addField<DAS_BIND_MANAGED_FIELD(FontLoaderData)>("FontLoaderData","FontLoaderData");
	}
};

// from imgui/imgui.h:3711:8
struct ImFontGlyph_GeneratedAnnotation : ManagedStructureAnnotation<ImFontGlyph> {
	ImFontGlyph_GeneratedAnnotation(ModuleLibrary & ml) : ManagedStructureAnnotation ("ImFontGlyph", ml, "ImFontGlyph") {
	}
	void init () {
		addField<DAS_BIND_MANAGED_FIELD(AdvanceX)>("AdvanceX","AdvanceX");
		addField<DAS_BIND_MANAGED_FIELD(X0)>("X0","X0");
		addField<DAS_BIND_MANAGED_FIELD(Y0)>("Y0","Y0");
		addField<DAS_BIND_MANAGED_FIELD(X1)>("X1","X1");
		addField<DAS_BIND_MANAGED_FIELD(Y1)>("Y1","Y1");
		addField<DAS_BIND_MANAGED_FIELD(U0)>("U0","U0");
		addField<DAS_BIND_MANAGED_FIELD(V0)>("V0","V0");
		addField<DAS_BIND_MANAGED_FIELD(U1)>("U1","U1");
		addField<DAS_BIND_MANAGED_FIELD(V1)>("V1","V1");
		addField<DAS_BIND_MANAGED_FIELD(PackId)>("PackId","PackId");
	}
};

// from imgui/imgui.h:3727:8
struct ImFontGlyphRangesBuilder_GeneratedAnnotation : ManagedStructureAnnotation<ImFontGlyphRangesBuilder> {
	ImFontGlyphRangesBuilder_GeneratedAnnotation(ModuleLibrary & ml) : ManagedStructureAnnotation ("ImFontGlyphRangesBuilder", ml, "ImFontGlyphRangesBuilder") {
	}
	void init () {
		addField<DAS_BIND_MANAGED_FIELD(UsedChars)>("UsedChars","UsedChars");
	}
};

// from imgui/imgui.h:3749:8
struct ImFontAtlasRect_GeneratedAnnotation : ManagedStructureAnnotation<ImFontAtlasRect> {
	ImFontAtlasRect_GeneratedAnnotation(ModuleLibrary & ml) : ManagedStructureAnnotation ("ImFontAtlasRect", ml, "ImFontAtlasRect") {
	}
	void init () {
		addField<DAS_BIND_MANAGED_FIELD(x)>("x","x");
		addField<DAS_BIND_MANAGED_FIELD(y)>("y","y");
		addField<DAS_BIND_MANAGED_FIELD(w)>("w","w");
		addField<DAS_BIND_MANAGED_FIELD(h)>("h","h");
		addField<DAS_BIND_MANAGED_FIELD(uv0)>("uv0","uv0");
		addField<DAS_BIND_MANAGED_FIELD(uv1)>("uv1","uv1");
	}
};

// from imgui/imgui.h:3786:8
struct ImFontAtlas_GeneratedAnnotation : ManagedStructureAnnotation<ImFontAtlas> {
	ImFontAtlas_GeneratedAnnotation(ModuleLibrary & ml) : ManagedStructureAnnotation ("ImFontAtlas", ml, "ImFontAtlas") {
	}
	void init () {
		addField<ImFontAtlasFlags_,offsetof(ManagedType,Flags)>("Flags","Flags")
			.adjustAot("das_reinterpret<ImFontAtlasFlags_>::pass(",")");
		addField<DAS_BIND_MANAGED_FIELD(TexDesiredFormat)>("TexDesiredFormat","TexDesiredFormat");
		addField<DAS_BIND_MANAGED_FIELD(TexGlyphPadding)>("TexGlyphPadding","TexGlyphPadding");
		addField<DAS_BIND_MANAGED_FIELD(TexMinWidth)>("TexMinWidth","TexMinWidth");
		addField<DAS_BIND_MANAGED_FIELD(TexMinHeight)>("TexMinHeight","TexMinHeight");
		addField<DAS_BIND_MANAGED_FIELD(TexMaxWidth)>("TexMaxWidth","TexMaxWidth");
		addField<DAS_BIND_MANAGED_FIELD(TexMaxHeight)>("TexMaxHeight","TexMaxHeight");
		addField<DAS_BIND_MANAGED_FIELD(UserData)>("UserData","UserData");
		addField<DAS_BIND_MANAGED_FIELD(TexRef)>("TexRef","TexRef");
		addField<DAS_BIND_MANAGED_FIELD(TexData)>("TexData","TexData");
		addField<DAS_BIND_MANAGED_FIELD(TexList)>("TexList","TexList");
		addField<DAS_BIND_MANAGED_FIELD(Locked)>("Locked","Locked");
		addField<DAS_BIND_MANAGED_FIELD(RendererHasTextures)>("RendererHasTextures","RendererHasTextures");
		addField<DAS_BIND_MANAGED_FIELD(TexIsBuilt)>("TexIsBuilt","TexIsBuilt");
		addField<DAS_BIND_MANAGED_FIELD(TexPixelsUseColors)>("TexPixelsUseColors","TexPixelsUseColors");
		addField<DAS_BIND_MANAGED_FIELD(TexUvScale)>("TexUvScale","TexUvScale");
		addField<DAS_BIND_MANAGED_FIELD(TexUvWhitePixel)>("TexUvWhitePixel","TexUvWhitePixel");
		addField<DAS_BIND_MANAGED_FIELD(Fonts)>("Fonts","Fonts");
		addField<DAS_BIND_MANAGED_FIELD(Sources)>("Sources","Sources");
		addField<DAS_BIND_MANAGED_FIELD(TexUvLines)>("TexUvLines","TexUvLines");
		addField<DAS_BIND_MANAGED_FIELD(TexNextUniqueID)>("TexNextUniqueID","TexNextUniqueID");
		addField<DAS_BIND_MANAGED_FIELD(FontNextUniqueID)>("FontNextUniqueID","FontNextUniqueID");
		addField<DAS_BIND_MANAGED_FIELD(DrawListSharedDatas)>("DrawListSharedDatas","DrawListSharedDatas");
		addField<DAS_BIND_MANAGED_FIELD(Builder)>("Builder","Builder");
		addField<DAS_BIND_MANAGED_FIELD(FontLoader)>("FontLoader","FontLoader");
		addField<DAS_BIND_MANAGED_FIELD(FontLoaderName)>("FontLoaderName","FontLoaderName");
		addField<DAS_BIND_MANAGED_FIELD(FontLoaderData)>("FontLoaderData","FontLoaderData");
		addField<DAS_BIND_MANAGED_FIELD(FontLoaderFlags)>("FontLoaderFlags","FontLoaderFlags");
		addField<DAS_BIND_MANAGED_FIELD(RefCount)>("RefCount","RefCount");
		addField<DAS_BIND_MANAGED_FIELD(OwnerContext)>("OwnerContext","OwnerContext");
	}
};

// from imgui/imgui.h:3937:8
struct ImFontBaked_GeneratedAnnotation : ManagedStructureAnnotation<ImFontBaked> {
	ImFontBaked_GeneratedAnnotation(ModuleLibrary & ml) : ManagedStructureAnnotation ("ImFontBaked", ml, "ImFontBaked") {
	}
	void init () {
		addField<DAS_BIND_MANAGED_FIELD(IndexAdvanceX)>("IndexAdvanceX","IndexAdvanceX");
		addField<DAS_BIND_MANAGED_FIELD(FallbackAdvanceX)>("FallbackAdvanceX","FallbackAdvanceX");
		addField<DAS_BIND_MANAGED_FIELD(Size)>("Size","Size");
		addField<DAS_BIND_MANAGED_FIELD(RasterizerDensity)>("RasterizerDensity","RasterizerDensity");
		addField<DAS_BIND_MANAGED_FIELD(IndexLookup)>("IndexLookup","IndexLookup");
		addField<DAS_BIND_MANAGED_FIELD(Glyphs)>("Glyphs","Glyphs");
		addField<DAS_BIND_MANAGED_FIELD(FallbackGlyphIndex)>("FallbackGlyphIndex","FallbackGlyphIndex");
		addField<DAS_BIND_MANAGED_FIELD(Ascent)>("Ascent","Ascent");
		addField<DAS_BIND_MANAGED_FIELD(Descent)>("Descent","Descent");
		addField<DAS_BIND_MANAGED_FIELD(LastUsedFrame)>("LastUsedFrame","LastUsedFrame");
		addField<DAS_BIND_MANAGED_FIELD(BakedId)>("BakedId","BakedId");
		addField<DAS_BIND_MANAGED_FIELD(OwnerFont)>("OwnerFont","OwnerFont");
		addField<DAS_BIND_MANAGED_FIELD(FontLoaderDatas)>("FontLoaderDatas","FontLoaderDatas");
	}
};

// from imgui/imgui.h:3985:8
struct ImFont_GeneratedAnnotation : ManagedStructureAnnotation<ImFont> {
	ImFont_GeneratedAnnotation(ModuleLibrary & ml) : ManagedStructureAnnotation ("ImFont", ml, "ImFont") {
	}
	void init () {
		addField<DAS_BIND_MANAGED_FIELD(LastBaked)>("LastBaked","LastBaked");
		addField<DAS_BIND_MANAGED_FIELD(OwnerAtlas)>("OwnerAtlas","OwnerAtlas");
		addField<DAS_BIND_MANAGED_FIELD(Flags)>("Flags","Flags");
		addField<DAS_BIND_MANAGED_FIELD(CurrentRasterizerDensity)>("CurrentRasterizerDensity","CurrentRasterizerDensity");
		addField<DAS_BIND_MANAGED_FIELD(FontId)>("FontId","FontId");
		addField<DAS_BIND_MANAGED_FIELD(LegacySize)>("LegacySize","LegacySize");
		addField<DAS_BIND_MANAGED_FIELD(Sources)>("Sources","Sources");
		addField<DAS_BIND_MANAGED_FIELD(EllipsisChar)>("EllipsisChar","EllipsisChar");
		addField<DAS_BIND_MANAGED_FIELD(FallbackChar)>("FallbackChar","FallbackChar");
		addField<DAS_BIND_MANAGED_FIELD(Used8kPagesMap)>("Used8kPagesMap","Used8kPagesMap");
		addField<DAS_BIND_MANAGED_FIELD(EllipsisAutoBake)>("EllipsisAutoBake","EllipsisAutoBake");
		addField<DAS_BIND_MANAGED_FIELD(RemapPairs)>("RemapPairs","RemapPairs");
	}
};

// from imgui/imgui.h:4083:8
struct ImGuiViewport_GeneratedAnnotation : ManagedStructureAnnotation<ImGuiViewport> {
	ImGuiViewport_GeneratedAnnotation(ModuleLibrary & ml) : ManagedStructureAnnotation ("ImGuiViewport", ml, "ImGuiViewport") {
	}
	void init () {
		addField<DAS_BIND_MANAGED_FIELD(ID)>("ID","ID");
		addField<ImGuiViewportFlags_,offsetof(ManagedType,Flags)>("Flags","Flags")
			.adjustAot("das_reinterpret<ImGuiViewportFlags_>::pass(",")");
		addField<DAS_BIND_MANAGED_FIELD(Pos)>("Pos","Pos");
		addField<DAS_BIND_MANAGED_FIELD(Size)>("Size","Size");
		addField<DAS_BIND_MANAGED_FIELD(FramebufferScale)>("FramebufferScale","FramebufferScale");
		addField<DAS_BIND_MANAGED_FIELD(WorkPos)>("WorkPos","WorkPos");
		addField<DAS_BIND_MANAGED_FIELD(WorkSize)>("WorkSize","WorkSize");
		addField<DAS_BIND_MANAGED_FIELD(DpiScale)>("DpiScale","DpiScale");
		addField<DAS_BIND_MANAGED_FIELD(ParentViewportId)>("ParentViewportId","ParentViewportId");
		addField<DAS_BIND_MANAGED_FIELD(ParentViewport)>("ParentViewport","ParentViewport");
		addField<DAS_BIND_MANAGED_FIELD(DrawData)>("DrawData","DrawData");
		addField<DAS_BIND_MANAGED_FIELD(RendererUserData)>("RendererUserData","RendererUserData");
		addField<DAS_BIND_MANAGED_FIELD(PlatformUserData)>("PlatformUserData","PlatformUserData");
		addField<DAS_BIND_MANAGED_FIELD(PlatformHandle)>("PlatformHandle","PlatformHandle");
		addField<DAS_BIND_MANAGED_FIELD(PlatformHandleRaw)>("PlatformHandleRaw","PlatformHandleRaw");
		addField<DAS_BIND_MANAGED_FIELD(PlatformWindowCreated)>("PlatformWindowCreated","PlatformWindowCreated");
		addField<DAS_BIND_MANAGED_FIELD(PlatformRequestMove)>("PlatformRequestMove","PlatformRequestMove");
		addField<DAS_BIND_MANAGED_FIELD(PlatformRequestResize)>("PlatformRequestResize","PlatformRequestResize");
		addField<DAS_BIND_MANAGED_FIELD(PlatformRequestClose)>("PlatformRequestClose","PlatformRequestClose");
	}
};

// from imgui/imgui.h:4170:8
struct ImGuiPlatformIO_GeneratedAnnotation : ManagedStructureAnnotation<ImGuiPlatformIO> {
	ImGuiPlatformIO_GeneratedAnnotation(ModuleLibrary & ml) : ManagedStructureAnnotation ("ImGuiPlatformIO", ml, "ImGuiPlatformIO") {
	}
	void init () {
		addField<DAS_BIND_MANAGED_FIELD(Platform_ClipboardUserData)>("Platform_ClipboardUserData","Platform_ClipboardUserData");
		addField<DAS_BIND_MANAGED_FIELD(Platform_OpenInShellUserData)>("Platform_OpenInShellUserData","Platform_OpenInShellUserData");
		addField<DAS_BIND_MANAGED_FIELD(Platform_ImeUserData)>("Platform_ImeUserData","Platform_ImeUserData");
		addField<DAS_BIND_MANAGED_FIELD(Platform_LocaleDecimalPoint)>("Platform_LocaleDecimalPoint","Platform_LocaleDecimalPoint");
		addField<DAS_BIND_MANAGED_FIELD(Renderer_TextureMaxWidth)>("Renderer_TextureMaxWidth","Renderer_TextureMaxWidth");
		addField<DAS_BIND_MANAGED_FIELD(Renderer_TextureMaxHeight)>("Renderer_TextureMaxHeight","Renderer_TextureMaxHeight");
		addField<DAS_BIND_MANAGED_FIELD(Renderer_RenderState)>("Renderer_RenderState","Renderer_RenderState");
		addField<DAS_BIND_MANAGED_FIELD(Monitors)>("Monitors","Monitors");
		addField<DAS_BIND_MANAGED_FIELD(Textures)>("Textures","Textures");
		addField<DAS_BIND_MANAGED_FIELD(Viewports)>("Viewports","Viewports");
	}
};

// from imgui/imgui.h:4282:8
struct ImGuiPlatformMonitor_GeneratedAnnotation : ManagedStructureAnnotation<ImGuiPlatformMonitor> {
	ImGuiPlatformMonitor_GeneratedAnnotation(ModuleLibrary & ml) : ManagedStructureAnnotation ("ImGuiPlatformMonitor", ml, "ImGuiPlatformMonitor") {
	}
	void init () {
		addField<DAS_BIND_MANAGED_FIELD(MainPos)>("MainPos","MainPos");
		addField<DAS_BIND_MANAGED_FIELD(MainSize)>("MainSize","MainSize");
		addField<DAS_BIND_MANAGED_FIELD(WorkPos)>("WorkPos","WorkPos");
		addField<DAS_BIND_MANAGED_FIELD(WorkSize)>("WorkSize","WorkSize");
		addField<DAS_BIND_MANAGED_FIELD(DpiScale)>("DpiScale","DpiScale");
		addField<DAS_BIND_MANAGED_FIELD(PlatformHandle)>("PlatformHandle","PlatformHandle");
	}
};

// from imgui/imgui.h:4292:8
struct ImGuiPlatformImeData_GeneratedAnnotation : ManagedStructureAnnotation<ImGuiPlatformImeData> {
	ImGuiPlatformImeData_GeneratedAnnotation(ModuleLibrary & ml) : ManagedStructureAnnotation ("ImGuiPlatformImeData", ml, "ImGuiPlatformImeData") {
	}
	void init () {
		addField<DAS_BIND_MANAGED_FIELD(WantVisible)>("WantVisible","WantVisible");
		addField<DAS_BIND_MANAGED_FIELD(WantTextInput)>("WantTextInput","WantTextInput");
		addField<DAS_BIND_MANAGED_FIELD(InputPos)>("InputPos","InputPos");
		addField<DAS_BIND_MANAGED_FIELD(InputLineHeight)>("InputLineHeight","InputLineHeight");
		addField<DAS_BIND_MANAGED_FIELD(ViewportId)>("ViewportId","ViewportId");
	}
};

