class AstFunctionAnnotation_Adapter {
protected:
  enum {
    __fn_transform = 0,
    __fn_verifyCall = 1,
    __fn_apply = 2,
    __fn_generic_apply = 3,
    __fn_finish = 4,
    __fn_patch = 5,
    __fn_fixup = 6,
    __fn_lint = 7,
    __fn_complete = 8,
    __fn_isCompatible = 9,
    __fn_isSpecialized = 10,
    __fn_appendToMangledName = 11,
  };
protected:
  int _das_class_method_offset[12];
public:
  AstFunctionAnnotation_Adapter ( const StructInfo * info ) {
      _das_class_method_offset[__fn_transform] = adapt_field_offset("transform",info);
      _das_class_method_offset[__fn_verifyCall] = adapt_field_offset("verifyCall",info);
      _das_class_method_offset[__fn_apply] = adapt_field_offset("apply",info);
      _das_class_method_offset[__fn_generic_apply] = adapt_field_offset("generic_apply",info);
      _das_class_method_offset[__fn_finish] = adapt_field_offset("finish",info);
      _das_class_method_offset[__fn_patch] = adapt_field_offset("patch",info);
      _das_class_method_offset[__fn_fixup] = adapt_field_offset("fixup",info);
      _das_class_method_offset[__fn_lint] = adapt_field_offset("lint",info);
      _das_class_method_offset[__fn_complete] = adapt_field_offset("complete",info);
      _das_class_method_offset[__fn_isCompatible] = adapt_field_offset("isCompatible",info);
      _das_class_method_offset[__fn_isSpecialized] = adapt_field_offset("isSpecialized",info);
      _das_class_method_offset[__fn_appendToMangledName] = adapt_field_offset("appendToMangledName",info);
  }
  __forceinline Func get_transform ( void * self ) const {
    return getDasClassMethod(self,_das_class_method_offset[__fn_transform]);
  }
  __forceinline smart_ptr<Expression> invoke_transform ( Context * __context__, Func __funcCall__, void * self, smart_ptr_raw<ExprCallFunc> call, das::string & errors ) const {
    return das_invoke_function<smart_ptr_raw<Expression>>::invoke
      <void *,smart_ptr_raw<ExprCallFunc>,das::string &>
        (__context__,nullptr,__funcCall__,
          self,call,errors).marshal();
  }
  __forceinline Func get_verifyCall ( void * self ) const {
    return getDasClassMethod(self,_das_class_method_offset[__fn_verifyCall]);
  }
  __forceinline bool invoke_verifyCall ( Context * __context__, Func __funcCall__, void * self, smart_ptr_raw<ExprCallFunc> call, AnnotationArgumentList const  & args, AnnotationArgumentList const  & progArgs, das::string & errors ) const {
    return das_invoke_function<bool>::invoke
      <void *,smart_ptr_raw<ExprCallFunc>,AnnotationArgumentList const  &,AnnotationArgumentList const  &,das::string &>
        (__context__,nullptr,__funcCall__,
          self,call,args,progArgs,errors);
  }
  __forceinline Func get_apply ( void * self ) const {
    return getDasClassMethod(self,_das_class_method_offset[__fn_apply]);
  }
  __forceinline bool invoke_apply ( Context * __context__, Func __funcCall__, void * self, smart_ptr_raw<Function> func, ModuleGroup & group, AnnotationArgumentList const  & args, das::string & errors ) const {
    return das_invoke_function<bool>::invoke
      <void *,smart_ptr_raw<Function>,ModuleGroup &,AnnotationArgumentList const  &,das::string &>
        (__context__,nullptr,__funcCall__,
          self,func,group,args,errors);
  }
  __forceinline Func get_generic_apply ( void * self ) const {
    return getDasClassMethod(self,_das_class_method_offset[__fn_generic_apply]);
  }
  __forceinline bool invoke_generic_apply ( Context * __context__, Func __funcCall__, void * self, smart_ptr_raw<Function> func, ModuleGroup & group, AnnotationArgumentList const  & args, das::string & errors ) const {
    return das_invoke_function<bool>::invoke
      <void *,smart_ptr_raw<Function>,ModuleGroup &,AnnotationArgumentList const  &,das::string &>
        (__context__,nullptr,__funcCall__,
          self,func,group,args,errors);
  }
  __forceinline Func get_finish ( void * self ) const {
    return getDasClassMethod(self,_das_class_method_offset[__fn_finish]);
  }
  __forceinline bool invoke_finish ( Context * __context__, Func __funcCall__, void * self, smart_ptr_raw<Function> func, ModuleGroup & group, AnnotationArgumentList const  & args, AnnotationArgumentList const  & progArgs, das::string & errors ) const {
    return das_invoke_function<bool>::invoke
      <void *,smart_ptr_raw<Function>,ModuleGroup &,AnnotationArgumentList const  &,AnnotationArgumentList const  &,das::string &>
        (__context__,nullptr,__funcCall__,
          self,func,group,args,progArgs,errors);
  }
  __forceinline Func get_patch ( void * self ) const {
    return getDasClassMethod(self,_das_class_method_offset[__fn_patch]);
  }
  __forceinline bool invoke_patch ( Context * __context__, Func __funcCall__, void * self, smart_ptr_raw<Function> func, ModuleGroup & group, AnnotationArgumentList const  & args, AnnotationArgumentList const  & progArgs, das::string & errors, bool & astChanged ) const {
    return das_invoke_function<bool>::invoke
      <void *,smart_ptr_raw<Function>,ModuleGroup &,AnnotationArgumentList const  &,AnnotationArgumentList const  &,das::string &,bool &>
        (__context__,nullptr,__funcCall__,
          self,func,group,args,progArgs,errors,astChanged);
  }
  __forceinline Func get_fixup ( void * self ) const {
    return getDasClassMethod(self,_das_class_method_offset[__fn_fixup]);
  }
  __forceinline bool invoke_fixup ( Context * __context__, Func __funcCall__, void * self, smart_ptr_raw<Function> func, ModuleGroup & group, AnnotationArgumentList const  & args, AnnotationArgumentList const  & progArgs, das::string & errors ) const {
    return das_invoke_function<bool>::invoke
      <void *,smart_ptr_raw<Function>,ModuleGroup &,AnnotationArgumentList const  &,AnnotationArgumentList const  &,das::string &>
        (__context__,nullptr,__funcCall__,
          self,func,group,args,progArgs,errors);
  }
  __forceinline Func get_lint ( void * self ) const {
    return getDasClassMethod(self,_das_class_method_offset[__fn_lint]);
  }
  __forceinline bool invoke_lint ( Context * __context__, Func __funcCall__, void * self, smart_ptr_raw<Function> func, ModuleGroup & group, AnnotationArgumentList const  & args, AnnotationArgumentList const  & progArgs, das::string & errors ) const {
    return das_invoke_function<bool>::invoke
      <void *,smart_ptr_raw<Function>,ModuleGroup &,AnnotationArgumentList const  &,AnnotationArgumentList const  &,das::string &>
        (__context__,nullptr,__funcCall__,
          self,func,group,args,progArgs,errors);
  }
  __forceinline Func get_complete ( void * self ) const {
    return getDasClassMethod(self,_das_class_method_offset[__fn_complete]);
  }
  __forceinline void invoke_complete ( Context * __context__, Func __funcCall__, void * self, smart_ptr_raw<Function> func, smart_ptr_raw<Context> ctx ) const {
    das_invoke_function<void>::invoke
      <void *,smart_ptr_raw<Function>,smart_ptr_raw<Context>>
        (__context__,nullptr,__funcCall__,
          self,func,ctx);
  }
  __forceinline Func get_isCompatible ( void * self ) const {
    return getDasClassMethod(self,_das_class_method_offset[__fn_isCompatible]);
  }
  __forceinline bool invoke_isCompatible ( Context * __context__, Func __funcCall__, void * self, smart_ptr_raw<Function> func, das::vector<smart_ptr<TypeDecl>> & types, AnnotationDeclaration const  & decl, das::string & errors ) const {
    return das_invoke_function<bool>::invoke
      <void *,smart_ptr_raw<Function>,das::vector<smart_ptr<TypeDecl>> &,AnnotationDeclaration const  &,das::string &>
        (__context__,nullptr,__funcCall__,
          self,func,types,decl,errors);
  }
  __forceinline Func get_isSpecialized ( void * self ) const {
    return getDasClassMethod(self,_das_class_method_offset[__fn_isSpecialized]);
  }
  __forceinline bool invoke_isSpecialized ( Context * __context__, Func __funcCall__, void * self ) const {
    return das_invoke_function<bool>::invoke
      <void *>
        (__context__,nullptr,__funcCall__,
          self);
  }
  __forceinline Func get_appendToMangledName ( void * self ) const {
    return getDasClassMethod(self,_das_class_method_offset[__fn_appendToMangledName]);
  }
  __forceinline void invoke_appendToMangledName ( Context * __context__, Func __funcCall__, void * self, smart_ptr_raw<Function> const  func, AnnotationDeclaration const  & decl, das::string & mangledName ) const {
    das_invoke_function<void>::invoke
      <void *,smart_ptr_raw<Function> const ,AnnotationDeclaration const  &,das::string &>
        (__context__,nullptr,__funcCall__,
          self,func,decl,mangledName);
  }
};

