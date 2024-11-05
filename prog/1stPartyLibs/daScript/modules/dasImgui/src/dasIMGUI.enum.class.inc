// this file is generated via Daslang automatic C++ binder
// all user modifications will be lost after this file is re-generated

// from imgui/imgui.h:1094:6
class Enumeration_ImGuiWindowFlags_ : public das::Enumeration {
public:
	Enumeration_ImGuiWindowFlags_() : das::Enumeration("ImGuiWindowFlags") {
		external = true;
		cppName = "ImGuiWindowFlags_";
		baseType = (das::Type) das::ToBasicType<int>::type;
		addIEx("None", "ImGuiWindowFlags_None", int64_t(ImGuiWindowFlags_::ImGuiWindowFlags_None), das::LineInfo());
		addIEx("NoTitleBar", "ImGuiWindowFlags_NoTitleBar", int64_t(ImGuiWindowFlags_::ImGuiWindowFlags_NoTitleBar), das::LineInfo());
		addIEx("NoResize", "ImGuiWindowFlags_NoResize", int64_t(ImGuiWindowFlags_::ImGuiWindowFlags_NoResize), das::LineInfo());
		addIEx("NoMove", "ImGuiWindowFlags_NoMove", int64_t(ImGuiWindowFlags_::ImGuiWindowFlags_NoMove), das::LineInfo());
		addIEx("NoScrollbar", "ImGuiWindowFlags_NoScrollbar", int64_t(ImGuiWindowFlags_::ImGuiWindowFlags_NoScrollbar), das::LineInfo());
		addIEx("NoScrollWithMouse", "ImGuiWindowFlags_NoScrollWithMouse", int64_t(ImGuiWindowFlags_::ImGuiWindowFlags_NoScrollWithMouse), das::LineInfo());
		addIEx("NoCollapse", "ImGuiWindowFlags_NoCollapse", int64_t(ImGuiWindowFlags_::ImGuiWindowFlags_NoCollapse), das::LineInfo());
		addIEx("AlwaysAutoResize", "ImGuiWindowFlags_AlwaysAutoResize", int64_t(ImGuiWindowFlags_::ImGuiWindowFlags_AlwaysAutoResize), das::LineInfo());
		addIEx("NoBackground", "ImGuiWindowFlags_NoBackground", int64_t(ImGuiWindowFlags_::ImGuiWindowFlags_NoBackground), das::LineInfo());
		addIEx("NoSavedSettings", "ImGuiWindowFlags_NoSavedSettings", int64_t(ImGuiWindowFlags_::ImGuiWindowFlags_NoSavedSettings), das::LineInfo());
		addIEx("NoMouseInputs", "ImGuiWindowFlags_NoMouseInputs", int64_t(ImGuiWindowFlags_::ImGuiWindowFlags_NoMouseInputs), das::LineInfo());
		addIEx("MenuBar", "ImGuiWindowFlags_MenuBar", int64_t(ImGuiWindowFlags_::ImGuiWindowFlags_MenuBar), das::LineInfo());
		addIEx("HorizontalScrollbar", "ImGuiWindowFlags_HorizontalScrollbar", int64_t(ImGuiWindowFlags_::ImGuiWindowFlags_HorizontalScrollbar), das::LineInfo());
		addIEx("NoFocusOnAppearing", "ImGuiWindowFlags_NoFocusOnAppearing", int64_t(ImGuiWindowFlags_::ImGuiWindowFlags_NoFocusOnAppearing), das::LineInfo());
		addIEx("NoBringToFrontOnFocus", "ImGuiWindowFlags_NoBringToFrontOnFocus", int64_t(ImGuiWindowFlags_::ImGuiWindowFlags_NoBringToFrontOnFocus), das::LineInfo());
		addIEx("AlwaysVerticalScrollbar", "ImGuiWindowFlags_AlwaysVerticalScrollbar", int64_t(ImGuiWindowFlags_::ImGuiWindowFlags_AlwaysVerticalScrollbar), das::LineInfo());
		addIEx("AlwaysHorizontalScrollbar", "ImGuiWindowFlags_AlwaysHorizontalScrollbar", int64_t(ImGuiWindowFlags_::ImGuiWindowFlags_AlwaysHorizontalScrollbar), das::LineInfo());
		addIEx("NoNavInputs", "ImGuiWindowFlags_NoNavInputs", int64_t(ImGuiWindowFlags_::ImGuiWindowFlags_NoNavInputs), das::LineInfo());
		addIEx("NoNavFocus", "ImGuiWindowFlags_NoNavFocus", int64_t(ImGuiWindowFlags_::ImGuiWindowFlags_NoNavFocus), das::LineInfo());
		addIEx("UnsavedDocument", "ImGuiWindowFlags_UnsavedDocument", int64_t(ImGuiWindowFlags_::ImGuiWindowFlags_UnsavedDocument), das::LineInfo());
		addIEx("NoDocking", "ImGuiWindowFlags_NoDocking", int64_t(ImGuiWindowFlags_::ImGuiWindowFlags_NoDocking), das::LineInfo());
		addIEx("NoNav", "ImGuiWindowFlags_NoNav", int64_t(ImGuiWindowFlags_::ImGuiWindowFlags_NoNav), das::LineInfo());
		addIEx("NoDecoration", "ImGuiWindowFlags_NoDecoration", int64_t(ImGuiWindowFlags_::ImGuiWindowFlags_NoDecoration), das::LineInfo());
		addIEx("NoInputs", "ImGuiWindowFlags_NoInputs", int64_t(ImGuiWindowFlags_::ImGuiWindowFlags_NoInputs), das::LineInfo());
		addIEx("ChildWindow", "ImGuiWindowFlags_ChildWindow", int64_t(ImGuiWindowFlags_::ImGuiWindowFlags_ChildWindow), das::LineInfo());
		addIEx("Tooltip", "ImGuiWindowFlags_Tooltip", int64_t(ImGuiWindowFlags_::ImGuiWindowFlags_Tooltip), das::LineInfo());
		addIEx("Popup", "ImGuiWindowFlags_Popup", int64_t(ImGuiWindowFlags_::ImGuiWindowFlags_Popup), das::LineInfo());
		addIEx("Modal", "ImGuiWindowFlags_Modal", int64_t(ImGuiWindowFlags_::ImGuiWindowFlags_Modal), das::LineInfo());
		addIEx("ChildMenu", "ImGuiWindowFlags_ChildMenu", int64_t(ImGuiWindowFlags_::ImGuiWindowFlags_ChildMenu), das::LineInfo());
		addIEx("DockNodeHost", "ImGuiWindowFlags_DockNodeHost", int64_t(ImGuiWindowFlags_::ImGuiWindowFlags_DockNodeHost), das::LineInfo());
	}
};

// from imgui/imgui.h:1145:6
class Enumeration_ImGuiChildFlags_ : public das::Enumeration {
public:
	Enumeration_ImGuiChildFlags_() : das::Enumeration("ImGuiChildFlags") {
		external = true;
		cppName = "ImGuiChildFlags_";
		baseType = (das::Type) das::ToBasicType<int>::type;
		addIEx("None", "ImGuiChildFlags_None", int64_t(ImGuiChildFlags_::ImGuiChildFlags_None), das::LineInfo());
		addIEx("Border", "ImGuiChildFlags_Border", int64_t(ImGuiChildFlags_::ImGuiChildFlags_Border), das::LineInfo());
		addIEx("AlwaysUseWindowPadding", "ImGuiChildFlags_AlwaysUseWindowPadding", int64_t(ImGuiChildFlags_::ImGuiChildFlags_AlwaysUseWindowPadding), das::LineInfo());
		addIEx("ResizeX", "ImGuiChildFlags_ResizeX", int64_t(ImGuiChildFlags_::ImGuiChildFlags_ResizeX), das::LineInfo());
		addIEx("ResizeY", "ImGuiChildFlags_ResizeY", int64_t(ImGuiChildFlags_::ImGuiChildFlags_ResizeY), das::LineInfo());
		addIEx("AutoResizeX", "ImGuiChildFlags_AutoResizeX", int64_t(ImGuiChildFlags_::ImGuiChildFlags_AutoResizeX), das::LineInfo());
		addIEx("AutoResizeY", "ImGuiChildFlags_AutoResizeY", int64_t(ImGuiChildFlags_::ImGuiChildFlags_AutoResizeY), das::LineInfo());
		addIEx("AlwaysAutoResize", "ImGuiChildFlags_AlwaysAutoResize", int64_t(ImGuiChildFlags_::ImGuiChildFlags_AlwaysAutoResize), das::LineInfo());
		addIEx("FrameStyle", "ImGuiChildFlags_FrameStyle", int64_t(ImGuiChildFlags_::ImGuiChildFlags_FrameStyle), das::LineInfo());
		addIEx("NavFlattened", "ImGuiChildFlags_NavFlattened", int64_t(ImGuiChildFlags_::ImGuiChildFlags_NavFlattened), das::LineInfo());
	}
};

// from imgui/imgui.h:1161:6
class Enumeration_ImGuiItemFlags_ : public das::Enumeration {
public:
	Enumeration_ImGuiItemFlags_() : das::Enumeration("ImGuiItemFlags") {
		external = true;
		cppName = "ImGuiItemFlags_";
		baseType = (das::Type) das::ToBasicType<int>::type;
		addIEx("None", "ImGuiItemFlags_None", int64_t(ImGuiItemFlags_::ImGuiItemFlags_None), das::LineInfo());
		addIEx("NoTabStop", "ImGuiItemFlags_NoTabStop", int64_t(ImGuiItemFlags_::ImGuiItemFlags_NoTabStop), das::LineInfo());
		addIEx("NoNav", "ImGuiItemFlags_NoNav", int64_t(ImGuiItemFlags_::ImGuiItemFlags_NoNav), das::LineInfo());
		addIEx("NoNavDefaultFocus", "ImGuiItemFlags_NoNavDefaultFocus", int64_t(ImGuiItemFlags_::ImGuiItemFlags_NoNavDefaultFocus), das::LineInfo());
		addIEx("ButtonRepeat", "ImGuiItemFlags_ButtonRepeat", int64_t(ImGuiItemFlags_::ImGuiItemFlags_ButtonRepeat), das::LineInfo());
		addIEx("AutoClosePopups", "ImGuiItemFlags_AutoClosePopups", int64_t(ImGuiItemFlags_::ImGuiItemFlags_AutoClosePopups), das::LineInfo());
	}
};

// from imgui/imgui.h:1173:6
class Enumeration_ImGuiInputTextFlags_ : public das::Enumeration {
public:
	Enumeration_ImGuiInputTextFlags_() : das::Enumeration("ImGuiInputTextFlags") {
		external = true;
		cppName = "ImGuiInputTextFlags_";
		baseType = (das::Type) das::ToBasicType<int>::type;
		addIEx("None", "ImGuiInputTextFlags_None", int64_t(ImGuiInputTextFlags_::ImGuiInputTextFlags_None), das::LineInfo());
		addIEx("CharsDecimal", "ImGuiInputTextFlags_CharsDecimal", int64_t(ImGuiInputTextFlags_::ImGuiInputTextFlags_CharsDecimal), das::LineInfo());
		addIEx("CharsHexadecimal", "ImGuiInputTextFlags_CharsHexadecimal", int64_t(ImGuiInputTextFlags_::ImGuiInputTextFlags_CharsHexadecimal), das::LineInfo());
		addIEx("CharsScientific", "ImGuiInputTextFlags_CharsScientific", int64_t(ImGuiInputTextFlags_::ImGuiInputTextFlags_CharsScientific), das::LineInfo());
		addIEx("CharsUppercase", "ImGuiInputTextFlags_CharsUppercase", int64_t(ImGuiInputTextFlags_::ImGuiInputTextFlags_CharsUppercase), das::LineInfo());
		addIEx("CharsNoBlank", "ImGuiInputTextFlags_CharsNoBlank", int64_t(ImGuiInputTextFlags_::ImGuiInputTextFlags_CharsNoBlank), das::LineInfo());
		addIEx("AllowTabInput", "ImGuiInputTextFlags_AllowTabInput", int64_t(ImGuiInputTextFlags_::ImGuiInputTextFlags_AllowTabInput), das::LineInfo());
		addIEx("EnterReturnsTrue", "ImGuiInputTextFlags_EnterReturnsTrue", int64_t(ImGuiInputTextFlags_::ImGuiInputTextFlags_EnterReturnsTrue), das::LineInfo());
		addIEx("EscapeClearsAll", "ImGuiInputTextFlags_EscapeClearsAll", int64_t(ImGuiInputTextFlags_::ImGuiInputTextFlags_EscapeClearsAll), das::LineInfo());
		addIEx("CtrlEnterForNewLine", "ImGuiInputTextFlags_CtrlEnterForNewLine", int64_t(ImGuiInputTextFlags_::ImGuiInputTextFlags_CtrlEnterForNewLine), das::LineInfo());
		addIEx("ReadOnly", "ImGuiInputTextFlags_ReadOnly", int64_t(ImGuiInputTextFlags_::ImGuiInputTextFlags_ReadOnly), das::LineInfo());
		addIEx("Password", "ImGuiInputTextFlags_Password", int64_t(ImGuiInputTextFlags_::ImGuiInputTextFlags_Password), das::LineInfo());
		addIEx("AlwaysOverwrite", "ImGuiInputTextFlags_AlwaysOverwrite", int64_t(ImGuiInputTextFlags_::ImGuiInputTextFlags_AlwaysOverwrite), das::LineInfo());
		addIEx("AutoSelectAll", "ImGuiInputTextFlags_AutoSelectAll", int64_t(ImGuiInputTextFlags_::ImGuiInputTextFlags_AutoSelectAll), das::LineInfo());
		addIEx("ParseEmptyRefVal", "ImGuiInputTextFlags_ParseEmptyRefVal", int64_t(ImGuiInputTextFlags_::ImGuiInputTextFlags_ParseEmptyRefVal), das::LineInfo());
		addIEx("DisplayEmptyRefVal", "ImGuiInputTextFlags_DisplayEmptyRefVal", int64_t(ImGuiInputTextFlags_::ImGuiInputTextFlags_DisplayEmptyRefVal), das::LineInfo());
		addIEx("NoHorizontalScroll", "ImGuiInputTextFlags_NoHorizontalScroll", int64_t(ImGuiInputTextFlags_::ImGuiInputTextFlags_NoHorizontalScroll), das::LineInfo());
		addIEx("NoUndoRedo", "ImGuiInputTextFlags_NoUndoRedo", int64_t(ImGuiInputTextFlags_::ImGuiInputTextFlags_NoUndoRedo), das::LineInfo());
		addIEx("CallbackCompletion", "ImGuiInputTextFlags_CallbackCompletion", int64_t(ImGuiInputTextFlags_::ImGuiInputTextFlags_CallbackCompletion), das::LineInfo());
		addIEx("CallbackHistory", "ImGuiInputTextFlags_CallbackHistory", int64_t(ImGuiInputTextFlags_::ImGuiInputTextFlags_CallbackHistory), das::LineInfo());
		addIEx("CallbackAlways", "ImGuiInputTextFlags_CallbackAlways", int64_t(ImGuiInputTextFlags_::ImGuiInputTextFlags_CallbackAlways), das::LineInfo());
		addIEx("CallbackCharFilter", "ImGuiInputTextFlags_CallbackCharFilter", int64_t(ImGuiInputTextFlags_::ImGuiInputTextFlags_CallbackCharFilter), das::LineInfo());
		addIEx("CallbackResize", "ImGuiInputTextFlags_CallbackResize", int64_t(ImGuiInputTextFlags_::ImGuiInputTextFlags_CallbackResize), das::LineInfo());
		addIEx("CallbackEdit", "ImGuiInputTextFlags_CallbackEdit", int64_t(ImGuiInputTextFlags_::ImGuiInputTextFlags_CallbackEdit), das::LineInfo());
	}
};

