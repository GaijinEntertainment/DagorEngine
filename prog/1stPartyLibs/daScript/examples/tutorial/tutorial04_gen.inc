class TutorialBaseClass {
protected:
  enum {
    __fn_update = 0,
    __fn_get_position = 1,
  };
protected:
  static int _das_class_method_offset[2];
public:
  TutorialBaseClass ( const StructInfo * info ) {
    if ( _das_class_method_offset[0]==0 ) {
      _das_class_method_offset[__fn_update] = adapt_field_offset("update",info);
      _das_class_method_offset[__fn_get_position] = adapt_field_offset("get_position",info);
    }
  }
  static __forceinline Func get_update ( void * self ) {
    return getDasClassMethod(self,_das_class_method_offset[__fn_update]);
  }
  static __forceinline void invoke_update ( Context * __context__, Func __funcCall__, void * self, float dt ) {
    das_invoke_function<void>::invoke
      <void *,float>
        (__context__,nullptr,__funcCall__,
          self,dt);
  }
  static __forceinline Func get_get_position ( void * self ) {
    return getDasClassMethod(self,_das_class_method_offset[__fn_get_position]);
  }
  static __forceinline float3 invoke_get_position ( Context * __context__, Func __funcCall__, void * self ) {
    return das_invoke_function<float3>::invoke
      <void *>
        (__context__,nullptr,__funcCall__,
          self);
  }
};
int TutorialBaseClass::_das_class_method_offset[2];

