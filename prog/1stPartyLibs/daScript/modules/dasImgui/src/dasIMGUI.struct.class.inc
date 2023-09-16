// this file is generated via daScript automatic C++ binder
// all user modifications will be lost after this file is re-generated

struct ImNewWrapper_GeneratedAnnotation : ManagedStructureAnnotation<ImNewWrapper> {
	ImNewWrapper_GeneratedAnnotation(ModuleLibrary & ml) : ManagedStructureAnnotation ("ImNewWrapper", ml, "ImNewWrapper") {
	}
	void init () {
	}
};

struct ImGuiStyle_GeneratedAnnotation : ManagedStructureAnnotation<ImGuiStyle> {
	ImGuiStyle_GeneratedAnnotation(ModuleLibrary & ml) : ManagedStructureAnnotation ("ImGuiStyle", ml, "ImGuiStyle") {
	}
	void init () {
		addField<DAS_BIND_MANAGED_FIELD(Alpha)>("Alpha","Alpha");
		addField<DAS_BIND_MANAGED_FIELD(WindowPadding)>("WindowPadding","WindowPadding");
		addField<DAS_BIND_MANAGED_FIELD(WindowRounding)>("WindowRounding","WindowRounding");
		addField<DAS_BIND_MANAGED_FIELD(WindowBorderSize)>("WindowBorderSize","WindowBorderSize");
		addField<DAS_BIND_MANAGED_FIELD(WindowMinSize)>("WindowMinSize","WindowMinSize");
		addField<DAS_BIND_MANAGED_FIELD(WindowTitleAlign)>("WindowTitleAlign","WindowTitleAlign");
		addField<ImGuiDir_,offsetof(ManagedType,WindowMenuButtonPosition)>("WindowMenuButtonPosition","WindowMenuButtonPosition")
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
		addField<DAS_BIND_MANAGED_FIELD(GrabMinSize)>("GrabMinSize","GrabMinSize");
		addField<DAS_BIND_MANAGED_FIELD(GrabRounding)>("GrabRounding","GrabRounding");
		addField<DAS_BIND_MANAGED_FIELD(LogSliderDeadzone)>("LogSliderDeadzone","LogSliderDeadzone");
		addField<DAS_BIND_MANAGED_FIELD(TabRounding)>("TabRounding","TabRounding");
		addField<DAS_BIND_MANAGED_FIELD(TabBorderSize)>("TabBorderSize","TabBorderSize");
		addField<DAS_BIND_MANAGED_FIELD(TabMinWidthForCloseButton)>("TabMinWidthForCloseButton","TabMinWidthForCloseButton");
		addField<ImGuiDir_,offsetof(ManagedType,ColorButtonPosition)>("ColorButtonPosition","ColorButtonPosition")
			.adjustAot("das_reinterpret<ImGuiDir_>::pass(",")");
		addField<DAS_BIND_MANAGED_FIELD(ButtonTextAlign)>("ButtonTextAlign","ButtonTextAlign");
		addField<DAS_BIND_MANAGED_FIELD(SelectableTextAlign)>("SelectableTextAlign","SelectableTextAlign");
		addField<DAS_BIND_MANAGED_FIELD(DisplayWindowPadding)>("DisplayWindowPadding","DisplayWindowPadding");
		addField<DAS_BIND_MANAGED_FIELD(DisplaySafeAreaPadding)>("DisplaySafeAreaPadding","DisplaySafeAreaPadding");
		addField<DAS_BIND_MANAGED_FIELD(MouseCursorScale)>("MouseCursorScale","MouseCursorScale");
		addField<DAS_BIND_MANAGED_FIELD(AntiAliasedLines)>("AntiAliasedLines","AntiAliasedLines");
		addField<DAS_BIND_MANAGED_FIELD(AntiAliasedLinesUseTex)>("AntiAliasedLinesUseTex","AntiAliasedLinesUseTex");
		addField<DAS_BIND_MANAGED_FIELD(AntiAliasedFill)>("AntiAliasedFill","AntiAliasedFill");
		addField<DAS_BIND_MANAGED_FIELD(CurveTessellationTol)>("CurveTessellationTol","CurveTessellationTol");
		addField<DAS_BIND_MANAGED_FIELD(CircleTessellationMaxError)>("CircleTessellationMaxError","CircleTessellationMaxError");
		addField<DAS_BIND_MANAGED_FIELD(Colors)>("Colors","Colors");
	}
};