// from imgui/imgui.h:1212:6
class Enumeration_ImGuiTreeNodeFlags_ : public das::Enumeration {
public:
	Enumeration_ImGuiTreeNodeFlags_() : das::Enumeration("ImGuiTreeNodeFlags") {
		external = true;
		cppName = "ImGuiTreeNodeFlags_";
		baseType = (das::Type) das::ToBasicType<int>::type;
		addIEx("None", "ImGuiTreeNodeFlags_None", int64_t(ImGuiTreeNodeFlags_::ImGuiTreeNodeFlags_None), das::LineInfo());
		addIEx("Selected", "ImGuiTreeNodeFlags_Selected", int64_t(ImGuiTreeNodeFlags_::ImGuiTreeNodeFlags_Selected), das::LineInfo());
		addIEx("Framed", "ImGuiTreeNodeFlags_Framed", int64_t(ImGuiTreeNodeFlags_::ImGuiTreeNodeFlags_Framed), das::LineInfo());
		addIEx("AllowOverlap", "ImGuiTreeNodeFlags_AllowOverlap", int64_t(ImGuiTreeNodeFlags_::ImGuiTreeNodeFlags_AllowOverlap), das::LineInfo());
		addIEx("NoTreePushOnOpen", "ImGuiTreeNodeFlags_NoTreePushOnOpen", int64_t(ImGuiTreeNodeFlags_::ImGuiTreeNodeFlags_NoTreePushOnOpen), das::LineInfo());
		addIEx("NoAutoOpenOnLog", "ImGuiTreeNodeFlags_NoAutoOpenOnLog", int64_t(ImGuiTreeNodeFlags_::ImGuiTreeNodeFlags_NoAutoOpenOnLog), das::LineInfo());
		addIEx("DefaultOpen", "ImGuiTreeNodeFlags_DefaultOpen", int64_t(ImGuiTreeNodeFlags_::ImGuiTreeNodeFlags_DefaultOpen), das::LineInfo());
		addIEx("OpenOnDoubleClick", "ImGuiTreeNodeFlags_OpenOnDoubleClick", int64_t(ImGuiTreeNodeFlags_::ImGuiTreeNodeFlags_OpenOnDoubleClick), das::LineInfo());
		addIEx("OpenOnArrow", "ImGuiTreeNodeFlags_OpenOnArrow", int64_t(ImGuiTreeNodeFlags_::ImGuiTreeNodeFlags_OpenOnArrow), das::LineInfo());
		addIEx("Leaf", "ImGuiTreeNodeFlags_Leaf", int64_t(ImGuiTreeNodeFlags_::ImGuiTreeNodeFlags_Leaf), das::LineInfo());
		addIEx("Bullet", "ImGuiTreeNodeFlags_Bullet", int64_t(ImGuiTreeNodeFlags_::ImGuiTreeNodeFlags_Bullet), das::LineInfo());
		addIEx("FramePadding", "ImGuiTreeNodeFlags_FramePadding", int64_t(ImGuiTreeNodeFlags_::ImGuiTreeNodeFlags_FramePadding), das::LineInfo());
		addIEx("SpanAvailWidth", "ImGuiTreeNodeFlags_SpanAvailWidth", int64_t(ImGuiTreeNodeFlags_::ImGuiTreeNodeFlags_SpanAvailWidth), das::LineInfo());
		addIEx("SpanFullWidth", "ImGuiTreeNodeFlags_SpanFullWidth", int64_t(ImGuiTreeNodeFlags_::ImGuiTreeNodeFlags_SpanFullWidth), das::LineInfo());
		addIEx("SpanTextWidth", "ImGuiTreeNodeFlags_SpanTextWidth", int64_t(ImGuiTreeNodeFlags_::ImGuiTreeNodeFlags_SpanTextWidth), das::LineInfo());
		addIEx("SpanAllColumns", "ImGuiTreeNodeFlags_SpanAllColumns", int64_t(ImGuiTreeNodeFlags_::ImGuiTreeNodeFlags_SpanAllColumns), das::LineInfo());
		addIEx("NavLeftJumpsBackHere", "ImGuiTreeNodeFlags_NavLeftJumpsBackHere", int64_t(ImGuiTreeNodeFlags_::ImGuiTreeNodeFlags_NavLeftJumpsBackHere), das::LineInfo());
		addIEx("CollapsingHeader", "ImGuiTreeNodeFlags_CollapsingHeader", int64_t(ImGuiTreeNodeFlags_::ImGuiTreeNodeFlags_CollapsingHeader), das::LineInfo());
	}
};

// from imgui/imgui.h:1247:6
class Enumeration_ImGuiPopupFlags_ : public das::Enumeration {
public:
	Enumeration_ImGuiPopupFlags_() : das::Enumeration("ImGuiPopupFlags") {
		external = true;
		cppName = "ImGuiPopupFlags_";
		baseType = (das::Type) das::ToBasicType<int>::type;
		addIEx("None", "ImGuiPopupFlags_None", int64_t(ImGuiPopupFlags_::ImGuiPopupFlags_None), das::LineInfo());
		addIEx("MouseButtonLeft", "ImGuiPopupFlags_MouseButtonLeft", int64_t(ImGuiPopupFlags_::ImGuiPopupFlags_MouseButtonLeft), das::LineInfo());
		addIEx("MouseButtonRight", "ImGuiPopupFlags_MouseButtonRight", int64_t(ImGuiPopupFlags_::ImGuiPopupFlags_MouseButtonRight), das::LineInfo());
		addIEx("MouseButtonMiddle", "ImGuiPopupFlags_MouseButtonMiddle", int64_t(ImGuiPopupFlags_::ImGuiPopupFlags_MouseButtonMiddle), das::LineInfo());
		addIEx("MouseButtonMask_", "ImGuiPopupFlags_MouseButtonMask_", int64_t(ImGuiPopupFlags_::ImGuiPopupFlags_MouseButtonMask_), das::LineInfo());
		addIEx("MouseButtonDefault_", "ImGuiPopupFlags_MouseButtonDefault_", int64_t(ImGuiPopupFlags_::ImGuiPopupFlags_MouseButtonDefault_), das::LineInfo());
		addIEx("NoReopen", "ImGuiPopupFlags_NoReopen", int64_t(ImGuiPopupFlags_::ImGuiPopupFlags_NoReopen), das::LineInfo());
		addIEx("NoOpenOverExistingPopup", "ImGuiPopupFlags_NoOpenOverExistingPopup", int64_t(ImGuiPopupFlags_::ImGuiPopupFlags_NoOpenOverExistingPopup), das::LineInfo());
		addIEx("NoOpenOverItems", "ImGuiPopupFlags_NoOpenOverItems", int64_t(ImGuiPopupFlags_::ImGuiPopupFlags_NoOpenOverItems), das::LineInfo());
		addIEx("AnyPopupId", "ImGuiPopupFlags_AnyPopupId", int64_t(ImGuiPopupFlags_::ImGuiPopupFlags_AnyPopupId), das::LineInfo());
		addIEx("AnyPopupLevel", "ImGuiPopupFlags_AnyPopupLevel", int64_t(ImGuiPopupFlags_::ImGuiPopupFlags_AnyPopupLevel), das::LineInfo());
		addIEx("AnyPopup", "ImGuiPopupFlags_AnyPopup", int64_t(ImGuiPopupFlags_::ImGuiPopupFlags_AnyPopup), das::LineInfo());
	}
};

// from imgui/imgui.h:1265:6
class Enumeration_ImGuiSelectableFlags_ : public das::Enumeration {
public:
	Enumeration_ImGuiSelectableFlags_() : das::Enumeration("ImGuiSelectableFlags") {
		external = true;
		cppName = "ImGuiSelectableFlags_";
		baseType = (das::Type) das::ToBasicType<int>::type;
		addIEx("None", "ImGuiSelectableFlags_None", int64_t(ImGuiSelectableFlags_::ImGuiSelectableFlags_None), das::LineInfo());
		addIEx("NoAutoClosePopups", "ImGuiSelectableFlags_NoAutoClosePopups", int64_t(ImGuiSelectableFlags_::ImGuiSelectableFlags_NoAutoClosePopups), das::LineInfo());
		addIEx("SpanAllColumns", "ImGuiSelectableFlags_SpanAllColumns", int64_t(ImGuiSelectableFlags_::ImGuiSelectableFlags_SpanAllColumns), das::LineInfo());
		addIEx("AllowDoubleClick", "ImGuiSelectableFlags_AllowDoubleClick", int64_t(ImGuiSelectableFlags_::ImGuiSelectableFlags_AllowDoubleClick), das::LineInfo());
		addIEx("Disabled", "ImGuiSelectableFlags_Disabled", int64_t(ImGuiSelectableFlags_::ImGuiSelectableFlags_Disabled), das::LineInfo());
		addIEx("AllowOverlap", "ImGuiSelectableFlags_AllowOverlap", int64_t(ImGuiSelectableFlags_::ImGuiSelectableFlags_AllowOverlap), das::LineInfo());
		addIEx("Highlight", "ImGuiSelectableFlags_Highlight", int64_t(ImGuiSelectableFlags_::ImGuiSelectableFlags_Highlight), das::LineInfo());
	}
};

// from imgui/imgui.h:1282:6
class Enumeration_ImGuiComboFlags_ : public das::Enumeration {
public:
	Enumeration_ImGuiComboFlags_() : das::Enumeration("ImGuiComboFlags") {
		external = true;
		cppName = "ImGuiComboFlags_";
		baseType = (das::Type) das::ToBasicType<int>::type;
		addIEx("None", "ImGuiComboFlags_None", int64_t(ImGuiComboFlags_::ImGuiComboFlags_None), das::LineInfo());
		addIEx("PopupAlignLeft", "ImGuiComboFlags_PopupAlignLeft", int64_t(ImGuiComboFlags_::ImGuiComboFlags_PopupAlignLeft), das::LineInfo());
		addIEx("HeightSmall", "ImGuiComboFlags_HeightSmall", int64_t(ImGuiComboFlags_::ImGuiComboFlags_HeightSmall), das::LineInfo());
		addIEx("HeightRegular", "ImGuiComboFlags_HeightRegular", int64_t(ImGuiComboFlags_::ImGuiComboFlags_HeightRegular), das::LineInfo());
		addIEx("HeightLarge", "ImGuiComboFlags_HeightLarge", int64_t(ImGuiComboFlags_::ImGuiComboFlags_HeightLarge), das::LineInfo());
		addIEx("HeightLargest", "ImGuiComboFlags_HeightLargest", int64_t(ImGuiComboFlags_::ImGuiComboFlags_HeightLargest), das::LineInfo());
		addIEx("NoArrowButton", "ImGuiComboFlags_NoArrowButton", int64_t(ImGuiComboFlags_::ImGuiComboFlags_NoArrowButton), das::LineInfo());
		addIEx("NoPreview", "ImGuiComboFlags_NoPreview", int64_t(ImGuiComboFlags_::ImGuiComboFlags_NoPreview), das::LineInfo());
		addIEx("WidthFitPreview", "ImGuiComboFlags_WidthFitPreview", int64_t(ImGuiComboFlags_::ImGuiComboFlags_WidthFitPreview), das::LineInfo());
		addIEx("HeightMask_", "ImGuiComboFlags_HeightMask_", int64_t(ImGuiComboFlags_::ImGuiComboFlags_HeightMask_), das::LineInfo());
	}
};

// from imgui/imgui.h:1297:6
class Enumeration_ImGuiTabBarFlags_ : public das::Enumeration {
public:
	Enumeration_ImGuiTabBarFlags_() : das::Enumeration("ImGuiTabBarFlags") {
		external = true;
		cppName = "ImGuiTabBarFlags_";
		baseType = (das::Type) das::ToBasicType<int>::type;
		addIEx("None", "ImGuiTabBarFlags_None", int64_t(ImGuiTabBarFlags_::ImGuiTabBarFlags_None), das::LineInfo());
		addIEx("Reorderable", "ImGuiTabBarFlags_Reorderable", int64_t(ImGuiTabBarFlags_::ImGuiTabBarFlags_Reorderable), das::LineInfo());
		addIEx("AutoSelectNewTabs", "ImGuiTabBarFlags_AutoSelectNewTabs", int64_t(ImGuiTabBarFlags_::ImGuiTabBarFlags_AutoSelectNewTabs), das::LineInfo());
		addIEx("TabListPopupButton", "ImGuiTabBarFlags_TabListPopupButton", int64_t(ImGuiTabBarFlags_::ImGuiTabBarFlags_TabListPopupButton), das::LineInfo());
		addIEx("NoCloseWithMiddleMouseButton", "ImGuiTabBarFlags_NoCloseWithMiddleMouseButton", int64_t(ImGuiTabBarFlags_::ImGuiTabBarFlags_NoCloseWithMiddleMouseButton), das::LineInfo());
		addIEx("NoTabListScrollingButtons", "ImGuiTabBarFlags_NoTabListScrollingButtons", int64_t(ImGuiTabBarFlags_::ImGuiTabBarFlags_NoTabListScrollingButtons), das::LineInfo());
		addIEx("NoTooltip", "ImGuiTabBarFlags_NoTooltip", int64_t(ImGuiTabBarFlags_::ImGuiTabBarFlags_NoTooltip), das::LineInfo());
		addIEx("DrawSelectedOverline", "ImGuiTabBarFlags_DrawSelectedOverline", int64_t(ImGuiTabBarFlags_::ImGuiTabBarFlags_DrawSelectedOverline), das::LineInfo());
		addIEx("FittingPolicyResizeDown", "ImGuiTabBarFlags_FittingPolicyResizeDown", int64_t(ImGuiTabBarFlags_::ImGuiTabBarFlags_FittingPolicyResizeDown), das::LineInfo());
		addIEx("FittingPolicyScroll", "ImGuiTabBarFlags_FittingPolicyScroll", int64_t(ImGuiTabBarFlags_::ImGuiTabBarFlags_FittingPolicyScroll), das::LineInfo());
		addIEx("FittingPolicyMask_", "ImGuiTabBarFlags_FittingPolicyMask_", int64_t(ImGuiTabBarFlags_::ImGuiTabBarFlags_FittingPolicyMask_), das::LineInfo());
		addIEx("FittingPolicyDefault_", "ImGuiTabBarFlags_FittingPolicyDefault_", int64_t(ImGuiTabBarFlags_::ImGuiTabBarFlags_FittingPolicyDefault_), das::LineInfo());
	}
};

// from imgui/imgui.h:1314:6
class Enumeration_ImGuiTabItemFlags_ : public das::Enumeration {
public:
	Enumeration_ImGuiTabItemFlags_() : das::Enumeration("ImGuiTabItemFlags") {
		external = true;
		cppName = "ImGuiTabItemFlags_";
		baseType = (das::Type) das::ToBasicType<int>::type;
		addIEx("None", "ImGuiTabItemFlags_None", int64_t(ImGuiTabItemFlags_::ImGuiTabItemFlags_None), das::LineInfo());
		addIEx("UnsavedDocument", "ImGuiTabItemFlags_UnsavedDocument", int64_t(ImGuiTabItemFlags_::ImGuiTabItemFlags_UnsavedDocument), das::LineInfo());
		addIEx("SetSelected", "ImGuiTabItemFlags_SetSelected", int64_t(ImGuiTabItemFlags_::ImGuiTabItemFlags_SetSelected), das::LineInfo());
		addIEx("NoCloseWithMiddleMouseButton", "ImGuiTabItemFlags_NoCloseWithMiddleMouseButton", int64_t(ImGuiTabItemFlags_::ImGuiTabItemFlags_NoCloseWithMiddleMouseButton), das::LineInfo());
		addIEx("NoPushId", "ImGuiTabItemFlags_NoPushId", int64_t(ImGuiTabItemFlags_::ImGuiTabItemFlags_NoPushId), das::LineInfo());
		addIEx("NoTooltip", "ImGuiTabItemFlags_NoTooltip", int64_t(ImGuiTabItemFlags_::ImGuiTabItemFlags_NoTooltip), das::LineInfo());
		addIEx("NoReorder", "ImGuiTabItemFlags_NoReorder", int64_t(ImGuiTabItemFlags_::ImGuiTabItemFlags_NoReorder), das::LineInfo());
		addIEx("Leading", "ImGuiTabItemFlags_Leading", int64_t(ImGuiTabItemFlags_::ImGuiTabItemFlags_Leading), das::LineInfo());
		addIEx("Trailing", "ImGuiTabItemFlags_Trailing", int64_t(ImGuiTabItemFlags_::ImGuiTabItemFlags_Trailing), das::LineInfo());
		addIEx("NoAssumedClosure", "ImGuiTabItemFlags_NoAssumedClosure", int64_t(ImGuiTabItemFlags_::ImGuiTabItemFlags_NoAssumedClosure), das::LineInfo());
	}
};

// from imgui/imgui.h:1329:6
class Enumeration_ImGuiFocusedFlags_ : public das::Enumeration {
public:
	Enumeration_ImGuiFocusedFlags_() : das::Enumeration("ImGuiFocusedFlags") {
		external = true;
		cppName = "ImGuiFocusedFlags_";
		baseType = (das::Type) das::ToBasicType<int>::type;
		addIEx("None", "ImGuiFocusedFlags_None", int64_t(ImGuiFocusedFlags_::ImGuiFocusedFlags_None), das::LineInfo());
		addIEx("ChildWindows", "ImGuiFocusedFlags_ChildWindows", int64_t(ImGuiFocusedFlags_::ImGuiFocusedFlags_ChildWindows), das::LineInfo());
		addIEx("RootWindow", "ImGuiFocusedFlags_RootWindow", int64_t(ImGuiFocusedFlags_::ImGuiFocusedFlags_RootWindow), das::LineInfo());
		addIEx("AnyWindow", "ImGuiFocusedFlags_AnyWindow", int64_t(ImGuiFocusedFlags_::ImGuiFocusedFlags_AnyWindow), das::LineInfo());
		addIEx("NoPopupHierarchy", "ImGuiFocusedFlags_NoPopupHierarchy", int64_t(ImGuiFocusedFlags_::ImGuiFocusedFlags_NoPopupHierarchy), das::LineInfo());
		addIEx("DockHierarchy", "ImGuiFocusedFlags_DockHierarchy", int64_t(ImGuiFocusedFlags_::ImGuiFocusedFlags_DockHierarchy), das::LineInfo());
		addIEx("RootAndChildWindows", "ImGuiFocusedFlags_RootAndChildWindows", int64_t(ImGuiFocusedFlags_::ImGuiFocusedFlags_RootAndChildWindows), das::LineInfo());
	}
};