class AstBlockAnnotation_Adapter {
protected:
  enum {
    __fn_apply = 0,
    __fn_finish = 1,
  };
protected:
  int _das_class_method_offset[2];
public:
  AstBlockAnnotation_Adapter ( const StructInfo * info ) {
      _das_class_method_offset[__fn_apply] = adapt_field_offset("apply",info);
      _das_class_method_offset[__fn_finish] = adapt_field_offset("finish",info);
  }
  __forceinline Func get_apply ( void * self ) const {
    return getDasClassMethod(self,_das_class_method_offset[__fn_apply]);
  }
  __forceinline bool invoke_apply ( Context * __context__, Func __funcCall__, void * self, smart_ptr_raw<ExprBlock> blk, ModuleGroup & group, AnnotationArgumentList const  & args, das::string & errors ) const {
    return das_invoke_function<bool>::invoke
      <void *,smart_ptr_raw<ExprBlock>,ModuleGroup &,AnnotationArgumentList const  &,das::string &>
        (__context__,nullptr,__funcCall__,
          self,blk,group,args,errors);
  }
  __forceinline Func get_finish ( void * self ) const {
    return getDasClassMethod(self,_das_class_method_offset[__fn_finish]);
  }
  __forceinline bool invoke_finish ( Context * __context__, Func __funcCall__, void * self, smart_ptr_raw<ExprBlock> blk, ModuleGroup & group, AnnotationArgumentList const  & args, AnnotationArgumentList const  & progArgs, das::string & errors ) const {
    return das_invoke_function<bool>::invoke
      <void *,smart_ptr_raw<ExprBlock>,ModuleGroup &,AnnotationArgumentList const  &,AnnotationArgumentList const  &,das::string &>
        (__context__,nullptr,__funcCall__,
          self,blk,group,args,progArgs,errors);
  }
};