struct ImGuiIO_GeneratedAnnotation : ManagedStructureAnnotation<ImGuiIO> {
	ImGuiIO_GeneratedAnnotation(ModuleLibrary & ml) : ManagedStructureAnnotation ("ImGuiIO", ml, "ImGuiIO") {
	}
	void init () {
		addField<ImGuiConfigFlags_,offsetof(ManagedType,ConfigFlags)>("ConfigFlags","ConfigFlags")
			.adjustAot("das_reinterpret<ImGuiConfigFlags_>::pass(",")");
		addField<ImGuiBackendFlags_,offsetof(ManagedType,BackendFlags)>("BackendFlags","BackendFlags")
			.adjustAot("das_reinterpret<ImGuiBackendFlags_>::pass(",")");
		addField<DAS_BIND_MANAGED_FIELD(DisplaySize)>("DisplaySize","DisplaySize");
		addField<DAS_BIND_MANAGED_FIELD(DeltaTime)>("DeltaTime","DeltaTime");
		addField<DAS_BIND_MANAGED_FIELD(IniSavingRate)>("IniSavingRate","IniSavingRate");
		addField<DAS_BIND_MANAGED_FIELD(IniFilename)>("IniFilename","IniFilename");
		addField<DAS_BIND_MANAGED_FIELD(LogFilename)>("LogFilename","LogFilename");
		addField<DAS_BIND_MANAGED_FIELD(MouseDoubleClickTime)>("MouseDoubleClickTime","MouseDoubleClickTime");
		addField<DAS_BIND_MANAGED_FIELD(MouseDoubleClickMaxDist)>("MouseDoubleClickMaxDist","MouseDoubleClickMaxDist");
		addField<DAS_BIND_MANAGED_FIELD(MouseDragThreshold)>("MouseDragThreshold","MouseDragThreshold");
		addField<DAS_BIND_MANAGED_FIELD(KeyMap)>("KeyMap","KeyMap");
		addField<DAS_BIND_MANAGED_FIELD(KeyRepeatDelay)>("KeyRepeatDelay","KeyRepeatDelay");
		addField<DAS_BIND_MANAGED_FIELD(KeyRepeatRate)>("KeyRepeatRate","KeyRepeatRate");
		addField<DAS_BIND_MANAGED_FIELD(UserData)>("UserData","UserData");
		addField<DAS_BIND_MANAGED_FIELD(Fonts)>("Fonts","Fonts");
		addField<DAS_BIND_MANAGED_FIELD(FontGlobalScale)>("FontGlobalScale","FontGlobalScale");
		addField<DAS_BIND_MANAGED_FIELD(FontAllowUserScaling)>("FontAllowUserScaling","FontAllowUserScaling");
		addField<DAS_BIND_MANAGED_FIELD(FontDefault)>("FontDefault","FontDefault");
		addField<DAS_BIND_MANAGED_FIELD(DisplayFramebufferScale)>("DisplayFramebufferScale","DisplayFramebufferScale");
		addField<DAS_BIND_MANAGED_FIELD(MouseDrawCursor)>("MouseDrawCursor","MouseDrawCursor");
		addField<DAS_BIND_MANAGED_FIELD(ConfigMacOSXBehaviors)>("ConfigMacOSXBehaviors","ConfigMacOSXBehaviors");
		addField<DAS_BIND_MANAGED_FIELD(ConfigInputTextCursorBlink)>("ConfigInputTextCursorBlink","ConfigInputTextCursorBlink");
		addField<DAS_BIND_MANAGED_FIELD(ConfigDragClickToInputText)>("ConfigDragClickToInputText","ConfigDragClickToInputText");
		addField<DAS_BIND_MANAGED_FIELD(ConfigWindowsResizeFromEdges)>("ConfigWindowsResizeFromEdges","ConfigWindowsResizeFromEdges");
		addField<DAS_BIND_MANAGED_FIELD(ConfigWindowsMoveFromTitleBarOnly)>("ConfigWindowsMoveFromTitleBarOnly","ConfigWindowsMoveFromTitleBarOnly");
		addField<DAS_BIND_MANAGED_FIELD(ConfigMemoryCompactTimer)>("ConfigMemoryCompactTimer","ConfigMemoryCompactTimer");
		addField<DAS_BIND_MANAGED_FIELD(BackendPlatformName)>("BackendPlatformName","BackendPlatformName");
		addField<DAS_BIND_MANAGED_FIELD(BackendRendererName)>("BackendRendererName","BackendRendererName");
		addField<DAS_BIND_MANAGED_FIELD(BackendPlatformUserData)>("BackendPlatformUserData","BackendPlatformUserData");
		addField<DAS_BIND_MANAGED_FIELD(BackendRendererUserData)>("BackendRendererUserData","BackendRendererUserData");
		addField<DAS_BIND_MANAGED_FIELD(BackendLanguageUserData)>("BackendLanguageUserData","BackendLanguageUserData");
		addField<DAS_BIND_MANAGED_FIELD(ClipboardUserData)>("ClipboardUserData","ClipboardUserData");
		addField<DAS_BIND_MANAGED_FIELD(ImeWindowHandle)>("ImeWindowHandle","ImeWindowHandle");
		addField<DAS_BIND_MANAGED_FIELD(MousePos)>("MousePos","MousePos");
		addField<DAS_BIND_MANAGED_FIELD(MouseDown)>("MouseDown","MouseDown");
		addField<DAS_BIND_MANAGED_FIELD(MouseWheel)>("MouseWheel","MouseWheel");
		addField<DAS_BIND_MANAGED_FIELD(MouseWheelH)>("MouseWheelH","MouseWheelH");
		addField<DAS_BIND_MANAGED_FIELD(KeyCtrl)>("KeyCtrl","KeyCtrl");
		addField<DAS_BIND_MANAGED_FIELD(KeyShift)>("KeyShift","KeyShift");
		addField<DAS_BIND_MANAGED_FIELD(KeyAlt)>("KeyAlt","KeyAlt");
		addField<DAS_BIND_MANAGED_FIELD(KeySuper)>("KeySuper","KeySuper");
		addField<DAS_BIND_MANAGED_FIELD(KeysDown)>("KeysDown","KeysDown");
		addField<DAS_BIND_MANAGED_FIELD(NavInputs)>("NavInputs","NavInputs");
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
		addField<DAS_BIND_MANAGED_FIELD(MetricsActiveAllocations)>("MetricsActiveAllocations","MetricsActiveAllocations");
		addField<DAS_BIND_MANAGED_FIELD(MouseDelta)>("MouseDelta","MouseDelta");
		addField<ImGuiKeyModFlags_,offsetof(ManagedType,KeyMods)>("KeyMods","KeyMods")
			.adjustAot("das_reinterpret<ImGuiKeyModFlags_>::pass(",")");
		addField<DAS_BIND_MANAGED_FIELD(MousePosPrev)>("MousePosPrev","MousePosPrev");
		addField<DAS_BIND_MANAGED_FIELD(MouseClickedPos)>("MouseClickedPos","MouseClickedPos");
		addField<DAS_BIND_MANAGED_FIELD(MouseClickedTime)>("MouseClickedTime","MouseClickedTime");
		addField<DAS_BIND_MANAGED_FIELD(MouseClicked)>("MouseClicked","MouseClicked");
		addField<DAS_BIND_MANAGED_FIELD(MouseDoubleClicked)>("MouseDoubleClicked","MouseDoubleClicked");
		addField<DAS_BIND_MANAGED_FIELD(MouseReleased)>("MouseReleased","MouseReleased");
		addField<DAS_BIND_MANAGED_FIELD(MouseDownOwned)>("MouseDownOwned","MouseDownOwned");
		addField<DAS_BIND_MANAGED_FIELD(MouseDownWasDoubleClick)>("MouseDownWasDoubleClick","MouseDownWasDoubleClick");
		addField<DAS_BIND_MANAGED_FIELD(MouseDownDuration)>("MouseDownDuration","MouseDownDuration");
		addField<DAS_BIND_MANAGED_FIELD(MouseDownDurationPrev)>("MouseDownDurationPrev","MouseDownDurationPrev");
		addField<DAS_BIND_MANAGED_FIELD(MouseDragMaxDistanceAbs)>("MouseDragMaxDistanceAbs","MouseDragMaxDistanceAbs");
		addField<DAS_BIND_MANAGED_FIELD(MouseDragMaxDistanceSqr)>("MouseDragMaxDistanceSqr","MouseDragMaxDistanceSqr");
		addField<DAS_BIND_MANAGED_FIELD(KeysDownDuration)>("KeysDownDuration","KeysDownDuration");
		addField<DAS_BIND_MANAGED_FIELD(KeysDownDurationPrev)>("KeysDownDurationPrev","KeysDownDurationPrev");
		addField<DAS_BIND_MANAGED_FIELD(NavInputsDownDuration)>("NavInputsDownDuration","NavInputsDownDuration");
		addField<DAS_BIND_MANAGED_FIELD(NavInputsDownDurationPrev)>("NavInputsDownDurationPrev","NavInputsDownDurationPrev");
		addField<DAS_BIND_MANAGED_FIELD(PenPressure)>("PenPressure","PenPressure");
		addField<DAS_BIND_MANAGED_FIELD(InputQueueSurrogate)>("InputQueueSurrogate","InputQueueSurrogate");
		addField<DAS_BIND_MANAGED_FIELD(InputQueueCharacters)>("InputQueueCharacters","InputQueueCharacters");
	}
};

