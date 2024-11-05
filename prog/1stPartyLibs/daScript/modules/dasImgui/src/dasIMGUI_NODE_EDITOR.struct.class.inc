// this file is generated via Daslang automatic C++ binder
// all user modifications will be lost after this file is re-generated

// from imgui-node-editor/imgui_node_editor.h:90:8
struct ax_NodeEditor_Config_GeneratedAnnotation : ManagedStructureAnnotation<ax::NodeEditor::Config> {
	virtual bool isLocal() const override { return true; }
	virtual bool canBePlacedInContainer() const override { return true; }
	ax_NodeEditor_Config_GeneratedAnnotation(ModuleLibrary & ml) : ManagedStructureAnnotation ("Config", ml, "ax::NodeEditor::Config") {
	}
	void init () {
		addField<DAS_BIND_MANAGED_FIELD(SettingsFile)>("SettingsFile","SettingsFile");
		addField<DAS_BIND_MANAGED_FIELD(UserPointer)>("UserPointer","UserPointer");
		addField<DAS_BIND_MANAGED_FIELD(CustomZoomLevels)>("CustomZoomLevels","CustomZoomLevels");
		addField<DAS_BIND_MANAGED_FIELD(CanvasSizeMode)>("CanvasSizeMode","CanvasSizeMode");
		addField<DAS_BIND_MANAGED_FIELD(DragButtonIndex)>("DragButtonIndex","DragButtonIndex");
		addField<DAS_BIND_MANAGED_FIELD(SelectButtonIndex)>("SelectButtonIndex","SelectButtonIndex");
		addField<DAS_BIND_MANAGED_FIELD(NavigateButtonIndex)>("NavigateButtonIndex","NavigateButtonIndex");
		addField<DAS_BIND_MANAGED_FIELD(ContextMenuButtonIndex)>("ContextMenuButtonIndex","ContextMenuButtonIndex");
	}
};

// from imgui-node-editor/imgui_node_editor.h:188:8
struct ax_NodeEditor_Style_GeneratedAnnotation : ManagedStructureAnnotation<ax::NodeEditor::Style> {
	ax_NodeEditor_Style_GeneratedAnnotation(ModuleLibrary & ml) : ManagedStructureAnnotation ("Style", ml, "ax::NodeEditor::Style") {
	}
	void init () {
		addField<DAS_BIND_MANAGED_FIELD(NodePadding)>("NodePadding","NodePadding");
		addField<DAS_BIND_MANAGED_FIELD(NodeRounding)>("NodeRounding","NodeRounding");
		addField<DAS_BIND_MANAGED_FIELD(NodeBorderWidth)>("NodeBorderWidth","NodeBorderWidth");
		addField<DAS_BIND_MANAGED_FIELD(HoveredNodeBorderWidth)>("HoveredNodeBorderWidth","HoveredNodeBorderWidth");
		addField<DAS_BIND_MANAGED_FIELD(HoverNodeBorderOffset)>("HoverNodeBorderOffset","HoverNodeBorderOffset");
		addField<DAS_BIND_MANAGED_FIELD(SelectedNodeBorderWidth)>("SelectedNodeBorderWidth","SelectedNodeBorderWidth");
		addField<DAS_BIND_MANAGED_FIELD(SelectedNodeBorderOffset)>("SelectedNodeBorderOffset","SelectedNodeBorderOffset");
		addField<DAS_BIND_MANAGED_FIELD(PinRounding)>("PinRounding","PinRounding");
		addField<DAS_BIND_MANAGED_FIELD(PinBorderWidth)>("PinBorderWidth","PinBorderWidth");
		addField<DAS_BIND_MANAGED_FIELD(LinkStrength)>("LinkStrength","LinkStrength");
		addField<DAS_BIND_MANAGED_FIELD(SourceDirection)>("SourceDirection","SourceDirection");
		addField<DAS_BIND_MANAGED_FIELD(TargetDirection)>("TargetDirection","TargetDirection");
		addField<DAS_BIND_MANAGED_FIELD(ScrollDuration)>("ScrollDuration","ScrollDuration");
		addField<DAS_BIND_MANAGED_FIELD(FlowMarkerDistance)>("FlowMarkerDistance","FlowMarkerDistance");
		addField<DAS_BIND_MANAGED_FIELD(FlowSpeed)>("FlowSpeed","FlowSpeed");
		addField<DAS_BIND_MANAGED_FIELD(FlowDuration)>("FlowDuration","FlowDuration");
		addField<DAS_BIND_MANAGED_FIELD(PivotAlignment)>("PivotAlignment","PivotAlignment");
		addField<DAS_BIND_MANAGED_FIELD(PivotSize)>("PivotSize","PivotSize");
		addField<DAS_BIND_MANAGED_FIELD(PivotScale)>("PivotScale","PivotScale");
		addField<DAS_BIND_MANAGED_FIELD(PinCorners)>("PinCorners","PinCorners");
		addField<DAS_BIND_MANAGED_FIELD(PinRadius)>("PinRadius","PinRadius");
		addField<DAS_BIND_MANAGED_FIELD(PinArrowSize)>("PinArrowSize","PinArrowSize");
		addField<DAS_BIND_MANAGED_FIELD(PinArrowWidth)>("PinArrowWidth","PinArrowWidth");
		addField<DAS_BIND_MANAGED_FIELD(GroupRounding)>("GroupRounding","GroupRounding");
		addField<DAS_BIND_MANAGED_FIELD(GroupBorderWidth)>("GroupBorderWidth","GroupBorderWidth");
		addField<DAS_BIND_MANAGED_FIELD(HighlightConnectedLinks)>("HighlightConnectedLinks","HighlightConnectedLinks");
		addField<DAS_BIND_MANAGED_FIELD(SnapLinkToPinDir)>("SnapLinkToPinDir","SnapLinkToPinDir");
		addField<DAS_BIND_MANAGED_FIELD(Colors)>("Colors","Colors");
	}
};