// from imgui/imgui.h:1343:6
class Enumeration_ImGuiHoveredFlags_ : public das::Enumeration {
public:
	Enumeration_ImGuiHoveredFlags_() : das::Enumeration("ImGuiHoveredFlags") {
		external = true;
		cppName = "ImGuiHoveredFlags_";
		baseType = (das::Type) das::ToBasicType<int>::type;
		addIEx("None", "ImGuiHoveredFlags_None", int64_t(ImGuiHoveredFlags_::ImGuiHoveredFlags_None), das::LineInfo());
		addIEx("ChildWindows", "ImGuiHoveredFlags_ChildWindows", int64_t(ImGuiHoveredFlags_::ImGuiHoveredFlags_ChildWindows), das::LineInfo());
		addIEx("RootWindow", "ImGuiHoveredFlags_RootWindow", int64_t(ImGuiHoveredFlags_::ImGuiHoveredFlags_RootWindow), das::LineInfo());
		addIEx("AnyWindow", "ImGuiHoveredFlags_AnyWindow", int64_t(ImGuiHoveredFlags_::ImGuiHoveredFlags_AnyWindow), das::LineInfo());
		addIEx("NoPopupHierarchy", "ImGuiHoveredFlags_NoPopupHierarchy", int64_t(ImGuiHoveredFlags_::ImGuiHoveredFlags_NoPopupHierarchy), das::LineInfo());
		addIEx("DockHierarchy", "ImGuiHoveredFlags_DockHierarchy", int64_t(ImGuiHoveredFlags_::ImGuiHoveredFlags_DockHierarchy), das::LineInfo());
		addIEx("AllowWhenBlockedByPopup", "ImGuiHoveredFlags_AllowWhenBlockedByPopup", int64_t(ImGuiHoveredFlags_::ImGuiHoveredFlags_AllowWhenBlockedByPopup), das::LineInfo());
		addIEx("AllowWhenBlockedByActiveItem", "ImGuiHoveredFlags_AllowWhenBlockedByActiveItem", int64_t(ImGuiHoveredFlags_::ImGuiHoveredFlags_AllowWhenBlockedByActiveItem), das::LineInfo());
		addIEx("AllowWhenOverlappedByItem", "ImGuiHoveredFlags_AllowWhenOverlappedByItem", int64_t(ImGuiHoveredFlags_::ImGuiHoveredFlags_AllowWhenOverlappedByItem), das::LineInfo());
		addIEx("AllowWhenOverlappedByWindow", "ImGuiHoveredFlags_AllowWhenOverlappedByWindow", int64_t(ImGuiHoveredFlags_::ImGuiHoveredFlags_AllowWhenOverlappedByWindow), das::LineInfo());
		addIEx("AllowWhenDisabled", "ImGuiHoveredFlags_AllowWhenDisabled", int64_t(ImGuiHoveredFlags_::ImGuiHoveredFlags_AllowWhenDisabled), das::LineInfo());
		addIEx("NoNavOverride", "ImGuiHoveredFlags_NoNavOverride", int64_t(ImGuiHoveredFlags_::ImGuiHoveredFlags_NoNavOverride), das::LineInfo());
		addIEx("AllowWhenOverlapped", "ImGuiHoveredFlags_AllowWhenOverlapped", int64_t(ImGuiHoveredFlags_::ImGuiHoveredFlags_AllowWhenOverlapped), das::LineInfo());
		addIEx("RectOnly", "ImGuiHoveredFlags_RectOnly", int64_t(ImGuiHoveredFlags_::ImGuiHoveredFlags_RectOnly), das::LineInfo());
		addIEx("RootAndChildWindows", "ImGuiHoveredFlags_RootAndChildWindows", int64_t(ImGuiHoveredFlags_::ImGuiHoveredFlags_RootAndChildWindows), das::LineInfo());
		addIEx("ForTooltip", "ImGuiHoveredFlags_ForTooltip", int64_t(ImGuiHoveredFlags_::ImGuiHoveredFlags_ForTooltip), das::LineInfo());
		addIEx("Stationary", "ImGuiHoveredFlags_Stationary", int64_t(ImGuiHoveredFlags_::ImGuiHoveredFlags_Stationary), das::LineInfo());
		addIEx("DelayNone", "ImGuiHoveredFlags_DelayNone", int64_t(ImGuiHoveredFlags_::ImGuiHoveredFlags_DelayNone), das::LineInfo());
		addIEx("DelayShort", "ImGuiHoveredFlags_DelayShort", int64_t(ImGuiHoveredFlags_::ImGuiHoveredFlags_DelayShort), das::LineInfo());
		addIEx("DelayNormal", "ImGuiHoveredFlags_DelayNormal", int64_t(ImGuiHoveredFlags_::ImGuiHoveredFlags_DelayNormal), das::LineInfo());
		addIEx("NoSharedDelay", "ImGuiHoveredFlags_NoSharedDelay", int64_t(ImGuiHoveredFlags_::ImGuiHoveredFlags_NoSharedDelay), das::LineInfo());
	}
};

// from imgui/imgui.h:1383:6
class Enumeration_ImGuiDockNodeFlags_ : public das::Enumeration {
public:
	Enumeration_ImGuiDockNodeFlags_() : das::Enumeration("ImGuiDockNodeFlags") {
		external = true;
		cppName = "ImGuiDockNodeFlags_";
		baseType = (das::Type) das::ToBasicType<int>::type;
		addIEx("None", "ImGuiDockNodeFlags_None", int64_t(ImGuiDockNodeFlags_::ImGuiDockNodeFlags_None), das::LineInfo());
		addIEx("KeepAliveOnly", "ImGuiDockNodeFlags_KeepAliveOnly", int64_t(ImGuiDockNodeFlags_::ImGuiDockNodeFlags_KeepAliveOnly), das::LineInfo());
		addIEx("NoDockingOverCentralNode", "ImGuiDockNodeFlags_NoDockingOverCentralNode", int64_t(ImGuiDockNodeFlags_::ImGuiDockNodeFlags_NoDockingOverCentralNode), das::LineInfo());
		addIEx("PassthruCentralNode", "ImGuiDockNodeFlags_PassthruCentralNode", int64_t(ImGuiDockNodeFlags_::ImGuiDockNodeFlags_PassthruCentralNode), das::LineInfo());
		addIEx("NoDockingSplit", "ImGuiDockNodeFlags_NoDockingSplit", int64_t(ImGuiDockNodeFlags_::ImGuiDockNodeFlags_NoDockingSplit), das::LineInfo());
		addIEx("NoResize", "ImGuiDockNodeFlags_NoResize", int64_t(ImGuiDockNodeFlags_::ImGuiDockNodeFlags_NoResize), das::LineInfo());
		addIEx("AutoHideTabBar", "ImGuiDockNodeFlags_AutoHideTabBar", int64_t(ImGuiDockNodeFlags_::ImGuiDockNodeFlags_AutoHideTabBar), das::LineInfo());
		addIEx("NoUndocking", "ImGuiDockNodeFlags_NoUndocking", int64_t(ImGuiDockNodeFlags_::ImGuiDockNodeFlags_NoUndocking), das::LineInfo());
	}
};

// from imgui/imgui.h:1402:6
class Enumeration_ImGuiDragDropFlags_ : public das::Enumeration {
public:
	Enumeration_ImGuiDragDropFlags_() : das::Enumeration("ImGuiDragDropFlags") {
		external = true;
		cppName = "ImGuiDragDropFlags_";
		baseType = (das::Type) das::ToBasicType<int>::type;
		addIEx("None", "ImGuiDragDropFlags_None", int64_t(ImGuiDragDropFlags_::ImGuiDragDropFlags_None), das::LineInfo());
		addIEx("SourceNoPreviewTooltip", "ImGuiDragDropFlags_SourceNoPreviewTooltip", int64_t(ImGuiDragDropFlags_::ImGuiDragDropFlags_SourceNoPreviewTooltip), das::LineInfo());
		addIEx("SourceNoDisableHover", "ImGuiDragDropFlags_SourceNoDisableHover", int64_t(ImGuiDragDropFlags_::ImGuiDragDropFlags_SourceNoDisableHover), das::LineInfo());
		addIEx("SourceNoHoldToOpenOthers", "ImGuiDragDropFlags_SourceNoHoldToOpenOthers", int64_t(ImGuiDragDropFlags_::ImGuiDragDropFlags_SourceNoHoldToOpenOthers), das::LineInfo());
		addIEx("SourceAllowNullID", "ImGuiDragDropFlags_SourceAllowNullID", int64_t(ImGuiDragDropFlags_::ImGuiDragDropFlags_SourceAllowNullID), das::LineInfo());
		addIEx("SourceExtern", "ImGuiDragDropFlags_SourceExtern", int64_t(ImGuiDragDropFlags_::ImGuiDragDropFlags_SourceExtern), das::LineInfo());
		addIEx("PayloadAutoExpire", "ImGuiDragDropFlags_PayloadAutoExpire", int64_t(ImGuiDragDropFlags_::ImGuiDragDropFlags_PayloadAutoExpire), das::LineInfo());
		addIEx("PayloadNoCrossContext", "ImGuiDragDropFlags_PayloadNoCrossContext", int64_t(ImGuiDragDropFlags_::ImGuiDragDropFlags_PayloadNoCrossContext), das::LineInfo());
		addIEx("PayloadNoCrossProcess", "ImGuiDragDropFlags_PayloadNoCrossProcess", int64_t(ImGuiDragDropFlags_::ImGuiDragDropFlags_PayloadNoCrossProcess), das::LineInfo());
		addIEx("AcceptBeforeDelivery", "ImGuiDragDropFlags_AcceptBeforeDelivery", int64_t(ImGuiDragDropFlags_::ImGuiDragDropFlags_AcceptBeforeDelivery), das::LineInfo());
		addIEx("AcceptNoDrawDefaultRect", "ImGuiDragDropFlags_AcceptNoDrawDefaultRect", int64_t(ImGuiDragDropFlags_::ImGuiDragDropFlags_AcceptNoDrawDefaultRect), das::LineInfo());
		addIEx("AcceptNoPreviewTooltip", "ImGuiDragDropFlags_AcceptNoPreviewTooltip", int64_t(ImGuiDragDropFlags_::ImGuiDragDropFlags_AcceptNoPreviewTooltip), das::LineInfo());
		addIEx("AcceptPeekOnly", "ImGuiDragDropFlags_AcceptPeekOnly", int64_t(ImGuiDragDropFlags_::ImGuiDragDropFlags_AcceptPeekOnly), das::LineInfo());
	}
};

// from imgui/imgui.h:1430:6
class Enumeration_ImGuiDataType_ : public das::Enumeration {
public:
	Enumeration_ImGuiDataType_() : das::Enumeration("ImGuiDataType") {
		external = true;
		cppName = "ImGuiDataType_";
		baseType = (das::Type) das::ToBasicType<int>::type;
		addIEx("S8", "ImGuiDataType_S8", int64_t(ImGuiDataType_::ImGuiDataType_S8), das::LineInfo());
		addIEx("U8", "ImGuiDataType_U8", int64_t(ImGuiDataType_::ImGuiDataType_U8), das::LineInfo());
		addIEx("S16", "ImGuiDataType_S16", int64_t(ImGuiDataType_::ImGuiDataType_S16), das::LineInfo());
		addIEx("U16", "ImGuiDataType_U16", int64_t(ImGuiDataType_::ImGuiDataType_U16), das::LineInfo());
		addIEx("S32", "ImGuiDataType_S32", int64_t(ImGuiDataType_::ImGuiDataType_S32), das::LineInfo());
		addIEx("U32", "ImGuiDataType_U32", int64_t(ImGuiDataType_::ImGuiDataType_U32), das::LineInfo());
		addIEx("S64", "ImGuiDataType_S64", int64_t(ImGuiDataType_::ImGuiDataType_S64), das::LineInfo());
		addIEx("U64", "ImGuiDataType_U64", int64_t(ImGuiDataType_::ImGuiDataType_U64), das::LineInfo());
		addIEx("Float", "ImGuiDataType_Float", int64_t(ImGuiDataType_::ImGuiDataType_Float), das::LineInfo());
		addIEx("Double", "ImGuiDataType_Double", int64_t(ImGuiDataType_::ImGuiDataType_Double), das::LineInfo());
		addIEx("Bool", "ImGuiDataType_Bool", int64_t(ImGuiDataType_::ImGuiDataType_Bool), das::LineInfo());
		addIEx("COUNT", "ImGuiDataType_COUNT", int64_t(ImGuiDataType_::ImGuiDataType_COUNT), das::LineInfo());
	}
};

// from imgui/imgui.h:1447:6
class Enumeration_ImGuiDir : public das::Enumeration {
public:
	Enumeration_ImGuiDir() : das::Enumeration("ImGuiDir") {
		external = true;
		cppName = "ImGuiDir";
		baseType = (das::Type) das::ToBasicType<int>::type;
		addIEx("None", "ImGuiDir_None", int64_t(ImGuiDir::ImGuiDir_None), das::LineInfo());
		addIEx("Left", "ImGuiDir_Left", int64_t(ImGuiDir::ImGuiDir_Left), das::LineInfo());
		addIEx("Right", "ImGuiDir_Right", int64_t(ImGuiDir::ImGuiDir_Right), das::LineInfo());
		addIEx("Up", "ImGuiDir_Up", int64_t(ImGuiDir::ImGuiDir_Up), das::LineInfo());
		addIEx("Down", "ImGuiDir_Down", int64_t(ImGuiDir::ImGuiDir_Down), das::LineInfo());
		addIEx("COUNT", "ImGuiDir_COUNT", int64_t(ImGuiDir::ImGuiDir_COUNT), das::LineInfo());
	}
};

// from imgui/imgui.h:1458:6
class Enumeration_ImGuiSortDirection : public das::Enumeration {
public:
	Enumeration_ImGuiSortDirection() : das::Enumeration("ImGuiSortDirection") {
		external = true;
		cppName = "ImGuiSortDirection";
		baseType = (das::Type) das::ToBasicType<ImU8>::type;
		addIEx("None", "ImGuiSortDirection_None", int64_t(ImGuiSortDirection::ImGuiSortDirection_None), das::LineInfo());
		addIEx("Ascending", "ImGuiSortDirection_Ascending", int64_t(ImGuiSortDirection::ImGuiSortDirection_Ascending), das::LineInfo());
		addIEx("Descending", "ImGuiSortDirection_Descending", int64_t(ImGuiSortDirection::ImGuiSortDirection_Descending), das::LineInfo());
	}
};