struct ImGuiInputTextCallbackData_GeneratedAnnotation : ManagedStructureAnnotation<ImGuiInputTextCallbackData> {
	ImGuiInputTextCallbackData_GeneratedAnnotation(ModuleLibrary & ml) : ManagedStructureAnnotation ("ImGuiInputTextCallbackData", ml, "ImGuiInputTextCallbackData") {
	}
	void init () {
		addField<ImGuiInputTextFlags_,offsetof(ManagedType,EventFlag)>("EventFlag","EventFlag")
			.adjustAot("das_reinterpret<ImGuiInputTextFlags_>::pass(",")");
		addField<ImGuiInputTextFlags_,offsetof(ManagedType,Flags)>("Flags","Flags")
			.adjustAot("das_reinterpret<ImGuiInputTextFlags_>::pass(",")");
		addField<DAS_BIND_MANAGED_FIELD(UserData)>("UserData","UserData");
		addField<DAS_BIND_MANAGED_FIELD(EventChar)>("EventChar","EventChar");
		addField<ImGuiKey_,offsetof(ManagedType,EventKey)>("EventKey","EventKey")
			.adjustAot("das_reinterpret<ImGuiKey_>::pass(",")");
		addField<DAS_BIND_MANAGED_FIELD(Buf)>("Buf","Buf");
		addField<DAS_BIND_MANAGED_FIELD(BufTextLen)>("BufTextLen","BufTextLen");
		addField<DAS_BIND_MANAGED_FIELD(BufSize)>("BufSize","BufSize");
		addField<DAS_BIND_MANAGED_FIELD(BufDirty)>("BufDirty","BufDirty");
		addField<DAS_BIND_MANAGED_FIELD(CursorPos)>("CursorPos","CursorPos");
		addField<DAS_BIND_MANAGED_FIELD(SelectionStart)>("SelectionStart","SelectionStart");
		addField<DAS_BIND_MANAGED_FIELD(SelectionEnd)>("SelectionEnd","SelectionEnd");
	}
};

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