class AstStructureAnnotation_Adapter {
protected:
  enum {
    __fn_apply = 0,
    __fn_finish = 1,
    __fn_patch = 2,
    __fn_complete = 3,
    __fn_aotPrefix = 4,
    __fn_aotBody = 5,
    __fn_aotSuffix = 6,
  };
protected:
  int _das_class_method_offset[7];
public:
  AstStructureAnnotation_Adapter ( const StructInfo * info ) {
      _das_class_method_offset[__fn_apply] = adapt_field_offset("apply",info);
      _das_class_method_offset[__fn_finish] = adapt_field_offset("finish",info);
      _das_class_method_offset[__fn_patch] = adapt_field_offset("patch",info);
      _das_class_method_offset[__fn_complete] = adapt_field_offset("complete",info);
      _das_class_method_offset[__fn_aotPrefix] = adapt_field_offset("aotPrefix",info);
      _das_class_method_offset[__fn_aotBody] = adapt_field_offset("aotBody",info);
      _das_class_method_offset[__fn_aotSuffix] = adapt_field_offset("aotSuffix",info);
  }
  __forceinline Func get_apply ( void * self ) const {
    return getDasClassMethod(self,_das_class_method_offset[__fn_apply]);
  }
  __forceinline bool invoke_apply ( Context * __context__, Func __funcCall__, void * self, smart_ptr_raw<Structure> st, ModuleGroup & group, AnnotationArgumentList const  & args, das::string & errors ) const {
    return das_invoke_function<bool>::invoke
      <void *,smart_ptr_raw<Structure>,ModuleGroup &,AnnotationArgumentList const  &,das::string &>
        (__context__,nullptr,__funcCall__,
          self,st,group,args,errors);
  }
  __forceinline Func get_finish ( void * self ) const {
    return getDasClassMethod(self,_das_class_method_offset[__fn_finish]);
  }
  __forceinline bool invoke_finish ( Context * __context__, Func __funcCall__, void * self, smart_ptr_raw<Structure> st, ModuleGroup & group, AnnotationArgumentList const  & args, das::string & errors ) const {
    return das_invoke_function<bool>::invoke
      <void *,smart_ptr_raw<Structure>,ModuleGroup &,AnnotationArgumentList const  &,das::string &>
        (__context__,nullptr,__funcCall__,
          self,st,group,args,errors);
  }
  __forceinline Func get_patch ( void * self ) const {
    return getDasClassMethod(self,_das_class_method_offset[__fn_patch]);
  }
  __forceinline bool invoke_patch ( Context * __context__, Func __funcCall__, void * self, smart_ptr_raw<Structure> st, ModuleGroup & group, AnnotationArgumentList const  & args, das::string & errors, bool & astChanged ) const {
    return das_invoke_function<bool>::invoke
      <void *,smart_ptr_raw<Structure>,ModuleGroup &,AnnotationArgumentList const  &,das::string &,bool &>
        (__context__,nullptr,__funcCall__,
          self,st,group,args,errors,astChanged);
  }
  __forceinline Func get_complete ( void * self ) const {
    return getDasClassMethod(self,_das_class_method_offset[__fn_complete]);
  }
  __forceinline void invoke_complete ( Context * __context__, Func __funcCall__, void * self, smart_ptr_raw<Structure> st, smart_ptr_raw<Context> ctx ) const {
    das_invoke_function<void>::invoke
      <void *,smart_ptr_raw<Structure>,smart_ptr_raw<Context>>
        (__context__,nullptr,__funcCall__,
          self,st,ctx);
  }
  __forceinline Func get_aotPrefix ( void * self ) const {
    return getDasClassMethod(self,_das_class_method_offset[__fn_aotPrefix]);
  }
  __forceinline void invoke_aotPrefix ( Context * __context__, Func __funcCall__, void * self, smart_ptr_raw<Structure> st, AnnotationArgumentList const  & args, StringBuilderWriter & writer ) const {
    das_invoke_function<void>::invoke
      <void *,smart_ptr_raw<Structure>,AnnotationArgumentList const  &,StringBuilderWriter &>
        (__context__,nullptr,__funcCall__,
          self,st,args,writer);
  }
  __forceinline Func get_aotBody ( void * self ) const {
    return getDasClassMethod(self,_das_class_method_offset[__fn_aotBody]);
  }
  __forceinline void invoke_aotBody ( Context * __context__, Func __funcCall__, void * self, smart_ptr_raw<Structure> st, AnnotationArgumentList const  & args, StringBuilderWriter & writer ) const {
    das_invoke_function<void>::invoke
      <void *,smart_ptr_raw<Structure>,AnnotationArgumentList const  &,StringBuilderWriter &>
        (__context__,nullptr,__funcCall__,
          self,st,args,writer);
  }
  __forceinline Func get_aotSuffix ( void * self ) const {
    return getDasClassMethod(self,_das_class_method_offset[__fn_aotSuffix]);
  }
  __forceinline void invoke_aotSuffix ( Context * __context__, Func __funcCall__, void * self, smart_ptr_raw<Structure> st, AnnotationArgumentList const  & args, StringBuilderWriter & writer ) const {
    das_invoke_function<void>::invoke
      <void *,smart_ptr_raw<Structure>,AnnotationArgumentList const  &,StringBuilderWriter &>
        (__context__,nullptr,__funcCall__,
          self,st,args,writer);
  }
};

class AstEnumerationAnnotation_Adapter {
protected:
  enum {
    __fn_apply = 0,
  };
protected:
  int _das_class_method_offset[1];
public:
  AstEnumerationAnnotation_Adapter ( const StructInfo * info ) {
      _das_class_method_offset[__fn_apply] = adapt_field_offset("apply",info);
  }
  __forceinline Func get_apply ( void * self ) const {
    return getDasClassMethod(self,_das_class_method_offset[__fn_apply]);
  }
  __forceinline bool invoke_apply ( Context * __context__, Func __funcCall__, void * self, smart_ptr_raw<Enumeration> st, ModuleGroup & group, AnnotationArgumentList const  & args, das::string & errors ) const {
    return das_invoke_function<bool>::invoke
      <void *,smart_ptr_raw<Enumeration>,ModuleGroup &,AnnotationArgumentList const  &,das::string &>
        (__context__,nullptr,__funcCall__,
          self,st,group,args,errors);
  }
};

