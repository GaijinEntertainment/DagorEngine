// this file is generated via Daslang automatic C++ binder
// all user modifications will be lost after this file is re-generated

// from imgui-node-editor/imgui_node_editor.h:46:12
class Enumeration_ax_NodeEditor_PinKind : public das::Enumeration {
public:
	Enumeration_ax_NodeEditor_PinKind() : das::Enumeration("PinKind") {
		external = true;
		cppName = "ax::NodeEditor::PinKind";
		baseType = (das::Type) das::ToBasicType<int>::type;
		addIEx("Input", "Input", int64_t(ax::NodeEditor::PinKind::Input), das::LineInfo());
		addIEx("Output", "Output", int64_t(ax::NodeEditor::PinKind::Output), das::LineInfo());
	}
};

// from imgui-node-editor/imgui_node_editor.h:52:12
class Enumeration_ax_NodeEditor_FlowDirection : public das::Enumeration {
public:
	Enumeration_ax_NodeEditor_FlowDirection() : das::Enumeration("FlowDirection") {
		external = true;
		cppName = "ax::NodeEditor::FlowDirection";
		baseType = (das::Type) das::ToBasicType<int>::type;
		addIEx("Forward", "Forward", int64_t(ax::NodeEditor::FlowDirection::Forward), das::LineInfo());
		addIEx("Backward", "Backward", int64_t(ax::NodeEditor::FlowDirection::Backward), das::LineInfo());
	}
};

// from imgui-node-editor/imgui_node_editor.h:58:12
class Enumeration_ax_NodeEditor_CanvasSizeMode : public das::Enumeration {
public:
	Enumeration_ax_NodeEditor_CanvasSizeMode() : das::Enumeration("CanvasSizeMode") {
		external = true;
		cppName = "ax::NodeEditor::CanvasSizeMode";
		baseType = (das::Type) das::ToBasicType<int>::type;
		addIEx("FitVerticalView", "FitVerticalView", int64_t(ax::NodeEditor::CanvasSizeMode::FitVerticalView), das::LineInfo());
		addIEx("FitHorizontalView", "FitHorizontalView", int64_t(ax::NodeEditor::CanvasSizeMode::FitHorizontalView), das::LineInfo());
		addIEx("CenterOnly", "CenterOnly", int64_t(ax::NodeEditor::CanvasSizeMode::CenterOnly), das::LineInfo());
	}
};

// from imgui-node-editor/imgui_node_editor.h:67:12
class Enumeration_ax_NodeEditor_SaveReasonFlags : public das::Enumeration {
public:
	Enumeration_ax_NodeEditor_SaveReasonFlags() : das::Enumeration("SaveReasonFlags") {
		external = true;
		cppName = "ax::NodeEditor::SaveReasonFlags";
		baseType = (das::Type) das::ToBasicType<uint32_t>::type;
		addIEx("None", "None", int64_t(ax::NodeEditor::SaveReasonFlags::None), das::LineInfo());
		addIEx("Navigation", "Navigation", int64_t(ax::NodeEditor::SaveReasonFlags::Navigation), das::LineInfo());
		addIEx("Position", "Position", int64_t(ax::NodeEditor::SaveReasonFlags::Position), das::LineInfo());
		addIEx("Size", "Size", int64_t(ax::NodeEditor::SaveReasonFlags::Size), das::LineInfo());
		addIEx("Selection", "Selection", int64_t(ax::NodeEditor::SaveReasonFlags::Selection), das::LineInfo());
		addIEx("AddNode", "AddNode", int64_t(ax::NodeEditor::SaveReasonFlags::AddNode), das::LineInfo());
		addIEx("RemoveNode", "RemoveNode", int64_t(ax::NodeEditor::SaveReasonFlags::RemoveNode), das::LineInfo());
		addIEx("User", "User", int64_t(ax::NodeEditor::SaveReasonFlags::User), das::LineInfo());
	}
};