struct ImGuiTableColumnSortSpecs_GeneratedAnnotation : ManagedStructureAnnotation<ImGuiTableColumnSortSpecs> {
	virtual bool isLocal() const override { return true; }
	virtual bool canBePlacedInContainer() const override { return true; }
	ImGuiTableColumnSortSpecs_GeneratedAnnotation(ModuleLibrary & ml) : ManagedStructureAnnotation ("ImGuiTableColumnSortSpecs", ml, "ImGuiTableColumnSortSpecs") {
	}
	void init () {
		addField<DAS_BIND_MANAGED_FIELD(ColumnUserID)>("ColumnUserID","ColumnUserID");
		addField<DAS_BIND_MANAGED_FIELD(ColumnIndex)>("ColumnIndex","ColumnIndex");
		addField<DAS_BIND_MANAGED_FIELD(SortOrder)>("SortOrder","SortOrder");
	}
};

struct ImGuiTableSortSpecs_GeneratedAnnotation : ManagedStructureAnnotation<ImGuiTableSortSpecs> {
	ImGuiTableSortSpecs_GeneratedAnnotation(ModuleLibrary & ml) : ManagedStructureAnnotation ("ImGuiTableSortSpecs", ml, "ImGuiTableSortSpecs") {
	}
	void init () {
		addField<DAS_BIND_MANAGED_FIELD(Specs)>("Specs","Specs");
		addField<DAS_BIND_MANAGED_FIELD(SpecsCount)>("SpecsCount","SpecsCount");
		addField<DAS_BIND_MANAGED_FIELD(SpecsDirty)>("SpecsDirty","SpecsDirty");
	}
};

struct ImGuiOnceUponAFrame_GeneratedAnnotation : ManagedStructureAnnotation<ImGuiOnceUponAFrame> {
	ImGuiOnceUponAFrame_GeneratedAnnotation(ModuleLibrary & ml) : ManagedStructureAnnotation ("ImGuiOnceUponAFrame", ml, "ImGuiOnceUponAFrame") {
	}
	void init () {
		addField<DAS_BIND_MANAGED_FIELD(RefFrame)>("RefFrame","RefFrame");
	}
};

struct ImGuiTextFilter_ImGuiTextRange_GeneratedAnnotation : ManagedStructureAnnotation<ImGuiTextFilter::ImGuiTextRange> {
	ImGuiTextFilter_ImGuiTextRange_GeneratedAnnotation(ModuleLibrary & ml) : ManagedStructureAnnotation ("ImGuiTextRange", ml, "ImGuiTextFilter::ImGuiTextRange") {
	}
	void init () {
		addField<DAS_BIND_MANAGED_FIELD(b)>("b","b");
		addField<DAS_BIND_MANAGED_FIELD(e)>("e","e");
	}
};