class AstPassMacro_Adapter {
protected:
  enum {
    __fn_apply = 0,
  };
protected:
  int _das_class_method_offset[1];
public:
  AstPassMacro_Adapter ( const StructInfo * info ) {
      _das_class_method_offset[__fn_apply] = adapt_field_offset("apply",info);
  }
  __forceinline Func get_apply ( void * self ) const {
    return getDasClassMethod(self,_das_class_method_offset[__fn_apply]);
  }
  __forceinline bool invoke_apply ( Context * __context__, Func __funcCall__, void * self, smart_ptr_raw<Program> const  prog, Module * const  mod ) const {
    return das_invoke_function<bool>::invoke
      <void *,smart_ptr_raw<Program> const ,Module * const >
        (__context__,nullptr,__funcCall__,
          self,prog,mod);
  }
};

class AstVarianMacro_Adapter {
protected:
  enum {
    __fn_visitExprIsVariant = 0,
    __fn_visitExprAsVariant = 1,
    __fn_visitExprSafeAsVariant = 2,
  };
protected:
  int _das_class_method_offset[3];
public:
  AstVarianMacro_Adapter ( const StructInfo * info ) {
      _das_class_method_offset[__fn_visitExprIsVariant] = adapt_field_offset("visitExprIsVariant",info);
      _das_class_method_offset[__fn_visitExprAsVariant] = adapt_field_offset("visitExprAsVariant",info);
      _das_class_method_offset[__fn_visitExprSafeAsVariant] = adapt_field_offset("visitExprSafeAsVariant",info);
  }
  __forceinline Func get_visitExprIsVariant ( void * self ) const {
    return getDasClassMethod(self,_das_class_method_offset[__fn_visitExprIsVariant]);
  }
  __forceinline smart_ptr<Expression> invoke_visitExprIsVariant ( Context * __context__, Func __funcCall__, void * self, smart_ptr_raw<Program> const  prog, Module * const  mod, smart_ptr_raw<ExprIsVariant> const  expr ) const {
    return das_invoke_function<smart_ptr_raw<Expression>>::invoke
      <void *,smart_ptr_raw<Program> const ,Module * const ,smart_ptr_raw<ExprIsVariant> const >
        (__context__,nullptr,__funcCall__,
          self,prog,mod,expr).marshal();
  }
  __forceinline Func get_visitExprAsVariant ( void * self ) const {
    return getDasClassMethod(self,_das_class_method_offset[__fn_visitExprAsVariant]);
  }
  __forceinline smart_ptr<Expression> invoke_visitExprAsVariant ( Context * __context__, Func __funcCall__, void * self, smart_ptr_raw<Program> const  prog, Module * const  mod, smart_ptr_raw<ExprAsVariant> const  expr ) const {
    return das_invoke_function<smart_ptr_raw<Expression>>::invoke
      <void *,smart_ptr_raw<Program> const ,Module * const ,smart_ptr_raw<ExprAsVariant> const >
        (__context__,nullptr,__funcCall__,
          self,prog,mod,expr).marshal();
  }
  __forceinline Func get_visitExprSafeAsVariant ( void * self ) const {
    return getDasClassMethod(self,_das_class_method_offset[__fn_visitExprSafeAsVariant]);
  }
  __forceinline smart_ptr<Expression> invoke_visitExprSafeAsVariant ( Context * __context__, Func __funcCall__, void * self, smart_ptr_raw<Program> const  prog, Module * const  mod, smart_ptr_raw<ExprSafeAsVariant> const  expr ) const {
    return das_invoke_function<smart_ptr_raw<Expression>>::invoke
      <void *,smart_ptr_raw<Program> const ,Module * const ,smart_ptr_raw<ExprSafeAsVariant> const >
        (__context__,nullptr,__funcCall__,
          self,prog,mod,expr).marshal();
  }
};

class AstForLoopMacro_Adapter {
protected:
  enum {
    __fn_visitExprFor = 0,
  };
protected:
  int _das_class_method_offset[1];
public:
  AstForLoopMacro_Adapter ( const StructInfo * info ) {
      _das_class_method_offset[__fn_visitExprFor] = adapt_field_offset("visitExprFor",info);
  }
  __forceinline Func get_visitExprFor ( void * self ) const {
    return getDasClassMethod(self,_das_class_method_offset[__fn_visitExprFor]);
  }
  __forceinline smart_ptr<Expression> invoke_visitExprFor ( Context * __context__, Func __funcCall__, void * self, smart_ptr_raw<Program> const  prog, Module * const  mod, smart_ptr_raw<ExprFor> const  expr ) const {
    return das_invoke_function<smart_ptr_raw<Expression>>::invoke
      <void *,smart_ptr_raw<Program> const ,Module * const ,smart_ptr_raw<ExprFor> const >
        (__context__,nullptr,__funcCall__,
          self,prog,mod,expr).marshal();
  }
};

class AstCaptureMacro_Adapter {
protected:
  enum {
    __fn_captureExpression = 0,
    __fn_captureFunction = 1,
  };
protected:
  int _das_class_method_offset[2];
public:
  AstCaptureMacro_Adapter ( const StructInfo * info ) {
      _das_class_method_offset[__fn_captureExpression] = adapt_field_offset("captureExpression",info);
      _das_class_method_offset[__fn_captureFunction] = adapt_field_offset("captureFunction",info);
  }
  __forceinline Func get_captureExpression ( void * self ) const {
    return getDasClassMethod(self,_das_class_method_offset[__fn_captureExpression]);
  }
  __forceinline smart_ptr<Expression> invoke_captureExpression ( Context * __context__, Func __funcCall__, void * self, Program * const  prog, Module * const  mod, smart_ptr_raw<Expression> const  expr, smart_ptr_raw<TypeDecl> const  etype ) const {
    return das_invoke_function<smart_ptr_raw<Expression>>::invoke
      <void *,Program * const ,Module * const ,smart_ptr_raw<Expression> const ,smart_ptr_raw<TypeDecl> const >
        (__context__,nullptr,__funcCall__,
          self,prog,mod,expr,etype).marshal();
  }
  __forceinline Func get_captureFunction ( void * self ) const {
    return getDasClassMethod(self,_das_class_method_offset[__fn_captureFunction]);
  }
  __forceinline void invoke_captureFunction ( Context * __context__, Func __funcCall__, void * self, Program * const  prog, Module * const  mod, Structure * lcs, smart_ptr_raw<Function> fun ) const {
    das_invoke_function<void>::invoke
      <void *,Program * const ,Module * const ,Structure *,smart_ptr_raw<Function>>
        (__context__,nullptr,__funcCall__,
          self,prog,mod,lcs,fun);
  }
};