// from imgui-node-editor/imgui_node_editor.h:130:6
class Enumeration_ax_NodeEditor_StyleColor : public das::Enumeration {
public:
	Enumeration_ax_NodeEditor_StyleColor() : das::Enumeration("StyleColor") {
		external = true;
		cppName = "ax::NodeEditor::StyleColor";
		baseType = (das::Type) das::ToBasicType<int>::type;
		addIEx("Bg", "StyleColor_Bg", int64_t(ax::NodeEditor::StyleColor::StyleColor_Bg), das::LineInfo());
		addIEx("Grid", "StyleColor_Grid", int64_t(ax::NodeEditor::StyleColor::StyleColor_Grid), das::LineInfo());
		addIEx("NodeBg", "StyleColor_NodeBg", int64_t(ax::NodeEditor::StyleColor::StyleColor_NodeBg), das::LineInfo());
		addIEx("NodeBorder", "StyleColor_NodeBorder", int64_t(ax::NodeEditor::StyleColor::StyleColor_NodeBorder), das::LineInfo());
		addIEx("HovNodeBorder", "StyleColor_HovNodeBorder", int64_t(ax::NodeEditor::StyleColor::StyleColor_HovNodeBorder), das::LineInfo());
		addIEx("SelNodeBorder", "StyleColor_SelNodeBorder", int64_t(ax::NodeEditor::StyleColor::StyleColor_SelNodeBorder), das::LineInfo());
		addIEx("NodeSelRect", "StyleColor_NodeSelRect", int64_t(ax::NodeEditor::StyleColor::StyleColor_NodeSelRect), das::LineInfo());
		addIEx("NodeSelRectBorder", "StyleColor_NodeSelRectBorder", int64_t(ax::NodeEditor::StyleColor::StyleColor_NodeSelRectBorder), das::LineInfo());
		addIEx("HovLinkBorder", "StyleColor_HovLinkBorder", int64_t(ax::NodeEditor::StyleColor::StyleColor_HovLinkBorder), das::LineInfo());
		addIEx("SelLinkBorder", "StyleColor_SelLinkBorder", int64_t(ax::NodeEditor::StyleColor::StyleColor_SelLinkBorder), das::LineInfo());
		addIEx("HighlightLinkBorder", "StyleColor_HighlightLinkBorder", int64_t(ax::NodeEditor::StyleColor::StyleColor_HighlightLinkBorder), das::LineInfo());
		addIEx("LinkSelRect", "StyleColor_LinkSelRect", int64_t(ax::NodeEditor::StyleColor::StyleColor_LinkSelRect), das::LineInfo());
		addIEx("LinkSelRectBorder", "StyleColor_LinkSelRectBorder", int64_t(ax::NodeEditor::StyleColor::StyleColor_LinkSelRectBorder), das::LineInfo());
		addIEx("PinRect", "StyleColor_PinRect", int64_t(ax::NodeEditor::StyleColor::StyleColor_PinRect), das::LineInfo());
		addIEx("PinRectBorder", "StyleColor_PinRectBorder", int64_t(ax::NodeEditor::StyleColor::StyleColor_PinRectBorder), das::LineInfo());
		addIEx("Flow", "StyleColor_Flow", int64_t(ax::NodeEditor::StyleColor::StyleColor_Flow), das::LineInfo());
		addIEx("FlowMarker", "StyleColor_FlowMarker", int64_t(ax::NodeEditor::StyleColor::StyleColor_FlowMarker), das::LineInfo());
		addIEx("GroupBg", "StyleColor_GroupBg", int64_t(ax::NodeEditor::StyleColor::StyleColor_GroupBg), das::LineInfo());
		addIEx("GroupBorder", "StyleColor_GroupBorder", int64_t(ax::NodeEditor::StyleColor::StyleColor_GroupBorder), das::LineInfo());
		addIEx("Count", "StyleColor_Count", int64_t(ax::NodeEditor::StyleColor::StyleColor_Count), das::LineInfo());
	}
};