// from imgui/imgui.h:1476:6
class Enumeration_ImGuiKey : public das::Enumeration {
public:
	Enumeration_ImGuiKey() : das::Enumeration("ImGuiKey") {
		external = true;
		cppName = "ImGuiKey";
		baseType = (das::Type) das::ToBasicType<int>::type;
		addIEx("None", "ImGuiKey_None", int64_t(ImGuiKey::ImGuiKey_None), das::LineInfo());
		addIEx("Tab", "ImGuiKey_Tab", int64_t(ImGuiKey::ImGuiKey_Tab), das::LineInfo());
		addIEx("LeftArrow", "ImGuiKey_LeftArrow", int64_t(ImGuiKey::ImGuiKey_LeftArrow), das::LineInfo());
		addIEx("RightArrow", "ImGuiKey_RightArrow", int64_t(ImGuiKey::ImGuiKey_RightArrow), das::LineInfo());
		addIEx("UpArrow", "ImGuiKey_UpArrow", int64_t(ImGuiKey::ImGuiKey_UpArrow), das::LineInfo());
		addIEx("DownArrow", "ImGuiKey_DownArrow", int64_t(ImGuiKey::ImGuiKey_DownArrow), das::LineInfo());
		addIEx("PageUp", "ImGuiKey_PageUp", int64_t(ImGuiKey::ImGuiKey_PageUp), das::LineInfo());
		addIEx("PageDown", "ImGuiKey_PageDown", int64_t(ImGuiKey::ImGuiKey_PageDown), das::LineInfo());
		addIEx("Home", "ImGuiKey_Home", int64_t(ImGuiKey::ImGuiKey_Home), das::LineInfo());
		addIEx("End", "ImGuiKey_End", int64_t(ImGuiKey::ImGuiKey_End), das::LineInfo());
		addIEx("Insert", "ImGuiKey_Insert", int64_t(ImGuiKey::ImGuiKey_Insert), das::LineInfo());
		addIEx("Delete", "ImGuiKey_Delete", int64_t(ImGuiKey::ImGuiKey_Delete), das::LineInfo());
		addIEx("Backspace", "ImGuiKey_Backspace", int64_t(ImGuiKey::ImGuiKey_Backspace), das::LineInfo());
		addIEx("Space", "ImGuiKey_Space", int64_t(ImGuiKey::ImGuiKey_Space), das::LineInfo());
		addIEx("Enter", "ImGuiKey_Enter", int64_t(ImGuiKey::ImGuiKey_Enter), das::LineInfo());
		addIEx("Escape", "ImGuiKey_Escape", int64_t(ImGuiKey::ImGuiKey_Escape), das::LineInfo());
		addIEx("LeftCtrl", "ImGuiKey_LeftCtrl", int64_t(ImGuiKey::ImGuiKey_LeftCtrl), das::LineInfo());
		addIEx("LeftShift", "ImGuiKey_LeftShift", int64_t(ImGuiKey::ImGuiKey_LeftShift), das::LineInfo());
		addIEx("LeftAlt", "ImGuiKey_LeftAlt", int64_t(ImGuiKey::ImGuiKey_LeftAlt), das::LineInfo());
		addIEx("LeftSuper", "ImGuiKey_LeftSuper", int64_t(ImGuiKey::ImGuiKey_LeftSuper), das::LineInfo());
		addIEx("RightCtrl", "ImGuiKey_RightCtrl", int64_t(ImGuiKey::ImGuiKey_RightCtrl), das::LineInfo());
		addIEx("RightShift", "ImGuiKey_RightShift", int64_t(ImGuiKey::ImGuiKey_RightShift), das::LineInfo());
		addIEx("RightAlt", "ImGuiKey_RightAlt", int64_t(ImGuiKey::ImGuiKey_RightAlt), das::LineInfo());
		addIEx("RightSuper", "ImGuiKey_RightSuper", int64_t(ImGuiKey::ImGuiKey_RightSuper), das::LineInfo());
		addIEx("Menu", "ImGuiKey_Menu", int64_t(ImGuiKey::ImGuiKey_Menu), das::LineInfo());
		addIEx("0", "ImGuiKey_0", int64_t(ImGuiKey::ImGuiKey_0), das::LineInfo());
		addIEx("1", "ImGuiKey_1", int64_t(ImGuiKey::ImGuiKey_1), das::LineInfo());
		addIEx("2", "ImGuiKey_2", int64_t(ImGuiKey::ImGuiKey_2), das::LineInfo());
		addIEx("3", "ImGuiKey_3", int64_t(ImGuiKey::ImGuiKey_3), das::LineInfo());
		addIEx("4", "ImGuiKey_4", int64_t(ImGuiKey::ImGuiKey_4), das::LineInfo());
		addIEx("5", "ImGuiKey_5", int64_t(ImGuiKey::ImGuiKey_5), das::LineInfo());
		addIEx("6", "ImGuiKey_6", int64_t(ImGuiKey::ImGuiKey_6), das::LineInfo());
		addIEx("7", "ImGuiKey_7", int64_t(ImGuiKey::ImGuiKey_7), das::LineInfo());
		addIEx("8", "ImGuiKey_8", int64_t(ImGuiKey::ImGuiKey_8), das::LineInfo());
		addIEx("9", "ImGuiKey_9", int64_t(ImGuiKey::ImGuiKey_9), das::LineInfo());
		addIEx("A", "ImGuiKey_A", int64_t(ImGuiKey::ImGuiKey_A), das::LineInfo());
		addIEx("B", "ImGuiKey_B", int64_t(ImGuiKey::ImGuiKey_B), das::LineInfo());
		addIEx("C", "ImGuiKey_C", int64_t(ImGuiKey::ImGuiKey_C), das::LineInfo());
		addIEx("D", "ImGuiKey_D", int64_t(ImGuiKey::ImGuiKey_D), das::LineInfo());
		addIEx("E", "ImGuiKey_E", int64_t(ImGuiKey::ImGuiKey_E), das::LineInfo());
		addIEx("F", "ImGuiKey_F", int64_t(ImGuiKey::ImGuiKey_F), das::LineInfo());
		addIEx("G", "ImGuiKey_G", int64_t(ImGuiKey::ImGuiKey_G), das::LineInfo());
		addIEx("H", "ImGuiKey_H", int64_t(ImGuiKey::ImGuiKey_H), das::LineInfo());
		addIEx("I", "ImGuiKey_I", int64_t(ImGuiKey::ImGuiKey_I), das::LineInfo());
		addIEx("J", "ImGuiKey_J", int64_t(ImGuiKey::ImGuiKey_J), das::LineInfo());
		addIEx("K", "ImGuiKey_K", int64_t(ImGuiKey::ImGuiKey_K), das::LineInfo());
		addIEx("L", "ImGuiKey_L", int64_t(ImGuiKey::ImGuiKey_L), das::LineInfo());
		addIEx("M", "ImGuiKey_M", int64_t(ImGuiKey::ImGuiKey_M), das::LineInfo());
		addIEx("N", "ImGuiKey_N", int64_t(ImGuiKey::ImGuiKey_N), das::LineInfo());
		addIEx("O", "ImGuiKey_O", int64_t(ImGuiKey::ImGuiKey_O), das::LineInfo());
		addIEx("P", "ImGuiKey_P", int64_t(ImGuiKey::ImGuiKey_P), das::LineInfo());
		addIEx("Q", "ImGuiKey_Q", int64_t(ImGuiKey::ImGuiKey_Q), das::LineInfo());
		addIEx("R", "ImGuiKey_R", int64_t(ImGuiKey::ImGuiKey_R), das::LineInfo());
		addIEx("S", "ImGuiKey_S", int64_t(ImGuiKey::ImGuiKey_S), das::LineInfo());
		addIEx("T", "ImGuiKey_T", int64_t(ImGuiKey::ImGuiKey_T), das::LineInfo());
		addIEx("U", "ImGuiKey_U", int64_t(ImGuiKey::ImGuiKey_U), das::LineInfo());
		addIEx("V", "ImGuiKey_V", int64_t(ImGuiKey::ImGuiKey_V), das::LineInfo());
		addIEx("W", "ImGuiKey_W", int64_t(ImGuiKey::ImGuiKey_W), das::LineInfo());
		addIEx("X", "ImGuiKey_X", int64_t(ImGuiKey::ImGuiKey_X), das::LineInfo());
		addIEx("Y", "ImGuiKey_Y", int64_t(ImGuiKey::ImGuiKey_Y), das::LineInfo());
		addIEx("Z", "ImGuiKey_Z", int64_t(ImGuiKey::ImGuiKey_Z), das::LineInfo());
		addIEx("F1", "ImGuiKey_F1", int64_t(ImGuiKey::ImGuiKey_F1), das::LineInfo());
		addIEx("F2", "ImGuiKey_F2", int64_t(ImGuiKey::ImGuiKey_F2), das::LineInfo());
		addIEx("F3", "ImGuiKey_F3", int64_t(ImGuiKey::ImGuiKey_F3), das::LineInfo());
		addIEx("F4", "ImGuiKey_F4", int64_t(ImGuiKey::ImGuiKey_F4), das::LineInfo());
		addIEx("F5", "ImGuiKey_F5", int64_t(ImGuiKey::ImGuiKey_F5), das::LineInfo());
		addIEx("F6", "ImGuiKey_F6", int64_t(ImGuiKey::ImGuiKey_F6), das::LineInfo());
		addIEx("F7", "ImGuiKey_F7", int64_t(ImGuiKey::ImGuiKey_F7), das::LineInfo());
		addIEx("F8", "ImGuiKey_F8", int64_t(ImGuiKey::ImGuiKey_F8), das::LineInfo());
		addIEx("F9", "ImGuiKey_F9", int64_t(ImGuiKey::ImGuiKey_F9), das::LineInfo());
		addIEx("F10", "ImGuiKey_F10", int64_t(ImGuiKey::ImGuiKey_F10), das::LineInfo());
		addIEx("F11", "ImGuiKey_F11", int64_t(ImGuiKey::ImGuiKey_F11), das::LineInfo());
		addIEx("F12", "ImGuiKey_F12", int64_t(ImGuiKey::ImGuiKey_F12), das::LineInfo());
		addIEx("F13", "ImGuiKey_F13", int64_t(ImGuiKey::ImGuiKey_F13), das::LineInfo());
		addIEx("F14", "ImGuiKey_F14", int64_t(ImGuiKey::ImGuiKey_F14), das::LineInfo());
		addIEx("F15", "ImGuiKey_F15", int64_t(ImGuiKey::ImGuiKey_F15), das::LineInfo());
		addIEx("F16", "ImGuiKey_F16", int64_t(ImGuiKey::ImGuiKey_F16), das::LineInfo());
		addIEx("F17", "ImGuiKey_F17", int64_t(ImGuiKey::ImGuiKey_F17), das::LineInfo());
		addIEx("F18", "ImGuiKey_F18", int64_t(ImGuiKey::ImGuiKey_F18), das::LineInfo());
		addIEx("F19", "ImGuiKey_F19", int64_t(ImGuiKey::ImGuiKey_F19), das::LineInfo());
		addIEx("F20", "ImGuiKey_F20", int64_t(ImGuiKey::ImGuiKey_F20), das::LineInfo());
		addIEx("F21", "ImGuiKey_F21", int64_t(ImGuiKey::ImGuiKey_F21), das::LineInfo());
		addIEx("F22", "ImGuiKey_F22", int64_t(ImGuiKey::ImGuiKey_F22), das::LineInfo());
		addIEx("F23", "ImGuiKey_F23", int64_t(ImGuiKey::ImGuiKey_F23), das::LineInfo());
		addIEx("F24", "ImGuiKey_F24", int64_t(ImGuiKey::ImGuiKey_F24), das::LineInfo());
		addIEx("Apostrophe", "ImGuiKey_Apostrophe", int64_t(ImGuiKey::ImGuiKey_Apostrophe), das::LineInfo());
		addIEx("Comma", "ImGuiKey_Comma", int64_t(ImGuiKey::ImGuiKey_Comma), das::LineInfo());
		addIEx("Minus", "ImGuiKey_Minus", int64_t(ImGuiKey::ImGuiKey_Minus), das::LineInfo());
		addIEx("Period", "ImGuiKey_Period", int64_t(ImGuiKey::ImGuiKey_Period), das::LineInfo());
		addIEx("Slash", "ImGuiKey_Slash", int64_t(ImGuiKey::ImGuiKey_Slash), das::LineInfo());
		addIEx("Semicolon", "ImGuiKey_Semicolon", int64_t(ImGuiKey::ImGuiKey_Semicolon), das::LineInfo());
		addIEx("Equal", "ImGuiKey_Equal", int64_t(ImGuiKey::ImGuiKey_Equal), das::LineInfo());
		addIEx("LeftBracket", "ImGuiKey_LeftBracket", int64_t(ImGuiKey::ImGuiKey_LeftBracket), das::LineInfo());
		addIEx("Backslash", "ImGuiKey_Backslash", int64_t(ImGuiKey::ImGuiKey_Backslash), das::LineInfo());
		addIEx("RightBracket", "ImGuiKey_RightBracket", int64_t(ImGuiKey::ImGuiKey_RightBracket), das::LineInfo());
		addIEx("GraveAccent", "ImGuiKey_GraveAccent", int64_t(ImGuiKey::ImGuiKey_GraveAccent), das::LineInfo());
		addIEx("CapsLock", "ImGuiKey_CapsLock", int64_t(ImGuiKey::ImGuiKey_CapsLock), das::LineInfo());
		addIEx("ScrollLock", "ImGuiKey_ScrollLock", int64_t(ImGuiKey::ImGuiKey_ScrollLock), das::LineInfo());
		addIEx("NumLock", "ImGuiKey_NumLock", int64_t(ImGuiKey::ImGuiKey_NumLock), das::LineInfo());
		addIEx("PrintScreen", "ImGuiKey_PrintScreen", int64_t(ImGuiKey::ImGuiKey_PrintScreen), das::LineInfo());
		addIEx("Pause", "ImGuiKey_Pause", int64_t(ImGuiKey::ImGuiKey_Pause), das::LineInfo());
		addIEx("Keypad0", "ImGuiKey_Keypad0", int64_t(ImGuiKey::ImGuiKey_Keypad0), das::LineInfo());
		addIEx("Keypad1", "ImGuiKey_Keypad1", int64_t(ImGuiKey::ImGuiKey_Keypad1), das::LineInfo());
		addIEx("Keypad2", "ImGuiKey_Keypad2", int64_t(ImGuiKey::ImGuiKey_Keypad2), das::LineInfo());
		addIEx("Keypad3", "ImGuiKey_Keypad3", int64_t(ImGuiKey::ImGuiKey_Keypad3), das::LineInfo());
		addIEx("Keypad4", "ImGuiKey_Keypad4", int64_t(ImGuiKey::ImGuiKey_Keypad4), das::LineInfo());
		addIEx("Keypad5", "ImGuiKey_Keypad5", int64_t(ImGuiKey::ImGuiKey_Keypad5), das::LineInfo());
		addIEx("Keypad6", "ImGuiKey_Keypad6", int64_t(ImGuiKey::ImGuiKey_Keypad6), das::LineInfo());
		addIEx("Keypad7", "ImGuiKey_Keypad7", int64_t(ImGuiKey::ImGuiKey_Keypad7), das::LineInfo());
		addIEx("Keypad8", "ImGuiKey_Keypad8", int64_t(ImGuiKey::ImGuiKey_Keypad8), das::LineInfo());
		addIEx("Keypad9", "ImGuiKey_Keypad9", int64_t(ImGuiKey::ImGuiKey_Keypad9), das::LineInfo());
		addIEx("KeypadDecimal", "ImGuiKey_KeypadDecimal", int64_t(ImGuiKey::ImGuiKey_KeypadDecimal), das::LineInfo());
		addIEx("KeypadDivide", "ImGuiKey_KeypadDivide", int64_t(ImGuiKey::ImGuiKey_KeypadDivide), das::LineInfo());
		addIEx("KeypadMultiply", "ImGuiKey_KeypadMultiply", int64_t(ImGuiKey::ImGuiKey_KeypadMultiply), das::LineInfo());
		addIEx("KeypadSubtract", "ImGuiKey_KeypadSubtract", int64_t(ImGuiKey::ImGuiKey_KeypadSubtract), das::LineInfo());
		addIEx("KeypadAdd", "ImGuiKey_KeypadAdd", int64_t(ImGuiKey::ImGuiKey_KeypadAdd), das::LineInfo());
		addIEx("KeypadEnter", "ImGuiKey_KeypadEnter", int64_t(ImGuiKey::ImGuiKey_KeypadEnter), das::LineInfo());
		addIEx("KeypadEqual", "ImGuiKey_KeypadEqual", int64_t(ImGuiKey::ImGuiKey_KeypadEqual), das::LineInfo());
		addIEx("AppBack", "ImGuiKey_AppBack", int64_t(ImGuiKey::ImGuiKey_AppBack), das::LineInfo());
		addIEx("AppForward", "ImGuiKey_AppForward", int64_t(ImGuiKey::ImGuiKey_AppForward), das::LineInfo());
		addIEx("GamepadStart", "ImGuiKey_GamepadStart", int64_t(ImGuiKey::ImGuiKey_GamepadStart), das::LineInfo());
		addIEx("GamepadBack", "ImGuiKey_GamepadBack", int64_t(ImGuiKey::ImGuiKey_GamepadBack), das::LineInfo());
		addIEx("GamepadFaceLeft", "ImGuiKey_GamepadFaceLeft", int64_t(ImGuiKey::ImGuiKey_GamepadFaceLeft), das::LineInfo());
		addIEx("GamepadFaceRight", "ImGuiKey_GamepadFaceRight", int64_t(ImGuiKey::ImGuiKey_GamepadFaceRight), das::LineInfo());
		addIEx("GamepadFaceUp", "ImGuiKey_GamepadFaceUp", int64_t(ImGuiKey::ImGuiKey_GamepadFaceUp), das::LineInfo());
		addIEx("GamepadFaceDown", "ImGuiKey_GamepadFaceDown", int64_t(ImGuiKey::ImGuiKey_GamepadFaceDown), das::LineInfo());
		addIEx("GamepadDpadLeft", "ImGuiKey_GamepadDpadLeft", int64_t(ImGuiKey::ImGuiKey_GamepadDpadLeft), das::LineInfo());
		addIEx("GamepadDpadRight", "ImGuiKey_GamepadDpadRight", int64_t(ImGuiKey::ImGuiKey_GamepadDpadRight), das::LineInfo());
		addIEx("GamepadDpadUp", "ImGuiKey_GamepadDpadUp", int64_t(ImGuiKey::ImGuiKey_GamepadDpadUp), das::LineInfo());
		addIEx("GamepadDpadDown", "ImGuiKey_GamepadDpadDown", int64_t(ImGuiKey::ImGuiKey_GamepadDpadDown), das::LineInfo());
		addIEx("GamepadL1", "ImGuiKey_GamepadL1", int64_t(ImGuiKey::ImGuiKey_GamepadL1), das::LineInfo());
		addIEx("GamepadR1", "ImGuiKey_GamepadR1", int64_t(ImGuiKey::ImGuiKey_GamepadR1), das::LineInfo());
		addIEx("GamepadL2", "ImGuiKey_GamepadL2", int64_t(ImGuiKey::ImGuiKey_GamepadL2), das::LineInfo());
		addIEx("GamepadR2", "ImGuiKey_GamepadR2", int64_t(ImGuiKey::ImGuiKey_GamepadR2), das::LineInfo());
		addIEx("GamepadL3", "ImGuiKey_GamepadL3", int64_t(ImGuiKey::ImGuiKey_GamepadL3), das::LineInfo());
		addIEx("GamepadR3", "ImGuiKey_GamepadR3", int64_t(ImGuiKey::ImGuiKey_GamepadR3), das::LineInfo());
		addIEx("GamepadLStickLeft", "ImGuiKey_GamepadLStickLeft", int64_t(ImGuiKey::ImGuiKey_GamepadLStickLeft), das::LineInfo());
		addIEx("GamepadLStickRight", "ImGuiKey_GamepadLStickRight", int64_t(ImGuiKey::ImGuiKey_GamepadLStickRight), das::LineInfo());
		addIEx("GamepadLStickUp", "ImGuiKey_GamepadLStickUp", int64_t(ImGuiKey::ImGuiKey_GamepadLStickUp), das::LineInfo());
		addIEx("GamepadLStickDown", "ImGuiKey_GamepadLStickDown", int64_t(ImGuiKey::ImGuiKey_GamepadLStickDown), das::LineInfo());
		addIEx("GamepadRStickLeft", "ImGuiKey_GamepadRStickLeft", int64_t(ImGuiKey::ImGuiKey_GamepadRStickLeft), das::LineInfo());
		addIEx("GamepadRStickRight", "ImGuiKey_GamepadRStickRight", int64_t(ImGuiKey::ImGuiKey_GamepadRStickRight), das::LineInfo());
		addIEx("GamepadRStickUp", "ImGuiKey_GamepadRStickUp", int64_t(ImGuiKey::ImGuiKey_GamepadRStickUp), das::LineInfo());
		addIEx("GamepadRStickDown", "ImGuiKey_GamepadRStickDown", int64_t(ImGuiKey::ImGuiKey_GamepadRStickDown), das::LineInfo());
		addIEx("MouseLeft", "ImGuiKey_MouseLeft", int64_t(ImGuiKey::ImGuiKey_MouseLeft), das::LineInfo());
		addIEx("MouseRight", "ImGuiKey_MouseRight", int64_t(ImGuiKey::ImGuiKey_MouseRight), das::LineInfo());
		addIEx("MouseMiddle", "ImGuiKey_MouseMiddle", int64_t(ImGuiKey::ImGuiKey_MouseMiddle), das::LineInfo());
		addIEx("MouseX1", "ImGuiKey_MouseX1", int64_t(ImGuiKey::ImGuiKey_MouseX1), das::LineInfo());
		addIEx("MouseX2", "ImGuiKey_MouseX2", int64_t(ImGuiKey::ImGuiKey_MouseX2), das::LineInfo());
		addIEx("MouseWheelX", "ImGuiKey_MouseWheelX", int64_t(ImGuiKey::ImGuiKey_MouseWheelX), das::LineInfo());
		addIEx("MouseWheelY", "ImGuiKey_MouseWheelY", int64_t(ImGuiKey::ImGuiKey_MouseWheelY), das::LineInfo());
		addIEx("ReservedForModCtrl", "ImGuiKey_ReservedForModCtrl", int64_t(ImGuiKey::ImGuiKey_ReservedForModCtrl), das::LineInfo());
		addIEx("ReservedForModShift", "ImGuiKey_ReservedForModShift", int64_t(ImGuiKey::ImGuiKey_ReservedForModShift), das::LineInfo());
		addIEx("ReservedForModAlt", "ImGuiKey_ReservedForModAlt", int64_t(ImGuiKey::ImGuiKey_ReservedForModAlt), das::LineInfo());
		addIEx("ReservedForModSuper", "ImGuiKey_ReservedForModSuper", int64_t(ImGuiKey::ImGuiKey_ReservedForModSuper), das::LineInfo());
		addIEx("COUNT", "ImGuiKey_COUNT", int64_t(ImGuiKey::ImGuiKey_COUNT), das::LineInfo());
		addIEx("ImGuiMod_None", "ImGuiMod_None", int64_t(ImGuiKey::ImGuiMod_None), das::LineInfo());
		addIEx("ImGuiMod_Ctrl", "ImGuiMod_Ctrl", int64_t(ImGuiKey::ImGuiMod_Ctrl), das::LineInfo());
		addIEx("ImGuiMod_Shift", "ImGuiMod_Shift", int64_t(ImGuiKey::ImGuiMod_Shift), das::LineInfo());
		addIEx("ImGuiMod_Alt", "ImGuiMod_Alt", int64_t(ImGuiKey::ImGuiMod_Alt), das::LineInfo());
		addIEx("ImGuiMod_Super", "ImGuiMod_Super", int64_t(ImGuiKey::ImGuiMod_Super), das::LineInfo());
		addIEx("ImGuiMod_Mask_", "ImGuiMod_Mask_", int64_t(ImGuiKey::ImGuiMod_Mask_), das::LineInfo());
		addIEx("NamedKey_BEGIN", "ImGuiKey_NamedKey_BEGIN", int64_t(ImGuiKey::ImGuiKey_NamedKey_BEGIN), das::LineInfo());
		addIEx("NamedKey_END", "ImGuiKey_NamedKey_END", int64_t(ImGuiKey::ImGuiKey_NamedKey_END), das::LineInfo());
		addIEx("NamedKey_COUNT", "ImGuiKey_NamedKey_COUNT", int64_t(ImGuiKey::ImGuiKey_NamedKey_COUNT), das::LineInfo());
		addIEx("KeysData_SIZE", "ImGuiKey_KeysData_SIZE", int64_t(ImGuiKey::ImGuiKey_KeysData_SIZE), das::LineInfo());
		addIEx("KeysData_OFFSET", "ImGuiKey_KeysData_OFFSET", int64_t(ImGuiKey::ImGuiKey_KeysData_OFFSET), das::LineInfo());
	}
};