class AstSimulateMacro_Adapter {
protected:
  enum {
    __fn_preSimulate = 0,
    __fn_simulate = 1,
  };
protected:
  int _das_class_method_offset[2];
public:
  AstSimulateMacro_Adapter ( const StructInfo * info ) {
      _das_class_method_offset[__fn_preSimulate] = adapt_field_offset("preSimulate",info);
      _das_class_method_offset[__fn_simulate] = adapt_field_offset("simulate",info);
  }
  __forceinline Func get_preSimulate ( void * self ) const {
    return getDasClassMethod(self,_das_class_method_offset[__fn_preSimulate]);
  }
  __forceinline bool invoke_preSimulate ( Context * __context__, Func __funcCall__, void * self, Program * const  prog, Context * const  ctx ) const {
    return das_invoke_function<bool>::invoke
      <void *,Program * const ,Context * const >
        (__context__,nullptr,__funcCall__,
          self,prog,ctx);
  }
  __forceinline Func get_simulate ( void * self ) const {
    return getDasClassMethod(self,_das_class_method_offset[__fn_simulate]);
  }
  __forceinline bool invoke_simulate ( Context * __context__, Func __funcCall__, void * self, Program * const  prog, Context * const  ctx ) const {
    return das_invoke_function<bool>::invoke
      <void *,Program * const ,Context * const >
        (__context__,nullptr,__funcCall__,
          self,prog,ctx);
  }
};

class AstReaderMacro_Adapter {
protected:
  enum {
    __fn_accept = 0,
    __fn_visit = 1,
  };
protected:
  int _das_class_method_offset[2];
public:
  AstReaderMacro_Adapter ( const StructInfo * info ) {
      _das_class_method_offset[__fn_accept] = adapt_field_offset("accept",info);
      _das_class_method_offset[__fn_visit] = adapt_field_offset("visit",info);
  }
  __forceinline Func get_accept ( void * self ) const {
    return getDasClassMethod(self,_das_class_method_offset[__fn_accept]);
  }
  __forceinline bool invoke_accept ( Context * __context__, Func __funcCall__, void * self, smart_ptr_raw<Program> const  prog, Module * const  mod, ExprReader * const  expr, int32_t ch, LineInfo const  & info ) const {
    return das_invoke_function<bool>::invoke
      <void *,smart_ptr_raw<Program> const ,Module * const ,ExprReader * const ,int32_t,LineInfo const  &>
        (__context__,nullptr,__funcCall__,
          self,prog,mod,expr,ch,info);
  }
  __forceinline Func get_visit ( void * self ) const {
    return getDasClassMethod(self,_das_class_method_offset[__fn_visit]);
  }
  __forceinline smart_ptr<Expression> invoke_visit ( Context * __context__, Func __funcCall__, void * self, smart_ptr_raw<Program> const  prog, Module * const  mod, smart_ptr_raw<ExprReader> const  expr ) const {
    return das_invoke_function<smart_ptr_raw<Expression>>::invoke
      <void *,smart_ptr_raw<Program> const ,Module * const ,smart_ptr_raw<ExprReader> const >
        (__context__,nullptr,__funcCall__,
          self,prog,mod,expr).marshal();
  }
};