struct ImGuiTextFilter_GeneratedAnnotation : ManagedStructureAnnotation<ImGuiTextFilter> {
	ImGuiTextFilter_GeneratedAnnotation(ModuleLibrary & ml) : ManagedStructureAnnotation ("ImGuiTextFilter", ml, "ImGuiTextFilter") {
	}
	void init () {
		addField<DAS_BIND_MANAGED_FIELD(InputBuf)>("InputBuf","InputBuf");
		addField<DAS_BIND_MANAGED_FIELD(Filters)>("Filters","Filters");
		addField<DAS_BIND_MANAGED_FIELD(CountGrep)>("CountGrep","CountGrep");
	}
};

struct ImGuiTextBuffer_GeneratedAnnotation : ManagedStructureAnnotation<ImGuiTextBuffer> {
	ImGuiTextBuffer_GeneratedAnnotation(ModuleLibrary & ml) : ManagedStructureAnnotation ("ImGuiTextBuffer", ml, "ImGuiTextBuffer") {
	}
	void init () {
		addField<DAS_BIND_MANAGED_FIELD(Buf)>("Buf","Buf");
	}
};

struct ImGuiStorage_ImGuiStoragePair_GeneratedAnnotation : ManagedStructureAnnotation<ImGuiStorage::ImGuiStoragePair> {
	ImGuiStorage_ImGuiStoragePair_GeneratedAnnotation(ModuleLibrary & ml) : ManagedStructureAnnotation ("ImGuiStoragePair", ml, "ImGuiStorage::ImGuiStoragePair") {
	}
	void init () {
		addField<DAS_BIND_MANAGED_FIELD(key)>("key","key");
	}
};

struct ImGuiStorage_GeneratedAnnotation : ManagedStructureAnnotation<ImGuiStorage> {
	ImGuiStorage_GeneratedAnnotation(ModuleLibrary & ml) : ManagedStructureAnnotation ("ImGuiStorage", ml, "ImGuiStorage") {
	}
	void init () {
		addField<DAS_BIND_MANAGED_FIELD(Data)>("Data","Data");
	}
};

struct ImGuiListClipper_GeneratedAnnotation : ManagedStructureAnnotation<ImGuiListClipper> {
	ImGuiListClipper_GeneratedAnnotation(ModuleLibrary & ml) : ManagedStructureAnnotation ("ImGuiListClipper", ml, "ImGuiListClipper") {
	}
	void init () {
		addField<DAS_BIND_MANAGED_FIELD(DisplayStart)>("DisplayStart","DisplayStart");
		addField<DAS_BIND_MANAGED_FIELD(DisplayEnd)>("DisplayEnd","DisplayEnd");
		addField<DAS_BIND_MANAGED_FIELD(ItemsCount)>("ItemsCount","ItemsCount");
		addField<DAS_BIND_MANAGED_FIELD(StepNo)>("StepNo","StepNo");
		addField<DAS_BIND_MANAGED_FIELD(ItemsFrozen)>("ItemsFrozen","ItemsFrozen");
		addField<DAS_BIND_MANAGED_FIELD(ItemsHeight)>("ItemsHeight","ItemsHeight");
		addField<DAS_BIND_MANAGED_FIELD(StartPosY)>("StartPosY","StartPosY");
	}
};

struct ImDrawCmd_GeneratedAnnotation : ManagedStructureAnnotation<ImDrawCmd> {
	ImDrawCmd_GeneratedAnnotation(ModuleLibrary & ml) : ManagedStructureAnnotation ("ImDrawCmd", ml, "ImDrawCmd") {
	}
	void init () {
		addField<DAS_BIND_MANAGED_FIELD(ClipRect)>("ClipRect","ClipRect");
		addField<DAS_BIND_MANAGED_FIELD(TextureId)>("TextureId","TextureId");
		addField<DAS_BIND_MANAGED_FIELD(VtxOffset)>("VtxOffset","VtxOffset");
		addField<DAS_BIND_MANAGED_FIELD(IdxOffset)>("IdxOffset","IdxOffset");
		addField<DAS_BIND_MANAGED_FIELD(ElemCount)>("ElemCount","ElemCount");
		addField<DAS_BIND_MANAGED_FIELD(UserCallbackData)>("UserCallbackData","UserCallbackData");
	}
};

struct ImDrawVert_GeneratedAnnotation : ManagedStructureAnnotation<ImDrawVert> {
	ImDrawVert_GeneratedAnnotation(ModuleLibrary & ml) : ManagedStructureAnnotation ("ImDrawVert", ml, "ImDrawVert") {
	}
	void init () {
		addField<DAS_BIND_MANAGED_FIELD(pos)>("pos","pos");
		addField<DAS_BIND_MANAGED_FIELD(uv)>("uv","uv");
		addField<DAS_BIND_MANAGED_FIELD(col)>("col","col");
	}
};