// from imgui/imgui.h:1609:6
class Enumeration_ImGuiInputFlags_ : public das::Enumeration {
public:
	Enumeration_ImGuiInputFlags_() : das::Enumeration("ImGuiInputFlags") {
		external = true;
		cppName = "ImGuiInputFlags_";
		baseType = (das::Type) das::ToBasicType<int>::type;
		addIEx("None", "ImGuiInputFlags_None", int64_t(ImGuiInputFlags_::ImGuiInputFlags_None), das::LineInfo());
		addIEx("Repeat", "ImGuiInputFlags_Repeat", int64_t(ImGuiInputFlags_::ImGuiInputFlags_Repeat), das::LineInfo());
		addIEx("RouteActive", "ImGuiInputFlags_RouteActive", int64_t(ImGuiInputFlags_::ImGuiInputFlags_RouteActive), das::LineInfo());
		addIEx("RouteFocused", "ImGuiInputFlags_RouteFocused", int64_t(ImGuiInputFlags_::ImGuiInputFlags_RouteFocused), das::LineInfo());
		addIEx("RouteGlobal", "ImGuiInputFlags_RouteGlobal", int64_t(ImGuiInputFlags_::ImGuiInputFlags_RouteGlobal), das::LineInfo());
		addIEx("RouteAlways", "ImGuiInputFlags_RouteAlways", int64_t(ImGuiInputFlags_::ImGuiInputFlags_RouteAlways), das::LineInfo());
		addIEx("RouteOverFocused", "ImGuiInputFlags_RouteOverFocused", int64_t(ImGuiInputFlags_::ImGuiInputFlags_RouteOverFocused), das::LineInfo());
		addIEx("RouteOverActive", "ImGuiInputFlags_RouteOverActive", int64_t(ImGuiInputFlags_::ImGuiInputFlags_RouteOverActive), das::LineInfo());
		addIEx("RouteUnlessBgFocused", "ImGuiInputFlags_RouteUnlessBgFocused", int64_t(ImGuiInputFlags_::ImGuiInputFlags_RouteUnlessBgFocused), das::LineInfo());
		addIEx("RouteFromRootWindow", "ImGuiInputFlags_RouteFromRootWindow", int64_t(ImGuiInputFlags_::ImGuiInputFlags_RouteFromRootWindow), das::LineInfo());
		addIEx("Tooltip", "ImGuiInputFlags_Tooltip", int64_t(ImGuiInputFlags_::ImGuiInputFlags_Tooltip), das::LineInfo());
	}
};

// from imgui/imgui.h:1644:6
class Enumeration_ImGuiConfigFlags_ : public das::Enumeration {
public:
	Enumeration_ImGuiConfigFlags_() : das::Enumeration("ImGuiConfigFlags") {
		external = true;
		cppName = "ImGuiConfigFlags_";
		baseType = (das::Type) das::ToBasicType<int>::type;
		addIEx("None", "ImGuiConfigFlags_None", int64_t(ImGuiConfigFlags_::ImGuiConfigFlags_None), das::LineInfo());
		addIEx("NavEnableKeyboard", "ImGuiConfigFlags_NavEnableKeyboard", int64_t(ImGuiConfigFlags_::ImGuiConfigFlags_NavEnableKeyboard), das::LineInfo());
		addIEx("NavEnableGamepad", "ImGuiConfigFlags_NavEnableGamepad", int64_t(ImGuiConfigFlags_::ImGuiConfigFlags_NavEnableGamepad), das::LineInfo());
		addIEx("NavEnableSetMousePos", "ImGuiConfigFlags_NavEnableSetMousePos", int64_t(ImGuiConfigFlags_::ImGuiConfigFlags_NavEnableSetMousePos), das::LineInfo());
		addIEx("NavNoCaptureKeyboard", "ImGuiConfigFlags_NavNoCaptureKeyboard", int64_t(ImGuiConfigFlags_::ImGuiConfigFlags_NavNoCaptureKeyboard), das::LineInfo());
		addIEx("NoMouse", "ImGuiConfigFlags_NoMouse", int64_t(ImGuiConfigFlags_::ImGuiConfigFlags_NoMouse), das::LineInfo());
		addIEx("NoMouseCursorChange", "ImGuiConfigFlags_NoMouseCursorChange", int64_t(ImGuiConfigFlags_::ImGuiConfigFlags_NoMouseCursorChange), das::LineInfo());
		addIEx("NoKeyboard", "ImGuiConfigFlags_NoKeyboard", int64_t(ImGuiConfigFlags_::ImGuiConfigFlags_NoKeyboard), das::LineInfo());
		addIEx("DockingEnable", "ImGuiConfigFlags_DockingEnable", int64_t(ImGuiConfigFlags_::ImGuiConfigFlags_DockingEnable), das::LineInfo());
		addIEx("ViewportsEnable", "ImGuiConfigFlags_ViewportsEnable", int64_t(ImGuiConfigFlags_::ImGuiConfigFlags_ViewportsEnable), das::LineInfo());
		addIEx("DpiEnableScaleViewports", "ImGuiConfigFlags_DpiEnableScaleViewports", int64_t(ImGuiConfigFlags_::ImGuiConfigFlags_DpiEnableScaleViewports), das::LineInfo());
		addIEx("DpiEnableScaleFonts", "ImGuiConfigFlags_DpiEnableScaleFonts", int64_t(ImGuiConfigFlags_::ImGuiConfigFlags_DpiEnableScaleFonts), das::LineInfo());
		addIEx("IsSRGB", "ImGuiConfigFlags_IsSRGB", int64_t(ImGuiConfigFlags_::ImGuiConfigFlags_IsSRGB), das::LineInfo());
		addIEx("IsTouchScreen", "ImGuiConfigFlags_IsTouchScreen", int64_t(ImGuiConfigFlags_::ImGuiConfigFlags_IsTouchScreen), das::LineInfo());
	}
};

// from imgui/imgui.h:1670:6
class Enumeration_ImGuiBackendFlags_ : public das::Enumeration {
public:
	Enumeration_ImGuiBackendFlags_() : das::Enumeration("ImGuiBackendFlags") {
		external = true;
		cppName = "ImGuiBackendFlags_";
		baseType = (das::Type) das::ToBasicType<int>::type;
		addIEx("None", "ImGuiBackendFlags_None", int64_t(ImGuiBackendFlags_::ImGuiBackendFlags_None), das::LineInfo());
		addIEx("HasGamepad", "ImGuiBackendFlags_HasGamepad", int64_t(ImGuiBackendFlags_::ImGuiBackendFlags_HasGamepad), das::LineInfo());
		addIEx("HasMouseCursors", "ImGuiBackendFlags_HasMouseCursors", int64_t(ImGuiBackendFlags_::ImGuiBackendFlags_HasMouseCursors), das::LineInfo());
		addIEx("HasSetMousePos", "ImGuiBackendFlags_HasSetMousePos", int64_t(ImGuiBackendFlags_::ImGuiBackendFlags_HasSetMousePos), das::LineInfo());
		addIEx("RendererHasVtxOffset", "ImGuiBackendFlags_RendererHasVtxOffset", int64_t(ImGuiBackendFlags_::ImGuiBackendFlags_RendererHasVtxOffset), das::LineInfo());
		addIEx("PlatformHasViewports", "ImGuiBackendFlags_PlatformHasViewports", int64_t(ImGuiBackendFlags_::ImGuiBackendFlags_PlatformHasViewports), das::LineInfo());
		addIEx("HasMouseHoveredViewport", "ImGuiBackendFlags_HasMouseHoveredViewport", int64_t(ImGuiBackendFlags_::ImGuiBackendFlags_HasMouseHoveredViewport), das::LineInfo());
		addIEx("RendererHasViewports", "ImGuiBackendFlags_RendererHasViewports", int64_t(ImGuiBackendFlags_::ImGuiBackendFlags_RendererHasViewports), das::LineInfo());
	}
};

// from imgui/imgui.h:1685:6
class Enumeration_ImGuiCol_ : public das::Enumeration {
public:
	Enumeration_ImGuiCol_() : das::Enumeration("ImGuiCol") {
		external = true;
		cppName = "ImGuiCol_";
		baseType = (das::Type) das::ToBasicType<int>::type;
		addIEx("Text", "ImGuiCol_Text", int64_t(ImGuiCol_::ImGuiCol_Text), das::LineInfo());
		addIEx("TextDisabled", "ImGuiCol_TextDisabled", int64_t(ImGuiCol_::ImGuiCol_TextDisabled), das::LineInfo());
		addIEx("WindowBg", "ImGuiCol_WindowBg", int64_t(ImGuiCol_::ImGuiCol_WindowBg), das::LineInfo());
		addIEx("ChildBg", "ImGuiCol_ChildBg", int64_t(ImGuiCol_::ImGuiCol_ChildBg), das::LineInfo());
		addIEx("PopupBg", "ImGuiCol_PopupBg", int64_t(ImGuiCol_::ImGuiCol_PopupBg), das::LineInfo());
		addIEx("Border", "ImGuiCol_Border", int64_t(ImGuiCol_::ImGuiCol_Border), das::LineInfo());
		addIEx("BorderShadow", "ImGuiCol_BorderShadow", int64_t(ImGuiCol_::ImGuiCol_BorderShadow), das::LineInfo());
		addIEx("FrameBg", "ImGuiCol_FrameBg", int64_t(ImGuiCol_::ImGuiCol_FrameBg), das::LineInfo());
		addIEx("FrameBgHovered", "ImGuiCol_FrameBgHovered", int64_t(ImGuiCol_::ImGuiCol_FrameBgHovered), das::LineInfo());
		addIEx("FrameBgActive", "ImGuiCol_FrameBgActive", int64_t(ImGuiCol_::ImGuiCol_FrameBgActive), das::LineInfo());
		addIEx("TitleBg", "ImGuiCol_TitleBg", int64_t(ImGuiCol_::ImGuiCol_TitleBg), das::LineInfo());
		addIEx("TitleBgActive", "ImGuiCol_TitleBgActive", int64_t(ImGuiCol_::ImGuiCol_TitleBgActive), das::LineInfo());
		addIEx("TitleBgCollapsed", "ImGuiCol_TitleBgCollapsed", int64_t(ImGuiCol_::ImGuiCol_TitleBgCollapsed), das::LineInfo());
		addIEx("MenuBarBg", "ImGuiCol_MenuBarBg", int64_t(ImGuiCol_::ImGuiCol_MenuBarBg), das::LineInfo());
		addIEx("ScrollbarBg", "ImGuiCol_ScrollbarBg", int64_t(ImGuiCol_::ImGuiCol_ScrollbarBg), das::LineInfo());
		addIEx("ScrollbarGrab", "ImGuiCol_ScrollbarGrab", int64_t(ImGuiCol_::ImGuiCol_ScrollbarGrab), das::LineInfo());
		addIEx("ScrollbarGrabHovered", "ImGuiCol_ScrollbarGrabHovered", int64_t(ImGuiCol_::ImGuiCol_ScrollbarGrabHovered), das::LineInfo());
		addIEx("ScrollbarGrabActive", "ImGuiCol_ScrollbarGrabActive", int64_t(ImGuiCol_::ImGuiCol_ScrollbarGrabActive), das::LineInfo());
		addIEx("CheckMark", "ImGuiCol_CheckMark", int64_t(ImGuiCol_::ImGuiCol_CheckMark), das::LineInfo());
		addIEx("SliderGrab", "ImGuiCol_SliderGrab", int64_t(ImGuiCol_::ImGuiCol_SliderGrab), das::LineInfo());
		addIEx("SliderGrabActive", "ImGuiCol_SliderGrabActive", int64_t(ImGuiCol_::ImGuiCol_SliderGrabActive), das::LineInfo());
		addIEx("Button", "ImGuiCol_Button", int64_t(ImGuiCol_::ImGuiCol_Button), das::LineInfo());
		addIEx("ButtonHovered", "ImGuiCol_ButtonHovered", int64_t(ImGuiCol_::ImGuiCol_ButtonHovered), das::LineInfo());
		addIEx("ButtonActive", "ImGuiCol_ButtonActive", int64_t(ImGuiCol_::ImGuiCol_ButtonActive), das::LineInfo());
		addIEx("Header", "ImGuiCol_Header", int64_t(ImGuiCol_::ImGuiCol_Header), das::LineInfo());
		addIEx("HeaderHovered", "ImGuiCol_HeaderHovered", int64_t(ImGuiCol_::ImGuiCol_HeaderHovered), das::LineInfo());
		addIEx("HeaderActive", "ImGuiCol_HeaderActive", int64_t(ImGuiCol_::ImGuiCol_HeaderActive), das::LineInfo());
		addIEx("Separator", "ImGuiCol_Separator", int64_t(ImGuiCol_::ImGuiCol_Separator), das::LineInfo());
		addIEx("SeparatorHovered", "ImGuiCol_SeparatorHovered", int64_t(ImGuiCol_::ImGuiCol_SeparatorHovered), das::LineInfo());
		addIEx("SeparatorActive", "ImGuiCol_SeparatorActive", int64_t(ImGuiCol_::ImGuiCol_SeparatorActive), das::LineInfo());
		addIEx("ResizeGrip", "ImGuiCol_ResizeGrip", int64_t(ImGuiCol_::ImGuiCol_ResizeGrip), das::LineInfo());
		addIEx("ResizeGripHovered", "ImGuiCol_ResizeGripHovered", int64_t(ImGuiCol_::ImGuiCol_ResizeGripHovered), das::LineInfo());
		addIEx("ResizeGripActive", "ImGuiCol_ResizeGripActive", int64_t(ImGuiCol_::ImGuiCol_ResizeGripActive), das::LineInfo());
		addIEx("TabHovered", "ImGuiCol_TabHovered", int64_t(ImGuiCol_::ImGuiCol_TabHovered), das::LineInfo());
		addIEx("Tab", "ImGuiCol_Tab", int64_t(ImGuiCol_::ImGuiCol_Tab), das::LineInfo());
		addIEx("TabSelected", "ImGuiCol_TabSelected", int64_t(ImGuiCol_::ImGuiCol_TabSelected), das::LineInfo());
		addIEx("TabSelectedOverline", "ImGuiCol_TabSelectedOverline", int64_t(ImGuiCol_::ImGuiCol_TabSelectedOverline), das::LineInfo());
		addIEx("TabDimmed", "ImGuiCol_TabDimmed", int64_t(ImGuiCol_::ImGuiCol_TabDimmed), das::LineInfo());
		addIEx("TabDimmedSelected", "ImGuiCol_TabDimmedSelected", int64_t(ImGuiCol_::ImGuiCol_TabDimmedSelected), das::LineInfo());
		addIEx("TabDimmedSelectedOverline", "ImGuiCol_TabDimmedSelectedOverline", int64_t(ImGuiCol_::ImGuiCol_TabDimmedSelectedOverline), das::LineInfo());
		addIEx("DockingPreview", "ImGuiCol_DockingPreview", int64_t(ImGuiCol_::ImGuiCol_DockingPreview), das::LineInfo());
		addIEx("DockingEmptyBg", "ImGuiCol_DockingEmptyBg", int64_t(ImGuiCol_::ImGuiCol_DockingEmptyBg), das::LineInfo());
		addIEx("PlotLines", "ImGuiCol_PlotLines", int64_t(ImGuiCol_::ImGuiCol_PlotLines), das::LineInfo());
		addIEx("PlotLinesHovered", "ImGuiCol_PlotLinesHovered", int64_t(ImGuiCol_::ImGuiCol_PlotLinesHovered), das::LineInfo());
		addIEx("PlotHistogram", "ImGuiCol_PlotHistogram", int64_t(ImGuiCol_::ImGuiCol_PlotHistogram), das::LineInfo());
		addIEx("PlotHistogramHovered", "ImGuiCol_PlotHistogramHovered", int64_t(ImGuiCol_::ImGuiCol_PlotHistogramHovered), das::LineInfo());
		addIEx("TableHeaderBg", "ImGuiCol_TableHeaderBg", int64_t(ImGuiCol_::ImGuiCol_TableHeaderBg), das::LineInfo());
		addIEx("TableBorderStrong", "ImGuiCol_TableBorderStrong", int64_t(ImGuiCol_::ImGuiCol_TableBorderStrong), das::LineInfo());
		addIEx("TableBorderLight", "ImGuiCol_TableBorderLight", int64_t(ImGuiCol_::ImGuiCol_TableBorderLight), das::LineInfo());
		addIEx("TableRowBg", "ImGuiCol_TableRowBg", int64_t(ImGuiCol_::ImGuiCol_TableRowBg), das::LineInfo());
		addIEx("TableRowBgAlt", "ImGuiCol_TableRowBgAlt", int64_t(ImGuiCol_::ImGuiCol_TableRowBgAlt), das::LineInfo());
		addIEx("TextLink", "ImGuiCol_TextLink", int64_t(ImGuiCol_::ImGuiCol_TextLink), das::LineInfo());
		addIEx("TextSelectedBg", "ImGuiCol_TextSelectedBg", int64_t(ImGuiCol_::ImGuiCol_TextSelectedBg), das::LineInfo());
		addIEx("DragDropTarget", "ImGuiCol_DragDropTarget", int64_t(ImGuiCol_::ImGuiCol_DragDropTarget), das::LineInfo());
		addIEx("NavHighlight", "ImGuiCol_NavHighlight", int64_t(ImGuiCol_::ImGuiCol_NavHighlight), das::LineInfo());
		addIEx("NavWindowingHighlight", "ImGuiCol_NavWindowingHighlight", int64_t(ImGuiCol_::ImGuiCol_NavWindowingHighlight), das::LineInfo());
		addIEx("NavWindowingDimBg", "ImGuiCol_NavWindowingDimBg", int64_t(ImGuiCol_::ImGuiCol_NavWindowingDimBg), das::LineInfo());
		addIEx("ModalWindowDimBg", "ImGuiCol_ModalWindowDimBg", int64_t(ImGuiCol_::ImGuiCol_ModalWindowDimBg), das::LineInfo());
		addIEx("COUNT", "ImGuiCol_COUNT", int64_t(ImGuiCol_::ImGuiCol_COUNT), das::LineInfo());
	}
};