class AstCommentReader_Adapter {
protected:
  enum {
    __fn_open = 0,
    __fn_accept = 1,
    __fn_close = 2,
    __fn_beforeStructure = 3,
    __fn_afterStructure = 4,
    __fn_beforeStructureFields = 5,
    __fn_afterStructureField = 6,
    __fn_afterStructureFields = 7,
    __fn_beforeFunction = 8,
    __fn_afterFunction = 9,
    __fn_beforeGlobalVariables = 10,
    __fn_afterGlobalVariable = 11,
    __fn_afterGlobalVariables = 12,
    __fn_beforeVariant = 13,
    __fn_afterVariant = 14,
    __fn_beforeEnumeration = 15,
    __fn_afterEnumeration = 16,
    __fn_beforeAlias = 17,
    __fn_afterAlias = 18,
  };
protected:
  int _das_class_method_offset[19];
public:
  AstCommentReader_Adapter ( const StructInfo * info ) {
      _das_class_method_offset[__fn_open] = adapt_field_offset("open",info);
      _das_class_method_offset[__fn_accept] = adapt_field_offset("accept",info);
      _das_class_method_offset[__fn_close] = adapt_field_offset("close",info);
      _das_class_method_offset[__fn_beforeStructure] = adapt_field_offset("beforeStructure",info);
      _das_class_method_offset[__fn_afterStructure] = adapt_field_offset("afterStructure",info);
      _das_class_method_offset[__fn_beforeStructureFields] = adapt_field_offset("beforeStructureFields",info);
      _das_class_method_offset[__fn_afterStructureField] = adapt_field_offset("afterStructureField",info);
      _das_class_method_offset[__fn_afterStructureFields] = adapt_field_offset("afterStructureFields",info);
      _das_class_method_offset[__fn_beforeFunction] = adapt_field_offset("beforeFunction",info);
      _das_class_method_offset[__fn_afterFunction] = adapt_field_offset("afterFunction",info);
      _das_class_method_offset[__fn_beforeGlobalVariables] = adapt_field_offset("beforeGlobalVariables",info);
      _das_class_method_offset[__fn_afterGlobalVariable] = adapt_field_offset("afterGlobalVariable",info);
      _das_class_method_offset[__fn_afterGlobalVariables] = adapt_field_offset("afterGlobalVariables",info);
      _das_class_method_offset[__fn_beforeVariant] = adapt_field_offset("beforeVariant",info);
      _das_class_method_offset[__fn_afterVariant] = adapt_field_offset("afterVariant",info);
      _das_class_method_offset[__fn_beforeEnumeration] = adapt_field_offset("beforeEnumeration",info);
      _das_class_method_offset[__fn_afterEnumeration] = adapt_field_offset("afterEnumeration",info);
      _das_class_method_offset[__fn_beforeAlias] = adapt_field_offset("beforeAlias",info);
      _das_class_method_offset[__fn_afterAlias] = adapt_field_offset("afterAlias",info);
  }
  __forceinline Func get_open ( void * self ) const {
    return getDasClassMethod(self,_das_class_method_offset[__fn_open]);
  }
  __forceinline void invoke_open ( Context * __context__, Func __funcCall__, void * self, smart_ptr_raw<Program> const  prog, Module * const  mod, bool cpp, LineInfo const  & info ) const {
    das_invoke_function<void>::invoke
      <void *,smart_ptr_raw<Program> const ,Module * const ,bool,LineInfo const  &>
        (__context__,nullptr,__funcCall__,
          self,prog,mod,cpp,info);
  }
  __forceinline Func get_accept ( void * self ) const {
    return getDasClassMethod(self,_das_class_method_offset[__fn_accept]);
  }
  __forceinline void invoke_accept ( Context * __context__, Func __funcCall__, void * self, smart_ptr_raw<Program> const  prog, Module * const  mod, int32_t ch, LineInfo const  & info ) const {
    das_invoke_function<void>::invoke
      <void *,smart_ptr_raw<Program> const ,Module * const ,int32_t,LineInfo const  &>
        (__context__,nullptr,__funcCall__,
          self,prog,mod,ch,info);
  }
  __forceinline Func get_close ( void * self ) const {
    return getDasClassMethod(self,_das_class_method_offset[__fn_close]);
  }
  __forceinline void invoke_close ( Context * __context__, Func __funcCall__, void * self, smart_ptr_raw<Program> const  prog, Module * const  mod, LineInfo const  & info ) const {
    das_invoke_function<void>::invoke
      <void *,smart_ptr_raw<Program> const ,Module * const ,LineInfo const  &>
        (__context__,nullptr,__funcCall__,
          self,prog,mod,info);
  }
  __forceinline Func get_beforeStructure ( void * self ) const {
    return getDasClassMethod(self,_das_class_method_offset[__fn_beforeStructure]);
  }
  __forceinline void invoke_beforeStructure ( Context * __context__, Func __funcCall__, void * self, smart_ptr_raw<Program> const  prog, Module * const  mod, LineInfo const  & info ) const {
    das_invoke_function<void>::invoke
      <void *,smart_ptr_raw<Program> const ,Module * const ,LineInfo const  &>
        (__context__,nullptr,__funcCall__,
          self,prog,mod,info);
  }
  __forceinline Func get_afterStructure ( void * self ) const {
    return getDasClassMethod(self,_das_class_method_offset[__fn_afterStructure]);
  }
  __forceinline void invoke_afterStructure ( Context * __context__, Func __funcCall__, void * self, smart_ptr_raw<Structure> const  st, smart_ptr_raw<Program> const  prog, Module * const  mod, LineInfo const  & info ) const {
    das_invoke_function<void>::invoke
      <void *,smart_ptr_raw<Structure> const ,smart_ptr_raw<Program> const ,Module * const ,LineInfo const  &>
        (__context__,nullptr,__funcCall__,
          self,st,prog,mod,info);
  }
  __forceinline Func get_beforeStructureFields ( void * self ) const {
    return getDasClassMethod(self,_das_class_method_offset[__fn_beforeStructureFields]);
  }
  __forceinline void invoke_beforeStructureFields ( Context * __context__, Func __funcCall__, void * self, smart_ptr_raw<Program> const  prog, Module * const  mod, LineInfo const  & info ) const {
    das_invoke_function<void>::invoke
      <void *,smart_ptr_raw<Program> const ,Module * const ,LineInfo const  &>
        (__context__,nullptr,__funcCall__,
          self,prog,mod,info);
  }
  __forceinline Func get_afterStructureField ( void * self ) const {
    return getDasClassMethod(self,_das_class_method_offset[__fn_afterStructureField]);
  }
  __forceinline void invoke_afterStructureField ( Context * __context__, Func __funcCall__, void * self, char * const  name, smart_ptr_raw<Program> const  prog, Module * const  mod, LineInfo const  & info ) const {
    das_invoke_function<void>::invoke
      <void *,char * const ,smart_ptr_raw<Program> const ,Module * const ,LineInfo const  &>
        (__context__,nullptr,__funcCall__,
          self,name,prog,mod,info);
  }
  __forceinline Func get_afterStructureFields ( void * self ) const {
    return getDasClassMethod(self,_das_class_method_offset[__fn_afterStructureFields]);
  }
  __forceinline void invoke_afterStructureFields ( Context * __context__, Func __funcCall__, void * self, smart_ptr_raw<Program> const  prog, Module * const  mod, LineInfo const  & info ) const {
    das_invoke_function<void>::invoke
      <void *,smart_ptr_raw<Program> const ,Module * const ,LineInfo const  &>
        (__context__,nullptr,__funcCall__,
          self,prog,mod,info);
  }
  __forceinline Func get_beforeFunction ( void * self ) const {
    return getDasClassMethod(self,_das_class_method_offset[__fn_beforeFunction]);
  }
  __forceinline void invoke_beforeFunction ( Context * __context__, Func __funcCall__, void * self, smart_ptr_raw<Program> const  prog, Module * const  mod, LineInfo const  & info ) const {
    das_invoke_function<void>::invoke
      <void *,smart_ptr_raw<Program> const ,Module * const ,LineInfo const  &>
        (__context__,nullptr,__funcCall__,
          self,prog,mod,info);
  }
  __forceinline Func get_afterFunction ( void * self ) const {
    return getDasClassMethod(self,_das_class_method_offset[__fn_afterFunction]);
  }
  __forceinline void invoke_afterFunction ( Context * __context__, Func __funcCall__, void * self, smart_ptr_raw<Function> const  fn, smart_ptr_raw<Program> const  prog, Module * const  mod, LineInfo const  & info ) const {
    das_invoke_function<void>::invoke
      <void *,smart_ptr_raw<Function> const ,smart_ptr_raw<Program> const ,Module * const ,LineInfo const  &>
        (__context__,nullptr,__funcCall__,
          self,fn,prog,mod,info);
  }
  __forceinline Func get_beforeGlobalVariables ( void * self ) const {
    return getDasClassMethod(self,_das_class_method_offset[__fn_beforeGlobalVariables]);
  }
  __forceinline void invoke_beforeGlobalVariables ( Context * __context__, Func __funcCall__, void * self, smart_ptr_raw<Program> const  prog, Module * const  mod, LineInfo const  & info ) const {
    das_invoke_function<void>::invoke
      <void *,smart_ptr_raw<Program> const ,Module * const ,LineInfo const  &>
        (__context__,nullptr,__funcCall__,
          self,prog,mod,info);
  }
  __forceinline Func get_afterGlobalVariable ( void * self ) const {
    return getDasClassMethod(self,_das_class_method_offset[__fn_afterGlobalVariable]);
  }
  __forceinline void invoke_afterGlobalVariable ( Context * __context__, Func __funcCall__, void * self, char * const  name, smart_ptr_raw<Program> const  prog, Module * const  mod, LineInfo const  & info ) const {
    das_invoke_function<void>::invoke
      <void *,char * const ,smart_ptr_raw<Program> const ,Module * const ,LineInfo const  &>
        (__context__,nullptr,__funcCall__,
          self,name,prog,mod,info);
  }
  __forceinline Func get_afterGlobalVariables ( void * self ) const {
    return getDasClassMethod(self,_das_class_method_offset[__fn_afterGlobalVariables]);
  }
  __forceinline void invoke_afterGlobalVariables ( Context * __context__, Func __funcCall__, void * self, smart_ptr_raw<Program> const  prog, Module * const  mod, LineInfo const  & info ) const {
    das_invoke_function<void>::invoke
      <void *,smart_ptr_raw<Program> const ,Module * const ,LineInfo const  &>
        (__context__,nullptr,__funcCall__,
          self,prog,mod,info);
  }
  __forceinline Func get_beforeVariant ( void * self ) const {
    return getDasClassMethod(self,_das_class_method_offset[__fn_beforeVariant]);
  }
  __forceinline void invoke_beforeVariant ( Context * __context__, Func __funcCall__, void * self, smart_ptr_raw<Program> const  prog, Module * const  mod, LineInfo const  & info ) const {
    das_invoke_function<void>::invoke
      <void *,smart_ptr_raw<Program> const ,Module * const ,LineInfo const  &>
        (__context__,nullptr,__funcCall__,
          self,prog,mod,info);
  }
  __forceinline Func get_afterVariant ( void * self ) const {
    return getDasClassMethod(self,_das_class_method_offset[__fn_afterVariant]);
  }
  __forceinline void invoke_afterVariant ( Context * __context__, Func __funcCall__, void * self, char * const  name, smart_ptr_raw<Program> const  prog, Module * const  mod, LineInfo const  & info ) const {
    das_invoke_function<void>::invoke
      <void *,char * const ,smart_ptr_raw<Program> const ,Module * const ,LineInfo const  &>
        (__context__,nullptr,__funcCall__,
          self,name,prog,mod,info);
  }
  __forceinline Func get_beforeEnumeration ( void * self ) const {
    return getDasClassMethod(self,_das_class_method_offset[__fn_beforeEnumeration]);
  }
  __forceinline void invoke_beforeEnumeration ( Context * __context__, Func __funcCall__, void * self, smart_ptr_raw<Program> const  prog, Module * const  mod, LineInfo const  & info ) const {
    das_invoke_function<void>::invoke
      <void *,smart_ptr_raw<Program> const ,Module * const ,LineInfo const  &>
        (__context__,nullptr,__funcCall__,
          self,prog,mod,info);
  }
  __forceinline Func get_afterEnumeration ( void * self ) const {
    return getDasClassMethod(self,_das_class_method_offset[__fn_afterEnumeration]);
  }
  __forceinline void invoke_afterEnumeration ( Context * __context__, Func __funcCall__, void * self, char * const  name, smart_ptr_raw<Program> const  prog, Module * const  mod, LineInfo const  & info ) const {
    das_invoke_function<void>::invoke
      <void *,char * const ,smart_ptr_raw<Program> const ,Module * const ,LineInfo const  &>
        (__context__,nullptr,__funcCall__,
          self,name,prog,mod,info);
  }
  __forceinline Func get_beforeAlias ( void * self ) const {
    return getDasClassMethod(self,_das_class_method_offset[__fn_beforeAlias]);
  }
  __forceinline void invoke_beforeAlias ( Context * __context__, Func __funcCall__, void * self, smart_ptr_raw<Program> const  prog, Module * const  mod, LineInfo const  & info ) const {
    das_invoke_function<void>::invoke
      <void *,smart_ptr_raw<Program> const ,Module * const ,LineInfo const  &>
        (__context__,nullptr,__funcCall__,
          self,prog,mod,info);
  }
  __forceinline Func get_afterAlias ( void * self ) const {
    return getDasClassMethod(self,_das_class_method_offset[__fn_afterAlias]);
  }
  __forceinline void invoke_afterAlias ( Context * __context__, Func __funcCall__, void * self, char * const  name, smart_ptr_raw<Program> const  prog, Module * const  mod, LineInfo const  & info ) const {
    das_invoke_function<void>::invoke
      <void *,char * const ,smart_ptr_raw<Program> const ,Module * const ,LineInfo const  &>
        (__context__,nullptr,__funcCall__,
          self,name,prog,mod,info);
  }
};

