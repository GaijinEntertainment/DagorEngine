// this file is generated via daScript automatic C++ binder
// all user modifications will be lost after this file is re-generated

template <> struct cast_arg<SQWeakRef> {
	static __forceinline const SQWeakRef & to ( Context & ctx, SimNode * node ) {
		vec4f res = node->eval(ctx);
		return * cast<SQWeakRef *>::to(res);
	}
};
template <> struct cast_arg<tagSQObjectValue> {
	static __forceinline const tagSQObjectValue & to ( Context & ctx, SimNode * node ) {
		vec4f res = node->eval(ctx);
		return * cast<tagSQObjectValue *>::to(res);
	}
};
template <> struct cast_arg<tagSQObject> {
	static __forceinline const tagSQObject & to ( Context & ctx, SimNode * node ) {
		vec4f res = node->eval(ctx);
		return * cast<tagSQObject *>::to(res);
	}
};
template <> struct cast_arg<tagSQMemberHandle> {
	static __forceinline const tagSQMemberHandle & to ( Context & ctx, SimNode * node ) {
		vec4f res = node->eval(ctx);
		return * cast<tagSQMemberHandle *>::to(res);
	}
};
template <> struct cast_arg<tagSQStackInfos> {
	static __forceinline const tagSQStackInfos & to ( Context & ctx, SimNode * node ) {
		vec4f res = node->eval(ctx);
		return * cast<tagSQStackInfos *>::to(res);
	}
};
template <> struct cast_arg<tagSQRegFunction> {
	static __forceinline const tagSQRegFunction & to ( Context & ctx, SimNode * node ) {
		vec4f res = node->eval(ctx);
		return * cast<tagSQRegFunction *>::to(res);
	}
};
template <> struct cast_arg<tagSQFunctionInfo> {
	static __forceinline const tagSQFunctionInfo & to ( Context & ctx, SimNode * node ) {
		vec4f res = node->eval(ctx);
		return * cast<tagSQFunctionInfo *>::to(res);
	}
};
template <> struct cast_arg<SQStream> {
	static __forceinline const SQStream & to ( Context & ctx, SimNode * node ) {
		vec4f res = node->eval(ctx);
		return * cast<SQStream *>::to(res);
	}
};
template <> struct cast_arg<SQRexMatch> {
	static __forceinline const SQRexMatch & to ( Context & ctx, SimNode * node ) {
		vec4f res = node->eval(ctx);
		return * cast<SQRexMatch *>::to(res);
	}
};
