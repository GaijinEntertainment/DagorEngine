// this file is generated via Daslang automatic C++ binder
// all user modifications will be lost after this file is re-generated

template <> struct cast_arg<ax::NodeEditor::Config> {
	static __forceinline const ax::NodeEditor::Config & to ( Context & ctx, SimNode * node ) {
		vec4f res = node->eval(ctx);
		return * cast<ax::NodeEditor::Config *>::to(res);
	}
};
template <> struct cast_arg<ax::NodeEditor::Style> {
	static __forceinline const ax::NodeEditor::Style & to ( Context & ctx, SimNode * node ) {
		vec4f res = node->eval(ctx);
		return * cast<ax::NodeEditor::Style *>::to(res);
	}
};