class AstCallMacro_Adapter {
protected:
  enum {
    __fn_preVisit = 0,
    __fn_visit = 1,
    __fn_canVisitArgument = 2,
    __fn_canFoldReturnResult = 3,
  };
protected:
  int _das_class_method_offset[4];
public:
  AstCallMacro_Adapter ( const StructInfo * info ) {
      _das_class_method_offset[__fn_preVisit] = adapt_field_offset("preVisit",info);
      _das_class_method_offset[__fn_visit] = adapt_field_offset("visit",info);
      _das_class_method_offset[__fn_canVisitArgument] = adapt_field_offset("canVisitArgument",info);
      _das_class_method_offset[__fn_canFoldReturnResult] = adapt_field_offset("canFoldReturnResult",info);
  }
  __forceinline Func get_preVisit ( void * self ) const {
    return getDasClassMethod(self,_das_class_method_offset[__fn_preVisit]);
  }
  __forceinline void invoke_preVisit ( Context * __context__, Func __funcCall__, void * self, smart_ptr_raw<Program> const  prog, Module * const  mod, smart_ptr_raw<ExprCallMacro> const  expr ) const {
    das_invoke_function<void>::invoke
      <void *,smart_ptr_raw<Program> const ,Module * const ,smart_ptr_raw<ExprCallMacro> const >
        (__context__,nullptr,__funcCall__,
          self,prog,mod,expr);
  }
  __forceinline Func get_visit ( void * self ) const {
    return getDasClassMethod(self,_das_class_method_offset[__fn_visit]);
  }
  __forceinline smart_ptr<Expression> invoke_visit ( Context * __context__, Func __funcCall__, void * self, smart_ptr_raw<Program> const  prog, Module * const  mod, smart_ptr_raw<ExprCallMacro> const  expr ) const {
    return das_invoke_function<smart_ptr_raw<Expression>>::invoke
      <void *,smart_ptr_raw<Program> const ,Module * const ,smart_ptr_raw<ExprCallMacro> const >
        (__context__,nullptr,__funcCall__,
          self,prog,mod,expr).marshal();
  }
  __forceinline Func get_canVisitArgument ( void * self ) const {
    return getDasClassMethod(self,_das_class_method_offset[__fn_canVisitArgument]);
  }
  __forceinline bool invoke_canVisitArgument ( Context * __context__, Func __funcCall__, void * self, smart_ptr_raw<ExprCallMacro> const  expr, int32_t argIndex ) const {
    return das_invoke_function<bool>::invoke
      <void *,smart_ptr_raw<ExprCallMacro> const ,int32_t>
        (__context__,nullptr,__funcCall__,
          self,expr,argIndex);
  }
  __forceinline Func get_canFoldReturnResult ( void * self ) const {
    return getDasClassMethod(self,_das_class_method_offset[__fn_canFoldReturnResult]);
  }
  __forceinline bool invoke_canFoldReturnResult ( Context * __context__, Func __funcCall__, void * self, smart_ptr_raw<ExprCallMacro> const  expr ) const {
    return das_invoke_function<bool>::invoke
      <void *,smart_ptr_raw<ExprCallMacro> const >
        (__context__,nullptr,__funcCall__,
          self,expr);
  }
};