struct ImDrawCmdHeader_GeneratedAnnotation : ManagedStructureAnnotation<ImDrawCmdHeader> {
	ImDrawCmdHeader_GeneratedAnnotation(ModuleLibrary & ml) : ManagedStructureAnnotation ("ImDrawCmdHeader", ml, "ImDrawCmdHeader") {
	}
	void init () {
		addField<DAS_BIND_MANAGED_FIELD(ClipRect)>("ClipRect","ClipRect");
		addField<DAS_BIND_MANAGED_FIELD(TextureId)>("TextureId","TextureId");
		addField<DAS_BIND_MANAGED_FIELD(VtxOffset)>("VtxOffset","VtxOffset");
	}
};

struct ImDrawChannel_GeneratedAnnotation : ManagedStructureAnnotation<ImDrawChannel> {
	ImDrawChannel_GeneratedAnnotation(ModuleLibrary & ml) : ManagedStructureAnnotation ("ImDrawChannel", ml, "ImDrawChannel") {
	}
	void init () {
		addField<DAS_BIND_MANAGED_FIELD(_CmdBuffer)>("_CmdBuffer","_CmdBuffer");
		addField<DAS_BIND_MANAGED_FIELD(_IdxBuffer)>("_IdxBuffer","_IdxBuffer");
	}
};

struct ImDrawListSplitter_GeneratedAnnotation : ManagedStructureAnnotation<ImDrawListSplitter> {
	ImDrawListSplitter_GeneratedAnnotation(ModuleLibrary & ml) : ManagedStructureAnnotation ("ImDrawListSplitter", ml, "ImDrawListSplitter") {
	}
	void init () {
		addField<DAS_BIND_MANAGED_FIELD(_Current)>("_Current","_Current");
		addField<DAS_BIND_MANAGED_FIELD(_Count)>("_Count","_Count");
		addField<DAS_BIND_MANAGED_FIELD(_Channels)>("_Channels","_Channels");
	}
};

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
		addField<DAS_BIND_MANAGED_FIELD(_OwnerName)>("_OwnerName","_OwnerName");
		addField<DAS_BIND_MANAGED_FIELD(_VtxWritePtr)>("_VtxWritePtr","_VtxWritePtr");
		addField<DAS_BIND_MANAGED_FIELD(_IdxWritePtr)>("_IdxWritePtr","_IdxWritePtr");
		addField<DAS_BIND_MANAGED_FIELD(_ClipRectStack)>("_ClipRectStack","_ClipRectStack");
		addField<DAS_BIND_MANAGED_FIELD(_TextureIdStack)>("_TextureIdStack","_TextureIdStack");
		addField<DAS_BIND_MANAGED_FIELD(_Path)>("_Path","_Path");
		addField<DAS_BIND_MANAGED_FIELD(_CmdHeader)>("_CmdHeader","_CmdHeader");
		addField<DAS_BIND_MANAGED_FIELD(_Splitter)>("_Splitter","_Splitter");
		addField<DAS_BIND_MANAGED_FIELD(_FringeScale)>("_FringeScale","_FringeScale");
	}
};

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
	}
};