// from imgui/imgui.h:1762:6
class Enumeration_ImGuiStyleVar_ : public das::Enumeration {
public:
	Enumeration_ImGuiStyleVar_() : das::Enumeration("ImGuiStyleVar") {
		external = true;
		cppName = "ImGuiStyleVar_";
		baseType = (das::Type) das::ToBasicType<int>::type;
		addIEx("Alpha", "ImGuiStyleVar_Alpha", int64_t(ImGuiStyleVar_::ImGuiStyleVar_Alpha), das::LineInfo());
		addIEx("DisabledAlpha", "ImGuiStyleVar_DisabledAlpha", int64_t(ImGuiStyleVar_::ImGuiStyleVar_DisabledAlpha), das::LineInfo());
		addIEx("WindowPadding", "ImGuiStyleVar_WindowPadding", int64_t(ImGuiStyleVar_::ImGuiStyleVar_WindowPadding), das::LineInfo());
		addIEx("WindowRounding", "ImGuiStyleVar_WindowRounding", int64_t(ImGuiStyleVar_::ImGuiStyleVar_WindowRounding), das::LineInfo());
		addIEx("WindowBorderSize", "ImGuiStyleVar_WindowBorderSize", int64_t(ImGuiStyleVar_::ImGuiStyleVar_WindowBorderSize), das::LineInfo());
		addIEx("WindowMinSize", "ImGuiStyleVar_WindowMinSize", int64_t(ImGuiStyleVar_::ImGuiStyleVar_WindowMinSize), das::LineInfo());
		addIEx("WindowTitleAlign", "ImGuiStyleVar_WindowTitleAlign", int64_t(ImGuiStyleVar_::ImGuiStyleVar_WindowTitleAlign), das::LineInfo());
		addIEx("ChildRounding", "ImGuiStyleVar_ChildRounding", int64_t(ImGuiStyleVar_::ImGuiStyleVar_ChildRounding), das::LineInfo());
		addIEx("ChildBorderSize", "ImGuiStyleVar_ChildBorderSize", int64_t(ImGuiStyleVar_::ImGuiStyleVar_ChildBorderSize), das::LineInfo());
		addIEx("PopupRounding", "ImGuiStyleVar_PopupRounding", int64_t(ImGuiStyleVar_::ImGuiStyleVar_PopupRounding), das::LineInfo());
		addIEx("PopupBorderSize", "ImGuiStyleVar_PopupBorderSize", int64_t(ImGuiStyleVar_::ImGuiStyleVar_PopupBorderSize), das::LineInfo());
		addIEx("FramePadding", "ImGuiStyleVar_FramePadding", int64_t(ImGuiStyleVar_::ImGuiStyleVar_FramePadding), das::LineInfo());
		addIEx("FrameRounding", "ImGuiStyleVar_FrameRounding", int64_t(ImGuiStyleVar_::ImGuiStyleVar_FrameRounding), das::LineInfo());
		addIEx("FrameBorderSize", "ImGuiStyleVar_FrameBorderSize", int64_t(ImGuiStyleVar_::ImGuiStyleVar_FrameBorderSize), das::LineInfo());
		addIEx("ItemSpacing", "ImGuiStyleVar_ItemSpacing", int64_t(ImGuiStyleVar_::ImGuiStyleVar_ItemSpacing), das::LineInfo());
		addIEx("ItemInnerSpacing", "ImGuiStyleVar_ItemInnerSpacing", int64_t(ImGuiStyleVar_::ImGuiStyleVar_ItemInnerSpacing), das::LineInfo());
		addIEx("IndentSpacing", "ImGuiStyleVar_IndentSpacing", int64_t(ImGuiStyleVar_::ImGuiStyleVar_IndentSpacing), das::LineInfo());
		addIEx("CellPadding", "ImGuiStyleVar_CellPadding", int64_t(ImGuiStyleVar_::ImGuiStyleVar_CellPadding), das::LineInfo());
		addIEx("ScrollbarSize", "ImGuiStyleVar_ScrollbarSize", int64_t(ImGuiStyleVar_::ImGuiStyleVar_ScrollbarSize), das::LineInfo());
		addIEx("ScrollbarRounding", "ImGuiStyleVar_ScrollbarRounding", int64_t(ImGuiStyleVar_::ImGuiStyleVar_ScrollbarRounding), das::LineInfo());
		addIEx("GrabMinSize", "ImGuiStyleVar_GrabMinSize", int64_t(ImGuiStyleVar_::ImGuiStyleVar_GrabMinSize), das::LineInfo());
		addIEx("GrabRounding", "ImGuiStyleVar_GrabRounding", int64_t(ImGuiStyleVar_::ImGuiStyleVar_GrabRounding), das::LineInfo());
		addIEx("TabRounding", "ImGuiStyleVar_TabRounding", int64_t(ImGuiStyleVar_::ImGuiStyleVar_TabRounding), das::LineInfo());
		addIEx("TabBorderSize", "ImGuiStyleVar_TabBorderSize", int64_t(ImGuiStyleVar_::ImGuiStyleVar_TabBorderSize), das::LineInfo());
		addIEx("TabBarBorderSize", "ImGuiStyleVar_TabBarBorderSize", int64_t(ImGuiStyleVar_::ImGuiStyleVar_TabBarBorderSize), das::LineInfo());
		addIEx("TabBarOverlineSize", "ImGuiStyleVar_TabBarOverlineSize", int64_t(ImGuiStyleVar_::ImGuiStyleVar_TabBarOverlineSize), das::LineInfo());
		addIEx("TableAngledHeadersAngle", "ImGuiStyleVar_TableAngledHeadersAngle", int64_t(ImGuiStyleVar_::ImGuiStyleVar_TableAngledHeadersAngle), das::LineInfo());
		addIEx("TableAngledHeadersTextAlign", "ImGuiStyleVar_TableAngledHeadersTextAlign", int64_t(ImGuiStyleVar_::ImGuiStyleVar_TableAngledHeadersTextAlign), das::LineInfo());
		addIEx("ButtonTextAlign", "ImGuiStyleVar_ButtonTextAlign", int64_t(ImGuiStyleVar_::ImGuiStyleVar_ButtonTextAlign), das::LineInfo());
		addIEx("SelectableTextAlign", "ImGuiStyleVar_SelectableTextAlign", int64_t(ImGuiStyleVar_::ImGuiStyleVar_SelectableTextAlign), das::LineInfo());
		addIEx("SeparatorTextBorderSize", "ImGuiStyleVar_SeparatorTextBorderSize", int64_t(ImGuiStyleVar_::ImGuiStyleVar_SeparatorTextBorderSize), das::LineInfo());
		addIEx("SeparatorTextAlign", "ImGuiStyleVar_SeparatorTextAlign", int64_t(ImGuiStyleVar_::ImGuiStyleVar_SeparatorTextAlign), das::LineInfo());
		addIEx("SeparatorTextPadding", "ImGuiStyleVar_SeparatorTextPadding", int64_t(ImGuiStyleVar_::ImGuiStyleVar_SeparatorTextPadding), das::LineInfo());
		addIEx("DockingSeparatorSize", "ImGuiStyleVar_DockingSeparatorSize", int64_t(ImGuiStyleVar_::ImGuiStyleVar_DockingSeparatorSize), das::LineInfo());
		addIEx("COUNT", "ImGuiStyleVar_COUNT", int64_t(ImGuiStyleVar_::ImGuiStyleVar_COUNT), das::LineInfo());
	}
};

// from imgui/imgui.h:1803:6
class Enumeration_ImGuiButtonFlags_ : public das::Enumeration {
public:
	Enumeration_ImGuiButtonFlags_() : das::Enumeration("ImGuiButtonFlags") {
		external = true;
		cppName = "ImGuiButtonFlags_";
		baseType = (das::Type) das::ToBasicType<int>::type;
		addIEx("None", "ImGuiButtonFlags_None", int64_t(ImGuiButtonFlags_::ImGuiButtonFlags_None), das::LineInfo());
		addIEx("MouseButtonLeft", "ImGuiButtonFlags_MouseButtonLeft", int64_t(ImGuiButtonFlags_::ImGuiButtonFlags_MouseButtonLeft), das::LineInfo());
		addIEx("MouseButtonRight", "ImGuiButtonFlags_MouseButtonRight", int64_t(ImGuiButtonFlags_::ImGuiButtonFlags_MouseButtonRight), das::LineInfo());
		addIEx("MouseButtonMiddle", "ImGuiButtonFlags_MouseButtonMiddle", int64_t(ImGuiButtonFlags_::ImGuiButtonFlags_MouseButtonMiddle), das::LineInfo());
		addIEx("MouseButtonMask_", "ImGuiButtonFlags_MouseButtonMask_", int64_t(ImGuiButtonFlags_::ImGuiButtonFlags_MouseButtonMask_), das::LineInfo());
	}
};

// from imgui/imgui.h:1814:6
class Enumeration_ImGuiColorEditFlags_ : public das::Enumeration {
public:
	Enumeration_ImGuiColorEditFlags_() : das::Enumeration("ImGuiColorEditFlags") {
		external = true;
		cppName = "ImGuiColorEditFlags_";
		baseType = (das::Type) das::ToBasicType<int>::type;
		addIEx("None", "ImGuiColorEditFlags_None", int64_t(ImGuiColorEditFlags_::ImGuiColorEditFlags_None), das::LineInfo());
		addIEx("NoAlpha", "ImGuiColorEditFlags_NoAlpha", int64_t(ImGuiColorEditFlags_::ImGuiColorEditFlags_NoAlpha), das::LineInfo());
		addIEx("NoPicker", "ImGuiColorEditFlags_NoPicker", int64_t(ImGuiColorEditFlags_::ImGuiColorEditFlags_NoPicker), das::LineInfo());
		addIEx("NoOptions", "ImGuiColorEditFlags_NoOptions", int64_t(ImGuiColorEditFlags_::ImGuiColorEditFlags_NoOptions), das::LineInfo());
		addIEx("NoSmallPreview", "ImGuiColorEditFlags_NoSmallPreview", int64_t(ImGuiColorEditFlags_::ImGuiColorEditFlags_NoSmallPreview), das::LineInfo());
		addIEx("NoInputs", "ImGuiColorEditFlags_NoInputs", int64_t(ImGuiColorEditFlags_::ImGuiColorEditFlags_NoInputs), das::LineInfo());
		addIEx("NoTooltip", "ImGuiColorEditFlags_NoTooltip", int64_t(ImGuiColorEditFlags_::ImGuiColorEditFlags_NoTooltip), das::LineInfo());
		addIEx("NoLabel", "ImGuiColorEditFlags_NoLabel", int64_t(ImGuiColorEditFlags_::ImGuiColorEditFlags_NoLabel), das::LineInfo());
		addIEx("NoSidePreview", "ImGuiColorEditFlags_NoSidePreview", int64_t(ImGuiColorEditFlags_::ImGuiColorEditFlags_NoSidePreview), das::LineInfo());
		addIEx("NoDragDrop", "ImGuiColorEditFlags_NoDragDrop", int64_t(ImGuiColorEditFlags_::ImGuiColorEditFlags_NoDragDrop), das::LineInfo());
		addIEx("NoBorder", "ImGuiColorEditFlags_NoBorder", int64_t(ImGuiColorEditFlags_::ImGuiColorEditFlags_NoBorder), das::LineInfo());
		addIEx("AlphaBar", "ImGuiColorEditFlags_AlphaBar", int64_t(ImGuiColorEditFlags_::ImGuiColorEditFlags_AlphaBar), das::LineInfo());
		addIEx("AlphaPreview", "ImGuiColorEditFlags_AlphaPreview", int64_t(ImGuiColorEditFlags_::ImGuiColorEditFlags_AlphaPreview), das::LineInfo());
		addIEx("AlphaPreviewHalf", "ImGuiColorEditFlags_AlphaPreviewHalf", int64_t(ImGuiColorEditFlags_::ImGuiColorEditFlags_AlphaPreviewHalf), das::LineInfo());
		addIEx("HDR", "ImGuiColorEditFlags_HDR", int64_t(ImGuiColorEditFlags_::ImGuiColorEditFlags_HDR), das::LineInfo());
		addIEx("DisplayRGB", "ImGuiColorEditFlags_DisplayRGB", int64_t(ImGuiColorEditFlags_::ImGuiColorEditFlags_DisplayRGB), das::LineInfo());
		addIEx("DisplayHSV", "ImGuiColorEditFlags_DisplayHSV", int64_t(ImGuiColorEditFlags_::ImGuiColorEditFlags_DisplayHSV), das::LineInfo());
		addIEx("DisplayHex", "ImGuiColorEditFlags_DisplayHex", int64_t(ImGuiColorEditFlags_::ImGuiColorEditFlags_DisplayHex), das::LineInfo());
		addIEx("Uint8", "ImGuiColorEditFlags_Uint8", int64_t(ImGuiColorEditFlags_::ImGuiColorEditFlags_Uint8), das::LineInfo());
		addIEx("Float", "ImGuiColorEditFlags_Float", int64_t(ImGuiColorEditFlags_::ImGuiColorEditFlags_Float), das::LineInfo());
		addIEx("PickerHueBar", "ImGuiColorEditFlags_PickerHueBar", int64_t(ImGuiColorEditFlags_::ImGuiColorEditFlags_PickerHueBar), das::LineInfo());
		addIEx("PickerHueWheel", "ImGuiColorEditFlags_PickerHueWheel", int64_t(ImGuiColorEditFlags_::ImGuiColorEditFlags_PickerHueWheel), das::LineInfo());
		addIEx("InputRGB", "ImGuiColorEditFlags_InputRGB", int64_t(ImGuiColorEditFlags_::ImGuiColorEditFlags_InputRGB), das::LineInfo());
		addIEx("InputHSV", "ImGuiColorEditFlags_InputHSV", int64_t(ImGuiColorEditFlags_::ImGuiColorEditFlags_InputHSV), das::LineInfo());
		addIEx("DefaultOptions_", "ImGuiColorEditFlags_DefaultOptions_", int64_t(ImGuiColorEditFlags_::ImGuiColorEditFlags_DefaultOptions_), das::LineInfo());
		addIEx("DisplayMask_", "ImGuiColorEditFlags_DisplayMask_", int64_t(ImGuiColorEditFlags_::ImGuiColorEditFlags_DisplayMask_), das::LineInfo());
		addIEx("DataTypeMask_", "ImGuiColorEditFlags_DataTypeMask_", int64_t(ImGuiColorEditFlags_::ImGuiColorEditFlags_DataTypeMask_), das::LineInfo());
		addIEx("PickerMask_", "ImGuiColorEditFlags_PickerMask_", int64_t(ImGuiColorEditFlags_::ImGuiColorEditFlags_PickerMask_), das::LineInfo());
		addIEx("InputMask_", "ImGuiColorEditFlags_InputMask_", int64_t(ImGuiColorEditFlags_::ImGuiColorEditFlags_InputMask_), das::LineInfo());
	}
};