class AstTypeInfoMacro_Adapter {
protected:
  enum {
    __fn_getAstChange = 0,
    __fn_getAstType = 1,
  };
protected:
  int _das_class_method_offset[2];
public:
  AstTypeInfoMacro_Adapter ( const StructInfo * info ) {
      _das_class_method_offset[__fn_getAstChange] = adapt_field_offset("getAstChange",info);
      _das_class_method_offset[__fn_getAstType] = adapt_field_offset("getAstType",info);
  }
  __forceinline Func get_getAstChange ( void * self ) const {
    return getDasClassMethod(self,_das_class_method_offset[__fn_getAstChange]);
  }
  __forceinline smart_ptr<Expression> invoke_getAstChange ( Context * __context__, Func __funcCall__, void * self, smart_ptr_raw<ExprTypeInfo> const  expr, das::string & errors ) const {
    return das_invoke_function<smart_ptr_raw<Expression>>::invoke
      <void *,smart_ptr_raw<ExprTypeInfo> const ,das::string &>
        (__context__,nullptr,__funcCall__,
          self,expr,errors).marshal();
  }
  __forceinline Func get_getAstType ( void * self ) const {
    return getDasClassMethod(self,_das_class_method_offset[__fn_getAstType]);
  }
  __forceinline smart_ptr<TypeDecl> invoke_getAstType ( Context * __context__, Func __funcCall__, void * self, ModuleLibrary & lib, smart_ptr_raw<ExprTypeInfo> const  expr, das::string & errors ) const {
    return das_invoke_function<smart_ptr_raw<TypeDecl>>::invoke
      <void *,ModuleLibrary &,smart_ptr_raw<ExprTypeInfo> const ,das::string &>
        (__context__,nullptr,__funcCall__,
          self,lib,expr,errors).marshal();
  }
};