// from imgui-node-editor/imgui_node_editor.h:155:6
class Enumeration_ax_NodeEditor_StyleVar : public das::Enumeration {
public:
	Enumeration_ax_NodeEditor_StyleVar() : das::Enumeration("StyleVar") {
		external = true;
		cppName = "ax::NodeEditor::StyleVar";
		baseType = (das::Type) das::ToBasicType<int>::type;
		addIEx("NodePadding", "StyleVar_NodePadding", int64_t(ax::NodeEditor::StyleVar::StyleVar_NodePadding), das::LineInfo());
		addIEx("NodeRounding", "StyleVar_NodeRounding", int64_t(ax::NodeEditor::StyleVar::StyleVar_NodeRounding), das::LineInfo());
		addIEx("NodeBorderWidth", "StyleVar_NodeBorderWidth", int64_t(ax::NodeEditor::StyleVar::StyleVar_NodeBorderWidth), das::LineInfo());
		addIEx("HoveredNodeBorderWidth", "StyleVar_HoveredNodeBorderWidth", int64_t(ax::NodeEditor::StyleVar::StyleVar_HoveredNodeBorderWidth), das::LineInfo());
		addIEx("SelectedNodeBorderWidth", "StyleVar_SelectedNodeBorderWidth", int64_t(ax::NodeEditor::StyleVar::StyleVar_SelectedNodeBorderWidth), das::LineInfo());
		addIEx("PinRounding", "StyleVar_PinRounding", int64_t(ax::NodeEditor::StyleVar::StyleVar_PinRounding), das::LineInfo());
		addIEx("PinBorderWidth", "StyleVar_PinBorderWidth", int64_t(ax::NodeEditor::StyleVar::StyleVar_PinBorderWidth), das::LineInfo());
		addIEx("LinkStrength", "StyleVar_LinkStrength", int64_t(ax::NodeEditor::StyleVar::StyleVar_LinkStrength), das::LineInfo());
		addIEx("SourceDirection", "StyleVar_SourceDirection", int64_t(ax::NodeEditor::StyleVar::StyleVar_SourceDirection), das::LineInfo());
		addIEx("TargetDirection", "StyleVar_TargetDirection", int64_t(ax::NodeEditor::StyleVar::StyleVar_TargetDirection), das::LineInfo());
		addIEx("ScrollDuration", "StyleVar_ScrollDuration", int64_t(ax::NodeEditor::StyleVar::StyleVar_ScrollDuration), das::LineInfo());
		addIEx("FlowMarkerDistance", "StyleVar_FlowMarkerDistance", int64_t(ax::NodeEditor::StyleVar::StyleVar_FlowMarkerDistance), das::LineInfo());
		addIEx("FlowSpeed", "StyleVar_FlowSpeed", int64_t(ax::NodeEditor::StyleVar::StyleVar_FlowSpeed), das::LineInfo());
		addIEx("FlowDuration", "StyleVar_FlowDuration", int64_t(ax::NodeEditor::StyleVar::StyleVar_FlowDuration), das::LineInfo());
		addIEx("PivotAlignment", "StyleVar_PivotAlignment", int64_t(ax::NodeEditor::StyleVar::StyleVar_PivotAlignment), das::LineInfo());
		addIEx("PivotSize", "StyleVar_PivotSize", int64_t(ax::NodeEditor::StyleVar::StyleVar_PivotSize), das::LineInfo());
		addIEx("PivotScale", "StyleVar_PivotScale", int64_t(ax::NodeEditor::StyleVar::StyleVar_PivotScale), das::LineInfo());
		addIEx("PinCorners", "StyleVar_PinCorners", int64_t(ax::NodeEditor::StyleVar::StyleVar_PinCorners), das::LineInfo());
		addIEx("PinRadius", "StyleVar_PinRadius", int64_t(ax::NodeEditor::StyleVar::StyleVar_PinRadius), das::LineInfo());
		addIEx("PinArrowSize", "StyleVar_PinArrowSize", int64_t(ax::NodeEditor::StyleVar::StyleVar_PinArrowSize), das::LineInfo());
		addIEx("PinArrowWidth", "StyleVar_PinArrowWidth", int64_t(ax::NodeEditor::StyleVar::StyleVar_PinArrowWidth), das::LineInfo());
		addIEx("GroupRounding", "StyleVar_GroupRounding", int64_t(ax::NodeEditor::StyleVar::StyleVar_GroupRounding), das::LineInfo());
		addIEx("GroupBorderWidth", "StyleVar_GroupBorderWidth", int64_t(ax::NodeEditor::StyleVar::StyleVar_GroupBorderWidth), das::LineInfo());
		addIEx("HighlightConnectedLinks", "StyleVar_HighlightConnectedLinks", int64_t(ax::NodeEditor::StyleVar::StyleVar_HighlightConnectedLinks), das::LineInfo());
		addIEx("SnapLinkToPinDir", "StyleVar_SnapLinkToPinDir", int64_t(ax::NodeEditor::StyleVar::StyleVar_SnapLinkToPinDir), das::LineInfo());
		addIEx("HoveredNodeBorderOffset", "StyleVar_HoveredNodeBorderOffset", int64_t(ax::NodeEditor::StyleVar::StyleVar_HoveredNodeBorderOffset), das::LineInfo());
		addIEx("SelectedNodeBorderOffset", "StyleVar_SelectedNodeBorderOffset", int64_t(ax::NodeEditor::StyleVar::StyleVar_SelectedNodeBorderOffset), das::LineInfo());
		addIEx("Count", "StyleVar_Count", int64_t(ax::NodeEditor::StyleVar::StyleVar_Count), das::LineInfo());
	}
};