// from imgui/imgui.h:1860:6
class Enumeration_ImGuiSliderFlags_ : public das::Enumeration {
public:
	Enumeration_ImGuiSliderFlags_() : das::Enumeration("ImGuiSliderFlags") {
		external = true;
		cppName = "ImGuiSliderFlags_";
		baseType = (das::Type) das::ToBasicType<int>::type;
		addIEx("None", "ImGuiSliderFlags_None", int64_t(ImGuiSliderFlags_::ImGuiSliderFlags_None), das::LineInfo());
		addIEx("AlwaysClamp", "ImGuiSliderFlags_AlwaysClamp", int64_t(ImGuiSliderFlags_::ImGuiSliderFlags_AlwaysClamp), das::LineInfo());
		addIEx("Logarithmic", "ImGuiSliderFlags_Logarithmic", int64_t(ImGuiSliderFlags_::ImGuiSliderFlags_Logarithmic), das::LineInfo());
		addIEx("NoRoundToFormat", "ImGuiSliderFlags_NoRoundToFormat", int64_t(ImGuiSliderFlags_::ImGuiSliderFlags_NoRoundToFormat), das::LineInfo());
		addIEx("NoInput", "ImGuiSliderFlags_NoInput", int64_t(ImGuiSliderFlags_::ImGuiSliderFlags_NoInput), das::LineInfo());
		addIEx("WrapAround", "ImGuiSliderFlags_WrapAround", int64_t(ImGuiSliderFlags_::ImGuiSliderFlags_WrapAround), das::LineInfo());
		addIEx("InvalidMask_", "ImGuiSliderFlags_InvalidMask_", int64_t(ImGuiSliderFlags_::ImGuiSliderFlags_InvalidMask_), das::LineInfo());
	}
};

// from imgui/imgui.h:1876:6
class Enumeration_ImGuiMouseButton_ : public das::Enumeration {
public:
	Enumeration_ImGuiMouseButton_() : das::Enumeration("ImGuiMouseButton") {
		external = true;
		cppName = "ImGuiMouseButton_";
		baseType = (das::Type) das::ToBasicType<int>::type;
		addIEx("Left", "ImGuiMouseButton_Left", int64_t(ImGuiMouseButton_::ImGuiMouseButton_Left), das::LineInfo());
		addIEx("Right", "ImGuiMouseButton_Right", int64_t(ImGuiMouseButton_::ImGuiMouseButton_Right), das::LineInfo());
		addIEx("Middle", "ImGuiMouseButton_Middle", int64_t(ImGuiMouseButton_::ImGuiMouseButton_Middle), das::LineInfo());
		addIEx("COUNT", "ImGuiMouseButton_COUNT", int64_t(ImGuiMouseButton_::ImGuiMouseButton_COUNT), das::LineInfo());
	}
};

// from imgui/imgui.h:1886:6
class Enumeration_ImGuiMouseCursor_ : public das::Enumeration {
public:
	Enumeration_ImGuiMouseCursor_() : das::Enumeration("ImGuiMouseCursor") {
		external = true;
		cppName = "ImGuiMouseCursor_";
		baseType = (das::Type) das::ToBasicType<int>::type;
		addIEx("None", "ImGuiMouseCursor_None", int64_t(ImGuiMouseCursor_::ImGuiMouseCursor_None), das::LineInfo());
		addIEx("Arrow", "ImGuiMouseCursor_Arrow", int64_t(ImGuiMouseCursor_::ImGuiMouseCursor_Arrow), das::LineInfo());
		addIEx("TextInput", "ImGuiMouseCursor_TextInput", int64_t(ImGuiMouseCursor_::ImGuiMouseCursor_TextInput), das::LineInfo());
		addIEx("ResizeAll", "ImGuiMouseCursor_ResizeAll", int64_t(ImGuiMouseCursor_::ImGuiMouseCursor_ResizeAll), das::LineInfo());
		addIEx("ResizeNS", "ImGuiMouseCursor_ResizeNS", int64_t(ImGuiMouseCursor_::ImGuiMouseCursor_ResizeNS), das::LineInfo());
		addIEx("ResizeEW", "ImGuiMouseCursor_ResizeEW", int64_t(ImGuiMouseCursor_::ImGuiMouseCursor_ResizeEW), das::LineInfo());
		addIEx("ResizeNESW", "ImGuiMouseCursor_ResizeNESW", int64_t(ImGuiMouseCursor_::ImGuiMouseCursor_ResizeNESW), das::LineInfo());
		addIEx("ResizeNWSE", "ImGuiMouseCursor_ResizeNWSE", int64_t(ImGuiMouseCursor_::ImGuiMouseCursor_ResizeNWSE), das::LineInfo());
		addIEx("Hand", "ImGuiMouseCursor_Hand", int64_t(ImGuiMouseCursor_::ImGuiMouseCursor_Hand), das::LineInfo());
		addIEx("NotAllowed", "ImGuiMouseCursor_NotAllowed", int64_t(ImGuiMouseCursor_::ImGuiMouseCursor_NotAllowed), das::LineInfo());
		addIEx("COUNT", "ImGuiMouseCursor_COUNT", int64_t(ImGuiMouseCursor_::ImGuiMouseCursor_COUNT), das::LineInfo());
	}
};

// from imgui/imgui.h:1905:6
class Enumeration_ImGuiMouseSource : public das::Enumeration {
public:
	Enumeration_ImGuiMouseSource() : das::Enumeration("ImGuiMouseSource") {
		external = true;
		cppName = "ImGuiMouseSource";
		baseType = (das::Type) das::ToBasicType<int>::type;
		addIEx("Mouse", "ImGuiMouseSource_Mouse", int64_t(ImGuiMouseSource::ImGuiMouseSource_Mouse), das::LineInfo());
		addIEx("TouchScreen", "ImGuiMouseSource_TouchScreen", int64_t(ImGuiMouseSource::ImGuiMouseSource_TouchScreen), das::LineInfo());
		addIEx("Pen", "ImGuiMouseSource_Pen", int64_t(ImGuiMouseSource::ImGuiMouseSource_Pen), das::LineInfo());
		addIEx("COUNT", "ImGuiMouseSource_COUNT", int64_t(ImGuiMouseSource::ImGuiMouseSource_COUNT), das::LineInfo());
	}
};

// from imgui/imgui.h:1916:6
class Enumeration_ImGuiCond_ : public das::Enumeration {
public:
	Enumeration_ImGuiCond_() : das::Enumeration("ImGuiCond") {
		external = true;
		cppName = "ImGuiCond_";
		baseType = (das::Type) das::ToBasicType<int>::type;
		addIEx("None", "ImGuiCond_None", int64_t(ImGuiCond_::ImGuiCond_None), das::LineInfo());
		addIEx("Always", "ImGuiCond_Always", int64_t(ImGuiCond_::ImGuiCond_Always), das::LineInfo());
		addIEx("Once", "ImGuiCond_Once", int64_t(ImGuiCond_::ImGuiCond_Once), das::LineInfo());
		addIEx("FirstUseEver", "ImGuiCond_FirstUseEver", int64_t(ImGuiCond_::ImGuiCond_FirstUseEver), das::LineInfo());
		addIEx("Appearing", "ImGuiCond_Appearing", int64_t(ImGuiCond_::ImGuiCond_Appearing), das::LineInfo());
	}
};

// from imgui/imgui.h:1951:6
class Enumeration_ImGuiTableFlags_ : public das::Enumeration {
public:
	Enumeration_ImGuiTableFlags_() : das::Enumeration("ImGuiTableFlags") {
		external = true;
		cppName = "ImGuiTableFlags_";
		baseType = (das::Type) das::ToBasicType<int>::type;
		addIEx("None", "ImGuiTableFlags_None", int64_t(ImGuiTableFlags_::ImGuiTableFlags_None), das::LineInfo());
		addIEx("Resizable", "ImGuiTableFlags_Resizable", int64_t(ImGuiTableFlags_::ImGuiTableFlags_Resizable), das::LineInfo());
		addIEx("Reorderable", "ImGuiTableFlags_Reorderable", int64_t(ImGuiTableFlags_::ImGuiTableFlags_Reorderable), das::LineInfo());
		addIEx("Hideable", "ImGuiTableFlags_Hideable", int64_t(ImGuiTableFlags_::ImGuiTableFlags_Hideable), das::LineInfo());
		addIEx("Sortable", "ImGuiTableFlags_Sortable", int64_t(ImGuiTableFlags_::ImGuiTableFlags_Sortable), das::LineInfo());
		addIEx("NoSavedSettings", "ImGuiTableFlags_NoSavedSettings", int64_t(ImGuiTableFlags_::ImGuiTableFlags_NoSavedSettings), das::LineInfo());
		addIEx("ContextMenuInBody", "ImGuiTableFlags_ContextMenuInBody", int64_t(ImGuiTableFlags_::ImGuiTableFlags_ContextMenuInBody), das::LineInfo());
		addIEx("RowBg", "ImGuiTableFlags_RowBg", int64_t(ImGuiTableFlags_::ImGuiTableFlags_RowBg), das::LineInfo());
		addIEx("BordersInnerH", "ImGuiTableFlags_BordersInnerH", int64_t(ImGuiTableFlags_::ImGuiTableFlags_BordersInnerH), das::LineInfo());
		addIEx("BordersOuterH", "ImGuiTableFlags_BordersOuterH", int64_t(ImGuiTableFlags_::ImGuiTableFlags_BordersOuterH), das::LineInfo());
		addIEx("BordersInnerV", "ImGuiTableFlags_BordersInnerV", int64_t(ImGuiTableFlags_::ImGuiTableFlags_BordersInnerV), das::LineInfo());
		addIEx("BordersOuterV", "ImGuiTableFlags_BordersOuterV", int64_t(ImGuiTableFlags_::ImGuiTableFlags_BordersOuterV), das::LineInfo());
		addIEx("BordersH", "ImGuiTableFlags_BordersH", int64_t(ImGuiTableFlags_::ImGuiTableFlags_BordersH), das::LineInfo());
		addIEx("BordersV", "ImGuiTableFlags_BordersV", int64_t(ImGuiTableFlags_::ImGuiTableFlags_BordersV), das::LineInfo());
		addIEx("BordersInner", "ImGuiTableFlags_BordersInner", int64_t(ImGuiTableFlags_::ImGuiTableFlags_BordersInner), das::LineInfo());
		addIEx("BordersOuter", "ImGuiTableFlags_BordersOuter", int64_t(ImGuiTableFlags_::ImGuiTableFlags_BordersOuter), das::LineInfo());
		addIEx("Borders", "ImGuiTableFlags_Borders", int64_t(ImGuiTableFlags_::ImGuiTableFlags_Borders), das::LineInfo());
		addIEx("NoBordersInBody", "ImGuiTableFlags_NoBordersInBody", int64_t(ImGuiTableFlags_::ImGuiTableFlags_NoBordersInBody), das::LineInfo());
		addIEx("NoBordersInBodyUntilResize", "ImGuiTableFlags_NoBordersInBodyUntilResize", int64_t(ImGuiTableFlags_::ImGuiTableFlags_NoBordersInBodyUntilResize), das::LineInfo());
		addIEx("SizingFixedFit", "ImGuiTableFlags_SizingFixedFit", int64_t(ImGuiTableFlags_::ImGuiTableFlags_SizingFixedFit), das::LineInfo());
		addIEx("SizingFixedSame", "ImGuiTableFlags_SizingFixedSame", int64_t(ImGuiTableFlags_::ImGuiTableFlags_SizingFixedSame), das::LineInfo());
		addIEx("SizingStretchProp", "ImGuiTableFlags_SizingStretchProp", int64_t(ImGuiTableFlags_::ImGuiTableFlags_SizingStretchProp), das::LineInfo());
		addIEx("SizingStretchSame", "ImGuiTableFlags_SizingStretchSame", int64_t(ImGuiTableFlags_::ImGuiTableFlags_SizingStretchSame), das::LineInfo());
		addIEx("NoHostExtendX", "ImGuiTableFlags_NoHostExtendX", int64_t(ImGuiTableFlags_::ImGuiTableFlags_NoHostExtendX), das::LineInfo());
		addIEx("NoHostExtendY", "ImGuiTableFlags_NoHostExtendY", int64_t(ImGuiTableFlags_::ImGuiTableFlags_NoHostExtendY), das::LineInfo());
		addIEx("NoKeepColumnsVisible", "ImGuiTableFlags_NoKeepColumnsVisible", int64_t(ImGuiTableFlags_::ImGuiTableFlags_NoKeepColumnsVisible), das::LineInfo());
		addIEx("PreciseWidths", "ImGuiTableFlags_PreciseWidths", int64_t(ImGuiTableFlags_::ImGuiTableFlags_PreciseWidths), das::LineInfo());
		addIEx("NoClip", "ImGuiTableFlags_NoClip", int64_t(ImGuiTableFlags_::ImGuiTableFlags_NoClip), das::LineInfo());
		addIEx("PadOuterX", "ImGuiTableFlags_PadOuterX", int64_t(ImGuiTableFlags_::ImGuiTableFlags_PadOuterX), das::LineInfo());
		addIEx("NoPadOuterX", "ImGuiTableFlags_NoPadOuterX", int64_t(ImGuiTableFlags_::ImGuiTableFlags_NoPadOuterX), das::LineInfo());
		addIEx("NoPadInnerX", "ImGuiTableFlags_NoPadInnerX", int64_t(ImGuiTableFlags_::ImGuiTableFlags_NoPadInnerX), das::LineInfo());
		addIEx("ScrollX", "ImGuiTableFlags_ScrollX", int64_t(ImGuiTableFlags_::ImGuiTableFlags_ScrollX), das::LineInfo());
		addIEx("ScrollY", "ImGuiTableFlags_ScrollY", int64_t(ImGuiTableFlags_::ImGuiTableFlags_ScrollY), das::LineInfo());
		addIEx("SortMulti", "ImGuiTableFlags_SortMulti", int64_t(ImGuiTableFlags_::ImGuiTableFlags_SortMulti), das::LineInfo());
		addIEx("SortTristate", "ImGuiTableFlags_SortTristate", int64_t(ImGuiTableFlags_::ImGuiTableFlags_SortTristate), das::LineInfo());
		addIEx("HighlightHoveredColumn", "ImGuiTableFlags_HighlightHoveredColumn", int64_t(ImGuiTableFlags_::ImGuiTableFlags_HighlightHoveredColumn), das::LineInfo());
		addIEx("SizingMask_", "ImGuiTableFlags_SizingMask_", int64_t(ImGuiTableFlags_::ImGuiTableFlags_SizingMask_), das::LineInfo());
	}
};

// from imgui/imgui.h:2004:6
class Enumeration_ImGuiTableColumnFlags_ : public das::Enumeration {
public:
	Enumeration_ImGuiTableColumnFlags_() : das::Enumeration("ImGuiTableColumnFlags") {
		external = true;
		cppName = "ImGuiTableColumnFlags_";
		baseType = (das::Type) das::ToBasicType<int>::type;
		addIEx("None", "ImGuiTableColumnFlags_None", int64_t(ImGuiTableColumnFlags_::ImGuiTableColumnFlags_None), das::LineInfo());
		addIEx("Disabled", "ImGuiTableColumnFlags_Disabled", int64_t(ImGuiTableColumnFlags_::ImGuiTableColumnFlags_Disabled), das::LineInfo());
		addIEx("DefaultHide", "ImGuiTableColumnFlags_DefaultHide", int64_t(ImGuiTableColumnFlags_::ImGuiTableColumnFlags_DefaultHide), das::LineInfo());
		addIEx("DefaultSort", "ImGuiTableColumnFlags_DefaultSort", int64_t(ImGuiTableColumnFlags_::ImGuiTableColumnFlags_DefaultSort), das::LineInfo());
		addIEx("WidthStretch", "ImGuiTableColumnFlags_WidthStretch", int64_t(ImGuiTableColumnFlags_::ImGuiTableColumnFlags_WidthStretch), das::LineInfo());
		addIEx("WidthFixed", "ImGuiTableColumnFlags_WidthFixed", int64_t(ImGuiTableColumnFlags_::ImGuiTableColumnFlags_WidthFixed), das::LineInfo());
		addIEx("NoResize", "ImGuiTableColumnFlags_NoResize", int64_t(ImGuiTableColumnFlags_::ImGuiTableColumnFlags_NoResize), das::LineInfo());
		addIEx("NoReorder", "ImGuiTableColumnFlags_NoReorder", int64_t(ImGuiTableColumnFlags_::ImGuiTableColumnFlags_NoReorder), das::LineInfo());
		addIEx("NoHide", "ImGuiTableColumnFlags_NoHide", int64_t(ImGuiTableColumnFlags_::ImGuiTableColumnFlags_NoHide), das::LineInfo());
		addIEx("NoClip", "ImGuiTableColumnFlags_NoClip", int64_t(ImGuiTableColumnFlags_::ImGuiTableColumnFlags_NoClip), das::LineInfo());
		addIEx("NoSort", "ImGuiTableColumnFlags_NoSort", int64_t(ImGuiTableColumnFlags_::ImGuiTableColumnFlags_NoSort), das::LineInfo());
		addIEx("NoSortAscending", "ImGuiTableColumnFlags_NoSortAscending", int64_t(ImGuiTableColumnFlags_::ImGuiTableColumnFlags_NoSortAscending), das::LineInfo());
		addIEx("NoSortDescending", "ImGuiTableColumnFlags_NoSortDescending", int64_t(ImGuiTableColumnFlags_::ImGuiTableColumnFlags_NoSortDescending), das::LineInfo());
		addIEx("NoHeaderLabel", "ImGuiTableColumnFlags_NoHeaderLabel", int64_t(ImGuiTableColumnFlags_::ImGuiTableColumnFlags_NoHeaderLabel), das::LineInfo());
		addIEx("NoHeaderWidth", "ImGuiTableColumnFlags_NoHeaderWidth", int64_t(ImGuiTableColumnFlags_::ImGuiTableColumnFlags_NoHeaderWidth), das::LineInfo());
		addIEx("PreferSortAscending", "ImGuiTableColumnFlags_PreferSortAscending", int64_t(ImGuiTableColumnFlags_::ImGuiTableColumnFlags_PreferSortAscending), das::LineInfo());
		addIEx("PreferSortDescending", "ImGuiTableColumnFlags_PreferSortDescending", int64_t(ImGuiTableColumnFlags_::ImGuiTableColumnFlags_PreferSortDescending), das::LineInfo());
		addIEx("IndentEnable", "ImGuiTableColumnFlags_IndentEnable", int64_t(ImGuiTableColumnFlags_::ImGuiTableColumnFlags_IndentEnable), das::LineInfo());
		addIEx("IndentDisable", "ImGuiTableColumnFlags_IndentDisable", int64_t(ImGuiTableColumnFlags_::ImGuiTableColumnFlags_IndentDisable), das::LineInfo());
		addIEx("AngledHeader", "ImGuiTableColumnFlags_AngledHeader", int64_t(ImGuiTableColumnFlags_::ImGuiTableColumnFlags_AngledHeader), das::LineInfo());
		addIEx("IsEnabled", "ImGuiTableColumnFlags_IsEnabled", int64_t(ImGuiTableColumnFlags_::ImGuiTableColumnFlags_IsEnabled), das::LineInfo());
		addIEx("IsVisible", "ImGuiTableColumnFlags_IsVisible", int64_t(ImGuiTableColumnFlags_::ImGuiTableColumnFlags_IsVisible), das::LineInfo());
		addIEx("IsSorted", "ImGuiTableColumnFlags_IsSorted", int64_t(ImGuiTableColumnFlags_::ImGuiTableColumnFlags_IsSorted), das::LineInfo());
		addIEx("IsHovered", "ImGuiTableColumnFlags_IsHovered", int64_t(ImGuiTableColumnFlags_::ImGuiTableColumnFlags_IsHovered), das::LineInfo());
		addIEx("WidthMask_", "ImGuiTableColumnFlags_WidthMask_", int64_t(ImGuiTableColumnFlags_::ImGuiTableColumnFlags_WidthMask_), das::LineInfo());
		addIEx("IndentMask_", "ImGuiTableColumnFlags_IndentMask_", int64_t(ImGuiTableColumnFlags_::ImGuiTableColumnFlags_IndentMask_), das::LineInfo());
		addIEx("StatusMask_", "ImGuiTableColumnFlags_StatusMask_", int64_t(ImGuiTableColumnFlags_::ImGuiTableColumnFlags_StatusMask_), das::LineInfo());
		addIEx("NoDirectResize_", "ImGuiTableColumnFlags_NoDirectResize_", int64_t(ImGuiTableColumnFlags_::ImGuiTableColumnFlags_NoDirectResize_), das::LineInfo());
	}
};