struct ImFontConfig_GeneratedAnnotation : ManagedStructureAnnotation<ImFontConfig> {
	ImFontConfig_GeneratedAnnotation(ModuleLibrary & ml) : ManagedStructureAnnotation ("ImFontConfig", ml, "ImFontConfig") {
	}
	void init () {
		addField<DAS_BIND_MANAGED_FIELD(FontData)>("FontData","FontData");
		addField<DAS_BIND_MANAGED_FIELD(FontDataSize)>("FontDataSize","FontDataSize");
		addField<DAS_BIND_MANAGED_FIELD(FontDataOwnedByAtlas)>("FontDataOwnedByAtlas","FontDataOwnedByAtlas");
		addField<DAS_BIND_MANAGED_FIELD(FontNo)>("FontNo","FontNo");
		addField<DAS_BIND_MANAGED_FIELD(SizePixels)>("SizePixels","SizePixels");
		addField<DAS_BIND_MANAGED_FIELD(OversampleH)>("OversampleH","OversampleH");
		addField<DAS_BIND_MANAGED_FIELD(OversampleV)>("OversampleV","OversampleV");
		addField<DAS_BIND_MANAGED_FIELD(PixelSnapH)>("PixelSnapH","PixelSnapH");
		addField<DAS_BIND_MANAGED_FIELD(GlyphExtraSpacing)>("GlyphExtraSpacing","GlyphExtraSpacing");
		addField<DAS_BIND_MANAGED_FIELD(GlyphOffset)>("GlyphOffset","GlyphOffset");
		addField<DAS_BIND_MANAGED_FIELD(GlyphRanges)>("GlyphRanges","GlyphRanges");
		addField<DAS_BIND_MANAGED_FIELD(GlyphMinAdvanceX)>("GlyphMinAdvanceX","GlyphMinAdvanceX");
		addField<DAS_BIND_MANAGED_FIELD(GlyphMaxAdvanceX)>("GlyphMaxAdvanceX","GlyphMaxAdvanceX");
		addField<DAS_BIND_MANAGED_FIELD(MergeMode)>("MergeMode","MergeMode");
		addField<DAS_BIND_MANAGED_FIELD(FontBuilderFlags)>("FontBuilderFlags","FontBuilderFlags");
		addField<DAS_BIND_MANAGED_FIELD(RasterizerMultiply)>("RasterizerMultiply","RasterizerMultiply");
		addField<DAS_BIND_MANAGED_FIELD(EllipsisChar)>("EllipsisChar","EllipsisChar");
		addField<DAS_BIND_MANAGED_FIELD(Name)>("Name","Name");
		addField<DAS_BIND_MANAGED_FIELD(DstFont)>("DstFont","DstFont");
	}
};

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
	}
};

struct ImFontGlyphRangesBuilder_GeneratedAnnotation : ManagedStructureAnnotation<ImFontGlyphRangesBuilder> {
	ImFontGlyphRangesBuilder_GeneratedAnnotation(ModuleLibrary & ml) : ManagedStructureAnnotation ("ImFontGlyphRangesBuilder", ml, "ImFontGlyphRangesBuilder") {
	}
	void init () {
		addField<DAS_BIND_MANAGED_FIELD(UsedChars)>("UsedChars","UsedChars");
	}
};

struct ImFontAtlasCustomRect_GeneratedAnnotation : ManagedStructureAnnotation<ImFontAtlasCustomRect> {
	ImFontAtlasCustomRect_GeneratedAnnotation(ModuleLibrary & ml) : ManagedStructureAnnotation ("ImFontAtlasCustomRect", ml, "ImFontAtlasCustomRect") {
	}
	void init () {
		addField<DAS_BIND_MANAGED_FIELD(Width)>("Width","Width");
		addField<DAS_BIND_MANAGED_FIELD(Height)>("Height","Height");
		addField<DAS_BIND_MANAGED_FIELD(X)>("X","X");
		addField<DAS_BIND_MANAGED_FIELD(Y)>("Y","Y");
		addField<DAS_BIND_MANAGED_FIELD(GlyphID)>("GlyphID","GlyphID");
		addField<DAS_BIND_MANAGED_FIELD(GlyphAdvanceX)>("GlyphAdvanceX","GlyphAdvanceX");
		addField<DAS_BIND_MANAGED_FIELD(GlyphOffset)>("GlyphOffset","GlyphOffset");
		addField<DAS_BIND_MANAGED_FIELD(Font)>("Font","Font");
	}
};

struct ImFontAtlas_GeneratedAnnotation : ManagedStructureAnnotation<ImFontAtlas> {
	ImFontAtlas_GeneratedAnnotation(ModuleLibrary & ml) : ManagedStructureAnnotation ("ImFontAtlas", ml, "ImFontAtlas") {
	}
	void init () {
		addField<ImFontAtlasFlags_,offsetof(ManagedType,Flags)>("Flags","Flags")
			.adjustAot("das_reinterpret<ImFontAtlasFlags_>::pass(",")");
		addField<DAS_BIND_MANAGED_FIELD(TexID)>("TexID","TexID");
		addField<DAS_BIND_MANAGED_FIELD(TexDesiredWidth)>("TexDesiredWidth","TexDesiredWidth");
		addField<DAS_BIND_MANAGED_FIELD(TexGlyphPadding)>("TexGlyphPadding","TexGlyphPadding");
		addField<DAS_BIND_MANAGED_FIELD(Locked)>("Locked","Locked");
		addField<DAS_BIND_MANAGED_FIELD(TexPixelsUseColors)>("TexPixelsUseColors","TexPixelsUseColors");
		addField<DAS_BIND_MANAGED_FIELD(TexPixelsAlpha8)>("TexPixelsAlpha8","TexPixelsAlpha8");
		addField<DAS_BIND_MANAGED_FIELD(TexPixelsRGBA32)>("TexPixelsRGBA32","TexPixelsRGBA32");
		addField<DAS_BIND_MANAGED_FIELD(TexWidth)>("TexWidth","TexWidth");
		addField<DAS_BIND_MANAGED_FIELD(TexHeight)>("TexHeight","TexHeight");
		addField<DAS_BIND_MANAGED_FIELD(TexUvScale)>("TexUvScale","TexUvScale");
		addField<DAS_BIND_MANAGED_FIELD(TexUvWhitePixel)>("TexUvWhitePixel","TexUvWhitePixel");
		addField<DAS_BIND_MANAGED_FIELD(Fonts)>("Fonts","Fonts");
		addField<DAS_BIND_MANAGED_FIELD(CustomRects)>("CustomRects","CustomRects");
		addField<DAS_BIND_MANAGED_FIELD(ConfigData)>("ConfigData","ConfigData");
		addField<DAS_BIND_MANAGED_FIELD(TexUvLines)>("TexUvLines","TexUvLines");
		addField<DAS_BIND_MANAGED_FIELD(FontBuilderIO)>("FontBuilderIO","FontBuilderIO");
		addField<DAS_BIND_MANAGED_FIELD(FontBuilderFlags)>("FontBuilderFlags","FontBuilderFlags");
		addField<DAS_BIND_MANAGED_FIELD(PackIdMouseCursors)>("PackIdMouseCursors","PackIdMouseCursors");
		addField<DAS_BIND_MANAGED_FIELD(PackIdLines)>("PackIdLines","PackIdLines");
	}
};