// from imgui/imgui.h:2042:6
class Enumeration_ImGuiTableRowFlags_ : public das::Enumeration {
public:
	Enumeration_ImGuiTableRowFlags_() : das::Enumeration("ImGuiTableRowFlags") {
		external = true;
		cppName = "ImGuiTableRowFlags_";
		baseType = (das::Type) das::ToBasicType<int>::type;
		addIEx("None", "ImGuiTableRowFlags_None", int64_t(ImGuiTableRowFlags_::ImGuiTableRowFlags_None), das::LineInfo());
		addIEx("Headers", "ImGuiTableRowFlags_Headers", int64_t(ImGuiTableRowFlags_::ImGuiTableRowFlags_Headers), das::LineInfo());
	}
};

// from imgui/imgui.h:2057:6
class Enumeration_ImGuiTableBgTarget_ : public das::Enumeration {
public:
	Enumeration_ImGuiTableBgTarget_() : das::Enumeration("ImGuiTableBgTarget") {
		external = true;
		cppName = "ImGuiTableBgTarget_";
		baseType = (das::Type) das::ToBasicType<int>::type;
		addIEx("None", "ImGuiTableBgTarget_None", int64_t(ImGuiTableBgTarget_::ImGuiTableBgTarget_None), das::LineInfo());
		addIEx("RowBg0", "ImGuiTableBgTarget_RowBg0", int64_t(ImGuiTableBgTarget_::ImGuiTableBgTarget_RowBg0), das::LineInfo());
		addIEx("RowBg1", "ImGuiTableBgTarget_RowBg1", int64_t(ImGuiTableBgTarget_::ImGuiTableBgTarget_RowBg1), das::LineInfo());
		addIEx("CellBg", "ImGuiTableBgTarget_CellBg", int64_t(ImGuiTableBgTarget_::ImGuiTableBgTarget_CellBg), das::LineInfo());
	}
};

// from imgui/imgui.h:2895:6
class Enumeration_ImGuiMultiSelectFlags_ : public das::Enumeration {
public:
	Enumeration_ImGuiMultiSelectFlags_() : das::Enumeration("ImGuiMultiSelectFlags") {
		external = true;
		cppName = "ImGuiMultiSelectFlags_";
		baseType = (das::Type) das::ToBasicType<int>::type;
		addIEx("None", "ImGuiMultiSelectFlags_None", int64_t(ImGuiMultiSelectFlags_::ImGuiMultiSelectFlags_None), das::LineInfo());
		addIEx("SingleSelect", "ImGuiMultiSelectFlags_SingleSelect", int64_t(ImGuiMultiSelectFlags_::ImGuiMultiSelectFlags_SingleSelect), das::LineInfo());
		addIEx("NoSelectAll", "ImGuiMultiSelectFlags_NoSelectAll", int64_t(ImGuiMultiSelectFlags_::ImGuiMultiSelectFlags_NoSelectAll), das::LineInfo());
		addIEx("NoRangeSelect", "ImGuiMultiSelectFlags_NoRangeSelect", int64_t(ImGuiMultiSelectFlags_::ImGuiMultiSelectFlags_NoRangeSelect), das::LineInfo());
		addIEx("NoAutoSelect", "ImGuiMultiSelectFlags_NoAutoSelect", int64_t(ImGuiMultiSelectFlags_::ImGuiMultiSelectFlags_NoAutoSelect), das::LineInfo());
		addIEx("NoAutoClear", "ImGuiMultiSelectFlags_NoAutoClear", int64_t(ImGuiMultiSelectFlags_::ImGuiMultiSelectFlags_NoAutoClear), das::LineInfo());
		addIEx("NoAutoClearOnReselect", "ImGuiMultiSelectFlags_NoAutoClearOnReselect", int64_t(ImGuiMultiSelectFlags_::ImGuiMultiSelectFlags_NoAutoClearOnReselect), das::LineInfo());
		addIEx("BoxSelect1d", "ImGuiMultiSelectFlags_BoxSelect1d", int64_t(ImGuiMultiSelectFlags_::ImGuiMultiSelectFlags_BoxSelect1d), das::LineInfo());
		addIEx("BoxSelect2d", "ImGuiMultiSelectFlags_BoxSelect2d", int64_t(ImGuiMultiSelectFlags_::ImGuiMultiSelectFlags_BoxSelect2d), das::LineInfo());
		addIEx("BoxSelectNoScroll", "ImGuiMultiSelectFlags_BoxSelectNoScroll", int64_t(ImGuiMultiSelectFlags_::ImGuiMultiSelectFlags_BoxSelectNoScroll), das::LineInfo());
		addIEx("ClearOnEscape", "ImGuiMultiSelectFlags_ClearOnEscape", int64_t(ImGuiMultiSelectFlags_::ImGuiMultiSelectFlags_ClearOnEscape), das::LineInfo());
		addIEx("ClearOnClickVoid", "ImGuiMultiSelectFlags_ClearOnClickVoid", int64_t(ImGuiMultiSelectFlags_::ImGuiMultiSelectFlags_ClearOnClickVoid), das::LineInfo());
		addIEx("ScopeWindow", "ImGuiMultiSelectFlags_ScopeWindow", int64_t(ImGuiMultiSelectFlags_::ImGuiMultiSelectFlags_ScopeWindow), das::LineInfo());
		addIEx("ScopeRect", "ImGuiMultiSelectFlags_ScopeRect", int64_t(ImGuiMultiSelectFlags_::ImGuiMultiSelectFlags_ScopeRect), das::LineInfo());
		addIEx("SelectOnClick", "ImGuiMultiSelectFlags_SelectOnClick", int64_t(ImGuiMultiSelectFlags_::ImGuiMultiSelectFlags_SelectOnClick), das::LineInfo());
		addIEx("SelectOnClickRelease", "ImGuiMultiSelectFlags_SelectOnClickRelease", int64_t(ImGuiMultiSelectFlags_::ImGuiMultiSelectFlags_SelectOnClickRelease), das::LineInfo());
		addIEx("NavWrapX", "ImGuiMultiSelectFlags_NavWrapX", int64_t(ImGuiMultiSelectFlags_::ImGuiMultiSelectFlags_NavWrapX), das::LineInfo());
	}
};

// from imgui/imgui.h:2934:6
class Enumeration_ImGuiSelectionRequestType : public das::Enumeration {
public:
	Enumeration_ImGuiSelectionRequestType() : das::Enumeration("ImGuiSelectionRequestType") {
		external = true;
		cppName = "ImGuiSelectionRequestType";
		baseType = (das::Type) das::ToBasicType<int>::type;
		addIEx("None", "ImGuiSelectionRequestType_None", int64_t(ImGuiSelectionRequestType::ImGuiSelectionRequestType_None), das::LineInfo());
		addIEx("SetAll", "ImGuiSelectionRequestType_SetAll", int64_t(ImGuiSelectionRequestType::ImGuiSelectionRequestType_SetAll), das::LineInfo());
		addIEx("SetRange", "ImGuiSelectionRequestType_SetRange", int64_t(ImGuiSelectionRequestType::ImGuiSelectionRequestType_SetRange), das::LineInfo());
	}
};

// from imgui/imgui.h:3101:6
class Enumeration_ImDrawFlags_ : public das::Enumeration {
public:
	Enumeration_ImDrawFlags_() : das::Enumeration("ImDrawFlags") {
		external = true;
		cppName = "ImDrawFlags_";
		baseType = (das::Type) das::ToBasicType<int>::type;
		addIEx("None", "ImDrawFlags_None", int64_t(ImDrawFlags_::ImDrawFlags_None), das::LineInfo());
		addIEx("Closed", "ImDrawFlags_Closed", int64_t(ImDrawFlags_::ImDrawFlags_Closed), das::LineInfo());
		addIEx("RoundCornersTopLeft", "ImDrawFlags_RoundCornersTopLeft", int64_t(ImDrawFlags_::ImDrawFlags_RoundCornersTopLeft), das::LineInfo());
		addIEx("RoundCornersTopRight", "ImDrawFlags_RoundCornersTopRight", int64_t(ImDrawFlags_::ImDrawFlags_RoundCornersTopRight), das::LineInfo());
		addIEx("RoundCornersBottomLeft", "ImDrawFlags_RoundCornersBottomLeft", int64_t(ImDrawFlags_::ImDrawFlags_RoundCornersBottomLeft), das::LineInfo());
		addIEx("RoundCornersBottomRight", "ImDrawFlags_RoundCornersBottomRight", int64_t(ImDrawFlags_::ImDrawFlags_RoundCornersBottomRight), das::LineInfo());
		addIEx("RoundCornersNone", "ImDrawFlags_RoundCornersNone", int64_t(ImDrawFlags_::ImDrawFlags_RoundCornersNone), das::LineInfo());
		addIEx("RoundCornersTop", "ImDrawFlags_RoundCornersTop", int64_t(ImDrawFlags_::ImDrawFlags_RoundCornersTop), das::LineInfo());
		addIEx("RoundCornersBottom", "ImDrawFlags_RoundCornersBottom", int64_t(ImDrawFlags_::ImDrawFlags_RoundCornersBottom), das::LineInfo());
		addIEx("RoundCornersLeft", "ImDrawFlags_RoundCornersLeft", int64_t(ImDrawFlags_::ImDrawFlags_RoundCornersLeft), das::LineInfo());
		addIEx("RoundCornersRight", "ImDrawFlags_RoundCornersRight", int64_t(ImDrawFlags_::ImDrawFlags_RoundCornersRight), das::LineInfo());
		addIEx("RoundCornersAll", "ImDrawFlags_RoundCornersAll", int64_t(ImDrawFlags_::ImDrawFlags_RoundCornersAll), das::LineInfo());
		addIEx("RoundCornersDefault_", "ImDrawFlags_RoundCornersDefault_", int64_t(ImDrawFlags_::ImDrawFlags_RoundCornersDefault_), das::LineInfo());
		addIEx("RoundCornersMask_", "ImDrawFlags_RoundCornersMask_", int64_t(ImDrawFlags_::ImDrawFlags_RoundCornersMask_), das::LineInfo());
	}
};

// from imgui/imgui.h:3121:6
class Enumeration_ImDrawListFlags_ : public das::Enumeration {
public:
	Enumeration_ImDrawListFlags_() : das::Enumeration("ImDrawListFlags") {
		external = true;
		cppName = "ImDrawListFlags_";
		baseType = (das::Type) das::ToBasicType<int>::type;
		addIEx("None", "ImDrawListFlags_None", int64_t(ImDrawListFlags_::ImDrawListFlags_None), das::LineInfo());
		addIEx("AntiAliasedLines", "ImDrawListFlags_AntiAliasedLines", int64_t(ImDrawListFlags_::ImDrawListFlags_AntiAliasedLines), das::LineInfo());
		addIEx("AntiAliasedLinesUseTex", "ImDrawListFlags_AntiAliasedLinesUseTex", int64_t(ImDrawListFlags_::ImDrawListFlags_AntiAliasedLinesUseTex), das::LineInfo());
		addIEx("AntiAliasedFill", "ImDrawListFlags_AntiAliasedFill", int64_t(ImDrawListFlags_::ImDrawListFlags_AntiAliasedFill), das::LineInfo());
		addIEx("AllowVtxOffset", "ImDrawListFlags_AllowVtxOffset", int64_t(ImDrawListFlags_::ImDrawListFlags_AllowVtxOffset), das::LineInfo());
	}
};

// from imgui/imgui.h:3373:6
class Enumeration_ImFontAtlasFlags_ : public das::Enumeration {
public:
	Enumeration_ImFontAtlasFlags_() : das::Enumeration("ImFontAtlasFlags") {
		external = true;
		cppName = "ImFontAtlasFlags_";
		baseType = (das::Type) das::ToBasicType<int>::type;
		addIEx("None", "ImFontAtlasFlags_None", int64_t(ImFontAtlasFlags_::ImFontAtlasFlags_None), das::LineInfo());
		addIEx("NoPowerOfTwoHeight", "ImFontAtlasFlags_NoPowerOfTwoHeight", int64_t(ImFontAtlasFlags_::ImFontAtlasFlags_NoPowerOfTwoHeight), das::LineInfo());
		addIEx("NoMouseCursors", "ImFontAtlasFlags_NoMouseCursors", int64_t(ImFontAtlasFlags_::ImFontAtlasFlags_NoMouseCursors), das::LineInfo());
		addIEx("NoBakedLines", "ImFontAtlasFlags_NoBakedLines", int64_t(ImFontAtlasFlags_::ImFontAtlasFlags_NoBakedLines), das::LineInfo());
	}
};

// from imgui/imgui.h:3560:6
class Enumeration_ImGuiViewportFlags_ : public das::Enumeration {
public:
	Enumeration_ImGuiViewportFlags_() : das::Enumeration("ImGuiViewportFlags") {
		external = true;
		cppName = "ImGuiViewportFlags_";
		baseType = (das::Type) das::ToBasicType<int>::type;
		addIEx("None", "ImGuiViewportFlags_None", int64_t(ImGuiViewportFlags_::ImGuiViewportFlags_None), das::LineInfo());
		addIEx("IsPlatformWindow", "ImGuiViewportFlags_IsPlatformWindow", int64_t(ImGuiViewportFlags_::ImGuiViewportFlags_IsPlatformWindow), das::LineInfo());
		addIEx("IsPlatformMonitor", "ImGuiViewportFlags_IsPlatformMonitor", int64_t(ImGuiViewportFlags_::ImGuiViewportFlags_IsPlatformMonitor), das::LineInfo());
		addIEx("OwnedByApp", "ImGuiViewportFlags_OwnedByApp", int64_t(ImGuiViewportFlags_::ImGuiViewportFlags_OwnedByApp), das::LineInfo());
		addIEx("NoDecoration", "ImGuiViewportFlags_NoDecoration", int64_t(ImGuiViewportFlags_::ImGuiViewportFlags_NoDecoration), das::LineInfo());
		addIEx("NoTaskBarIcon", "ImGuiViewportFlags_NoTaskBarIcon", int64_t(ImGuiViewportFlags_::ImGuiViewportFlags_NoTaskBarIcon), das::LineInfo());
		addIEx("NoFocusOnAppearing", "ImGuiViewportFlags_NoFocusOnAppearing", int64_t(ImGuiViewportFlags_::ImGuiViewportFlags_NoFocusOnAppearing), das::LineInfo());
		addIEx("NoFocusOnClick", "ImGuiViewportFlags_NoFocusOnClick", int64_t(ImGuiViewportFlags_::ImGuiViewportFlags_NoFocusOnClick), das::LineInfo());
		addIEx("NoInputs", "ImGuiViewportFlags_NoInputs", int64_t(ImGuiViewportFlags_::ImGuiViewportFlags_NoInputs), das::LineInfo());
		addIEx("NoRendererClear", "ImGuiViewportFlags_NoRendererClear", int64_t(ImGuiViewportFlags_::ImGuiViewportFlags_NoRendererClear), das::LineInfo());
		addIEx("NoAutoMerge", "ImGuiViewportFlags_NoAutoMerge", int64_t(ImGuiViewportFlags_::ImGuiViewportFlags_NoAutoMerge), das::LineInfo());
		addIEx("TopMost", "ImGuiViewportFlags_TopMost", int64_t(ImGuiViewportFlags_::ImGuiViewportFlags_TopMost), das::LineInfo());
		addIEx("CanHostOtherWindows", "ImGuiViewportFlags_CanHostOtherWindows", int64_t(ImGuiViewportFlags_::ImGuiViewportFlags_CanHostOtherWindows), das::LineInfo());
		addIEx("IsMinimized", "ImGuiViewportFlags_IsMinimized", int64_t(ImGuiViewportFlags_::ImGuiViewportFlags_IsMinimized), das::LineInfo());
		addIEx("IsFocused", "ImGuiViewportFlags_IsFocused", int64_t(ImGuiViewportFlags_::ImGuiViewportFlags_IsFocused), das::LineInfo());
	}
};