struct ImFont_GeneratedAnnotation : ManagedStructureAnnotation<ImFont> {
	ImFont_GeneratedAnnotation(ModuleLibrary & ml) : ManagedStructureAnnotation ("ImFont", ml, "ImFont") {
	}
	void init () {
		addField<DAS_BIND_MANAGED_FIELD(IndexAdvanceX)>("IndexAdvanceX","IndexAdvanceX");
		addField<DAS_BIND_MANAGED_FIELD(FallbackAdvanceX)>("FallbackAdvanceX","FallbackAdvanceX");
		addField<DAS_BIND_MANAGED_FIELD(FontSize)>("FontSize","FontSize");
		addField<DAS_BIND_MANAGED_FIELD(IndexLookup)>("IndexLookup","IndexLookup");
		addField<DAS_BIND_MANAGED_FIELD(Glyphs)>("Glyphs","Glyphs");
		addField<DAS_BIND_MANAGED_FIELD(FallbackGlyph)>("FallbackGlyph","FallbackGlyph");
		addField<DAS_BIND_MANAGED_FIELD(ContainerAtlas)>("ContainerAtlas","ContainerAtlas");
		addField<DAS_BIND_MANAGED_FIELD(ConfigData)>("ConfigData","ConfigData");
		addField<DAS_BIND_MANAGED_FIELD(ConfigDataCount)>("ConfigDataCount","ConfigDataCount");
		addField<DAS_BIND_MANAGED_FIELD(FallbackChar)>("FallbackChar","FallbackChar");
		addField<DAS_BIND_MANAGED_FIELD(EllipsisChar)>("EllipsisChar","EllipsisChar");
		addField<DAS_BIND_MANAGED_FIELD(DirtyLookupTables)>("DirtyLookupTables","DirtyLookupTables");
		addField<DAS_BIND_MANAGED_FIELD(Scale)>("Scale","Scale");
		addField<DAS_BIND_MANAGED_FIELD(Ascent)>("Ascent","Ascent");
		addField<DAS_BIND_MANAGED_FIELD(Descent)>("Descent","Descent");
		addField<DAS_BIND_MANAGED_FIELD(MetricsTotalSurface)>("MetricsTotalSurface","MetricsTotalSurface");
		addField<DAS_BIND_MANAGED_FIELD(Used4kPagesMap)>("Used4kPagesMap","Used4kPagesMap");
	}
};

struct ImGuiViewport_GeneratedAnnotation : ManagedStructureAnnotation<ImGuiViewport> {
	ImGuiViewport_GeneratedAnnotation(ModuleLibrary & ml) : ManagedStructureAnnotation ("ImGuiViewport", ml, "ImGuiViewport") {
	}
	void init () {
		addField<ImGuiViewportFlags_,offsetof(ManagedType,Flags)>("Flags","Flags")
			.adjustAot("das_reinterpret<ImGuiViewportFlags_>::pass(",")");
		addField<DAS_BIND_MANAGED_FIELD(Pos)>("Pos","Pos");
		addField<DAS_BIND_MANAGED_FIELD(Size)>("Size","Size");
		addField<DAS_BIND_MANAGED_FIELD(WorkPos)>("WorkPos","WorkPos");
		addField<DAS_BIND_MANAGED_FIELD(WorkSize)>("WorkSize","WorkSize");
	}
};

