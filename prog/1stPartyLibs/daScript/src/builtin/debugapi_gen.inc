class DapiDataWalker_Adapter {
protected:
  enum {
    __fn_canVisitHandle = 0,
    __fn_canVisitStructure = 1,
    __fn_canVisitArray = 2,
    __fn_canVisitArrayData = 3,
    __fn_canVisitTuple = 4,
    __fn_canVisitVariant = 5,
    __fn_canVisitTable = 6,
    __fn_canVisitTableData = 7,
    __fn_canVisitPointer = 8,
    __fn_canVisitLambda = 9,
    __fn_canVisitIterator = 10,
    __fn_beforeStructure = 11,
    __fn_afterStructure = 12,
    __fn_afterStructureCancel = 13,
    __fn_beforeStructureField = 14,
    __fn_afterStructureField = 15,
    __fn_beforeTuple = 16,
    __fn_afterTuple = 17,
    __fn_beforeTupleEntry = 18,
    __fn_afterTupleEntry = 19,
    __fn_beforeVariant = 20,
    __fn_afterVariant = 21,
    __fn_beforeArrayData = 22,
    __fn_afterArrayData = 23,
    __fn_beforeArrayElement = 24,
    __fn_afterArrayElement = 25,
    __fn_beforeDim = 26,
    __fn_afterDim = 27,
    __fn_beforeArray = 28,
    __fn_afterArray = 29,
    __fn_beforeTable = 30,
    __fn_beforeTableKey = 31,
    __fn_afterTableKey = 32,
    __fn_beforeTableValue = 33,
    __fn_afterTableValue = 34,
    __fn_afterTable = 35,
    __fn_beforeRef = 36,
    __fn_afterRef = 37,
    __fn_beforePtr = 38,
    __fn_afterPtr = 39,
    __fn_beforeHandle = 40,
    __fn_afterHandle = 41,
    __fn_beforeLambda = 42,
    __fn_afterLambda = 43,
    __fn_beforeIterator = 44,
    __fn_afterIterator = 45,
    __fn_Null = 46,
    __fn_VoidPtr = 47,
    __fn_Bool = 48,
    __fn_Int8 = 49,
    __fn_UInt8 = 50,
    __fn_Int16 = 51,
    __fn_UInt16 = 52,
    __fn_Int64 = 53,
    __fn_UInt64 = 54,
    __fn_String = 55,
    __fn_Double = 56,
    __fn_Float = 57,
    __fn_Int = 58,
    __fn_UInt = 59,
    __fn_Bitfield = 60,
    __fn_Int2 = 61,
    __fn_Int3 = 62,
    __fn_Int4 = 63,
    __fn_UInt2 = 64,
    __fn_UInt3 = 65,
    __fn_UInt4 = 66,
    __fn_Float2 = 67,
    __fn_Float3 = 68,
    __fn_Float4 = 69,
    __fn_Range = 70,
    __fn_URange = 71,
    __fn_Range64 = 72,
    __fn_URange64 = 73,
    __fn_WalkBlock = 74,
    __fn_WalkFunction = 75,
    __fn_WalkEnumeration = 76,
    __fn_WalkEnumeration8 = 77,
    __fn_WalkEnumeration16 = 78,
    __fn_FakeContext = 79,
    __fn_InvalidData = 80,
  };
protected:
  int _das_class_method_offset[81];
public:
  DapiDataWalker_Adapter ( const StructInfo * info ) {
      _das_class_method_offset[__fn_canVisitHandle] = adapt_field_offset("canVisitHandle",info);
      _das_class_method_offset[__fn_canVisitStructure] = adapt_field_offset("canVisitStructure",info);
      _das_class_method_offset[__fn_canVisitArray] = adapt_field_offset("canVisitArray",info);
      _das_class_method_offset[__fn_canVisitArrayData] = adapt_field_offset("canVisitArrayData",info);
      _das_class_method_offset[__fn_canVisitTuple] = adapt_field_offset("canVisitTuple",info);
      _das_class_method_offset[__fn_canVisitVariant] = adapt_field_offset("canVisitVariant",info);
      _das_class_method_offset[__fn_canVisitTable] = adapt_field_offset("canVisitTable",info);
      _das_class_method_offset[__fn_canVisitTableData] = adapt_field_offset("canVisitTableData",info);
      _das_class_method_offset[__fn_canVisitPointer] = adapt_field_offset("canVisitPointer",info);
      _das_class_method_offset[__fn_canVisitLambda] = adapt_field_offset("canVisitLambda",info);
      _das_class_method_offset[__fn_canVisitIterator] = adapt_field_offset("canVisitIterator",info);
      _das_class_method_offset[__fn_beforeStructure] = adapt_field_offset("beforeStructure",info);
      _das_class_method_offset[__fn_afterStructure] = adapt_field_offset("afterStructure",info);
      _das_class_method_offset[__fn_afterStructureCancel] = adapt_field_offset("afterStructureCancel",info);
      _das_class_method_offset[__fn_beforeStructureField] = adapt_field_offset("beforeStructureField",info);
      _das_class_method_offset[__fn_afterStructureField] = adapt_field_offset("afterStructureField",info);
      _das_class_method_offset[__fn_beforeTuple] = adapt_field_offset("beforeTuple",info);
      _das_class_method_offset[__fn_afterTuple] = adapt_field_offset("afterTuple",info);
      _das_class_method_offset[__fn_beforeTupleEntry] = adapt_field_offset("beforeTupleEntry",info);
      _das_class_method_offset[__fn_afterTupleEntry] = adapt_field_offset("afterTupleEntry",info);
      _das_class_method_offset[__fn_beforeVariant] = adapt_field_offset("beforeVariant",info);
      _das_class_method_offset[__fn_afterVariant] = adapt_field_offset("afterVariant",info);
      _das_class_method_offset[__fn_beforeArrayData] = adapt_field_offset("beforeArrayData",info);
      _das_class_method_offset[__fn_afterArrayData] = adapt_field_offset("afterArrayData",info);
      _das_class_method_offset[__fn_beforeArrayElement] = adapt_field_offset("beforeArrayElement",info);
      _das_class_method_offset[__fn_afterArrayElement] = adapt_field_offset("afterArrayElement",info);
      _das_class_method_offset[__fn_beforeDim] = adapt_field_offset("beforeDim",info);
      _das_class_method_offset[__fn_afterDim] = adapt_field_offset("afterDim",info);
      _das_class_method_offset[__fn_beforeArray] = adapt_field_offset("beforeArray",info);
      _das_class_method_offset[__fn_afterArray] = adapt_field_offset("afterArray",info);
      _das_class_method_offset[__fn_beforeTable] = adapt_field_offset("beforeTable",info);
      _das_class_method_offset[__fn_beforeTableKey] = adapt_field_offset("beforeTableKey",info);
      _das_class_method_offset[__fn_afterTableKey] = adapt_field_offset("afterTableKey",info);
      _das_class_method_offset[__fn_beforeTableValue] = adapt_field_offset("beforeTableValue",info);
      _das_class_method_offset[__fn_afterTableValue] = adapt_field_offset("afterTableValue",info);
      _das_class_method_offset[__fn_afterTable] = adapt_field_offset("afterTable",info);
      _das_class_method_offset[__fn_beforeRef] = adapt_field_offset("beforeRef",info);
      _das_class_method_offset[__fn_afterRef] = adapt_field_offset("afterRef",info);
      _das_class_method_offset[__fn_beforePtr] = adapt_field_offset("beforePtr",info);
      _das_class_method_offset[__fn_afterPtr] = adapt_field_offset("afterPtr",info);
      _das_class_method_offset[__fn_beforeHandle] = adapt_field_offset("beforeHandle",info);
      _das_class_method_offset[__fn_afterHandle] = adapt_field_offset("afterHandle",info);
      _das_class_method_offset[__fn_beforeLambda] = adapt_field_offset("beforeLambda",info);
      _das_class_method_offset[__fn_afterLambda] = adapt_field_offset("afterLambda",info);
      _das_class_method_offset[__fn_beforeIterator] = adapt_field_offset("beforeIterator",info);
      _das_class_method_offset[__fn_afterIterator] = adapt_field_offset("afterIterator",info);
      _das_class_method_offset[__fn_Null] = adapt_field_offset("Null",info);
      _das_class_method_offset[__fn_VoidPtr] = adapt_field_offset("VoidPtr",info);
      _das_class_method_offset[__fn_Bool] = adapt_field_offset("Bool",info);
      _das_class_method_offset[__fn_Int8] = adapt_field_offset("Int8",info);
      _das_class_method_offset[__fn_UInt8] = adapt_field_offset("UInt8",info);
      _das_class_method_offset[__fn_Int16] = adapt_field_offset("Int16",info);
      _das_class_method_offset[__fn_UInt16] = adapt_field_offset("UInt16",info);
      _das_class_method_offset[__fn_Int64] = adapt_field_offset("Int64",info);
      _das_class_method_offset[__fn_UInt64] = adapt_field_offset("UInt64",info);
      _das_class_method_offset[__fn_String] = adapt_field_offset("String",info);
      _das_class_method_offset[__fn_Double] = adapt_field_offset("Double",info);
      _das_class_method_offset[__fn_Float] = adapt_field_offset("Float",info);
      _das_class_method_offset[__fn_Int] = adapt_field_offset("Int",info);
      _das_class_method_offset[__fn_UInt] = adapt_field_offset("UInt",info);
      _das_class_method_offset[__fn_Bitfield] = adapt_field_offset("Bitfield",info);
      _das_class_method_offset[__fn_Int2] = adapt_field_offset("Int2",info);
      _das_class_method_offset[__fn_Int3] = adapt_field_offset("Int3",info);
      _das_class_method_offset[__fn_Int4] = adapt_field_offset("Int4",info);
      _das_class_method_offset[__fn_UInt2] = adapt_field_offset("UInt2",info);
      _das_class_method_offset[__fn_UInt3] = adapt_field_offset("UInt3",info);
      _das_class_method_offset[__fn_UInt4] = adapt_field_offset("UInt4",info);
      _das_class_method_offset[__fn_Float2] = adapt_field_offset("Float2",info);
      _das_class_method_offset[__fn_Float3] = adapt_field_offset("Float3",info);
      _das_class_method_offset[__fn_Float4] = adapt_field_offset("Float4",info);
      _das_class_method_offset[__fn_Range] = adapt_field_offset("Range",info);
      _das_class_method_offset[__fn_URange] = adapt_field_offset("URange",info);
      _das_class_method_offset[__fn_Range64] = adapt_field_offset("Range64",info);
      _das_class_method_offset[__fn_URange64] = adapt_field_offset("URange64",info);
      _das_class_method_offset[__fn_WalkBlock] = adapt_field_offset("WalkBlock",info);
      _das_class_method_offset[__fn_WalkFunction] = adapt_field_offset("WalkFunction",info);
      _das_class_method_offset[__fn_WalkEnumeration] = adapt_field_offset("WalkEnumeration",info);
      _das_class_method_offset[__fn_WalkEnumeration8] = adapt_field_offset("WalkEnumeration8",info);
      _das_class_method_offset[__fn_WalkEnumeration16] = adapt_field_offset("WalkEnumeration16",info);
      _das_class_method_offset[__fn_FakeContext] = adapt_field_offset("FakeContext",info);
      _das_class_method_offset[__fn_InvalidData] = adapt_field_offset("InvalidData",info);
  }
  __forceinline Func get_canVisitHandle ( void * self ) const {
    return getDasClassMethod(self,_das_class_method_offset[__fn_canVisitHandle]);
  }
  __forceinline bool invoke_canVisitHandle ( Context * __context__, Func __funcCall__, void * self, void * const  ps, TypeInfo const  & ti ) const {
    return das_invoke_function<bool>::invoke
      <void *,void * const ,TypeInfo const  &>
        (__context__,nullptr,__funcCall__,
          self,ps,ti);
  }
  __forceinline Func get_canVisitStructure ( void * self ) const {
    return getDasClassMethod(self,_das_class_method_offset[__fn_canVisitStructure]);
  }
  __forceinline bool invoke_canVisitStructure ( Context * __context__, Func __funcCall__, void * self, void * const  ps, StructInfo const  & si ) const {
    return das_invoke_function<bool>::invoke
      <void *,void * const ,StructInfo const  &>
        (__context__,nullptr,__funcCall__,
          self,ps,si);
  }
  __forceinline Func get_canVisitArray ( void * self ) const {
    return getDasClassMethod(self,_das_class_method_offset[__fn_canVisitArray]);
  }
  __forceinline bool invoke_canVisitArray ( Context * __context__, Func __funcCall__, void * self, void * const  pa, TypeInfo const  & ti ) const {
    return das_invoke_function<bool>::invoke
      <void *,void * const ,TypeInfo const  &>
        (__context__,nullptr,__funcCall__,
          self,pa,ti);
  }
  __forceinline Func get_canVisitArrayData ( void * self ) const {
    return getDasClassMethod(self,_das_class_method_offset[__fn_canVisitArrayData]);
  }
  __forceinline bool invoke_canVisitArrayData ( Context * __context__, Func __funcCall__, void * self, TypeInfo const  & ti, uint32_t count ) const {
    return das_invoke_function<bool>::invoke
      <void *,TypeInfo const  &,uint32_t>
        (__context__,nullptr,__funcCall__,
          self,ti,count);
  }
  __forceinline Func get_canVisitTuple ( void * self ) const {
    return getDasClassMethod(self,_das_class_method_offset[__fn_canVisitTuple]);
  }
  __forceinline bool invoke_canVisitTuple ( Context * __context__, Func __funcCall__, void * self, void * const  ps, TypeInfo const  & ti ) const {
    return das_invoke_function<bool>::invoke
      <void *,void * const ,TypeInfo const  &>
        (__context__,nullptr,__funcCall__,
          self,ps,ti);
  }
  __forceinline Func get_canVisitVariant ( void * self ) const {
    return getDasClassMethod(self,_das_class_method_offset[__fn_canVisitVariant]);
  }
  __forceinline bool invoke_canVisitVariant ( Context * __context__, Func __funcCall__, void * self, void * const  ps, TypeInfo const  & ti ) const {
    return das_invoke_function<bool>::invoke
      <void *,void * const ,TypeInfo const  &>
        (__context__,nullptr,__funcCall__,
          self,ps,ti);
  }
  __forceinline Func get_canVisitTable ( void * self ) const {
    return getDasClassMethod(self,_das_class_method_offset[__fn_canVisitTable]);
  }
  __forceinline bool invoke_canVisitTable ( Context * __context__, Func __funcCall__, void * self, void * const  ps, TypeInfo const  & ti ) const {
    return das_invoke_function<bool>::invoke
      <void *,void * const ,TypeInfo const  &>
        (__context__,nullptr,__funcCall__,
          self,ps,ti);
  }
  __forceinline Func get_canVisitTableData ( void * self ) const {
    return getDasClassMethod(self,_das_class_method_offset[__fn_canVisitTableData]);
  }
  __forceinline bool invoke_canVisitTableData ( Context * __context__, Func __funcCall__, void * self, TypeInfo const  & ti ) const {
    return das_invoke_function<bool>::invoke
      <void *,TypeInfo const  &>
        (__context__,nullptr,__funcCall__,
          self,ti);
  }
  __forceinline Func get_canVisitPointer ( void * self ) const {
    return getDasClassMethod(self,_das_class_method_offset[__fn_canVisitPointer]);
  }
  __forceinline bool invoke_canVisitPointer ( Context * __context__, Func __funcCall__, void * self, TypeInfo const  & ti ) const {
    return das_invoke_function<bool>::invoke
      <void *,TypeInfo const  &>
        (__context__,nullptr,__funcCall__,
          self,ti);
  }
  __forceinline Func get_canVisitLambda ( void * self ) const {
    return getDasClassMethod(self,_das_class_method_offset[__fn_canVisitLambda]);
  }
  __forceinline bool invoke_canVisitLambda ( Context * __context__, Func __funcCall__, void * self, TypeInfo const  & ti ) const {
    return das_invoke_function<bool>::invoke
      <void *,TypeInfo const  &>
        (__context__,nullptr,__funcCall__,
          self,ti);
  }
  __forceinline Func get_canVisitIterator ( void * self ) const {
    return getDasClassMethod(self,_das_class_method_offset[__fn_canVisitIterator]);
  }
  __forceinline bool invoke_canVisitIterator ( Context * __context__, Func __funcCall__, void * self, TypeInfo const  & ti ) const {
    return das_invoke_function<bool>::invoke
      <void *,TypeInfo const  &>
        (__context__,nullptr,__funcCall__,
          self,ti);
  }
  __forceinline Func get_beforeStructure ( void * self ) const {
    return getDasClassMethod(self,_das_class_method_offset[__fn_beforeStructure]);
  }
  __forceinline void invoke_beforeStructure ( Context * __context__, Func __funcCall__, void * self, void * const  ps, StructInfo const  & si ) const {
    das_invoke_function<void>::invoke
      <void *,void * const ,StructInfo const  &>
        (__context__,nullptr,__funcCall__,
          self,ps,si);
  }
  __forceinline Func get_afterStructure ( void * self ) const {
    return getDasClassMethod(self,_das_class_method_offset[__fn_afterStructure]);
  }
  __forceinline void invoke_afterStructure ( Context * __context__, Func __funcCall__, void * self, void * const  ps, StructInfo const  & si ) const {
    das_invoke_function<void>::invoke
      <void *,void * const ,StructInfo const  &>
        (__context__,nullptr,__funcCall__,
          self,ps,si);
  }
  __forceinline Func get_afterStructureCancel ( void * self ) const {
    return getDasClassMethod(self,_das_class_method_offset[__fn_afterStructureCancel]);
  }
  __forceinline void invoke_afterStructureCancel ( Context * __context__, Func __funcCall__, void * self, void * const  ps, StructInfo const  & si ) const {
    das_invoke_function<void>::invoke
      <void *,void * const ,StructInfo const  &>
        (__context__,nullptr,__funcCall__,
          self,ps,si);
  }
  __forceinline Func get_beforeStructureField ( void * self ) const {
    return getDasClassMethod(self,_das_class_method_offset[__fn_beforeStructureField]);
  }
  __forceinline void invoke_beforeStructureField ( Context * __context__, Func __funcCall__, void * self, void * const  ps, StructInfo const  & si, void * const  pv, VarInfo const  & vi, bool last ) const {
    das_invoke_function<void>::invoke
      <void *,void * const ,StructInfo const  &,void * const ,VarInfo const  &,bool>
        (__context__,nullptr,__funcCall__,
          self,ps,si,pv,vi,last);
  }
  __forceinline Func get_afterStructureField ( void * self ) const {
    return getDasClassMethod(self,_das_class_method_offset[__fn_afterStructureField]);
  }
  __forceinline void invoke_afterStructureField ( Context * __context__, Func __funcCall__, void * self, void * const  ps, StructInfo const  & si, void * const  pv, VarInfo const  & vi, bool last ) const {
    das_invoke_function<void>::invoke
      <void *,void * const ,StructInfo const  &,void * const ,VarInfo const  &,bool>
        (__context__,nullptr,__funcCall__,
          self,ps,si,pv,vi,last);
  }
  __forceinline Func get_beforeTuple ( void * self ) const {
    return getDasClassMethod(self,_das_class_method_offset[__fn_beforeTuple]);
  }
  __forceinline void invoke_beforeTuple ( Context * __context__, Func __funcCall__, void * self, void * const  ps, TypeInfo const  & ti ) const {
    das_invoke_function<void>::invoke
      <void *,void * const ,TypeInfo const  &>
        (__context__,nullptr,__funcCall__,
          self,ps,ti);
  }
  __forceinline Func get_afterTuple ( void * self ) const {
    return getDasClassMethod(self,_das_class_method_offset[__fn_afterTuple]);
  }
  __forceinline void invoke_afterTuple ( Context * __context__, Func __funcCall__, void * self, void * const  ps, TypeInfo const  & ti ) const {
    das_invoke_function<void>::invoke
      <void *,void * const ,TypeInfo const  &>
        (__context__,nullptr,__funcCall__,
          self,ps,ti);
  }
  __forceinline Func get_beforeTupleEntry ( void * self ) const {
    return getDasClassMethod(self,_das_class_method_offset[__fn_beforeTupleEntry]);
  }
  __forceinline void invoke_beforeTupleEntry ( Context * __context__, Func __funcCall__, void * self, void * const  ps, TypeInfo const  & ti, void * const  pv, TypeInfo const  & vi, bool last ) const {
    das_invoke_function<void>::invoke
      <void *,void * const ,TypeInfo const  &,void * const ,TypeInfo const  &,bool>
        (__context__,nullptr,__funcCall__,
          self,ps,ti,pv,vi,last);
  }
  __forceinline Func get_afterTupleEntry ( void * self ) const {
    return getDasClassMethod(self,_das_class_method_offset[__fn_afterTupleEntry]);
  }
  __forceinline void invoke_afterTupleEntry ( Context * __context__, Func __funcCall__, void * self, void * const  ps, TypeInfo const  & ti, void * const  pv, TypeInfo const  & vi, bool last ) const {
    das_invoke_function<void>::invoke
      <void *,void * const ,TypeInfo const  &,void * const ,TypeInfo const  &,bool>
        (__context__,nullptr,__funcCall__,
          self,ps,ti,pv,vi,last);
  }
  __forceinline Func get_beforeVariant ( void * self ) const {
    return getDasClassMethod(self,_das_class_method_offset[__fn_beforeVariant]);
  }
  __forceinline void invoke_beforeVariant ( Context * __context__, Func __funcCall__, void * self, void * const  ps, TypeInfo const  & ti ) const {
    das_invoke_function<void>::invoke
      <void *,void * const ,TypeInfo const  &>
        (__context__,nullptr,__funcCall__,
          self,ps,ti);
  }
  __forceinline Func get_afterVariant ( void * self ) const {
    return getDasClassMethod(self,_das_class_method_offset[__fn_afterVariant]);
  }
  __forceinline void invoke_afterVariant ( Context * __context__, Func __funcCall__, void * self, void * const  ps, TypeInfo const  & ti ) const {
    das_invoke_function<void>::invoke
      <void *,void * const ,TypeInfo const  &>
        (__context__,nullptr,__funcCall__,
          self,ps,ti);
  }
  __forceinline Func get_beforeArrayData ( void * self ) const {
    return getDasClassMethod(self,_das_class_method_offset[__fn_beforeArrayData]);
  }
  __forceinline void invoke_beforeArrayData ( Context * __context__, Func __funcCall__, void * self, void * const  ps, uint32_t stride, uint32_t count, TypeInfo const  & ti ) const {
    das_invoke_function<void>::invoke
      <void *,void * const ,uint32_t,uint32_t,TypeInfo const  &>
        (__context__,nullptr,__funcCall__,
          self,ps,stride,count,ti);
  }
  __forceinline Func get_afterArrayData ( void * self ) const {
    return getDasClassMethod(self,_das_class_method_offset[__fn_afterArrayData]);
  }
  __forceinline void invoke_afterArrayData ( Context * __context__, Func __funcCall__, void * self, void * const  ps, uint32_t stride, uint32_t count, TypeInfo const  & ti ) const {
    das_invoke_function<void>::invoke
      <void *,void * const ,uint32_t,uint32_t,TypeInfo const  &>
        (__context__,nullptr,__funcCall__,
          self,ps,stride,count,ti);
  }
  __forceinline Func get_beforeArrayElement ( void * self ) const {
    return getDasClassMethod(self,_das_class_method_offset[__fn_beforeArrayElement]);
  }
  __forceinline void invoke_beforeArrayElement ( Context * __context__, Func __funcCall__, void * self, void * const  ps, TypeInfo const  & ti, void * const  pe, uint32_t index, bool last ) const {
    das_invoke_function<void>::invoke
      <void *,void * const ,TypeInfo const  &,void * const ,uint32_t,bool>
        (__context__,nullptr,__funcCall__,
          self,ps,ti,pe,index,last);
  }
  __forceinline Func get_afterArrayElement ( void * self ) const {
    return getDasClassMethod(self,_das_class_method_offset[__fn_afterArrayElement]);
  }
  __forceinline void invoke_afterArrayElement ( Context * __context__, Func __funcCall__, void * self, void * const  ps, TypeInfo const  & ti, void * const  pe, uint32_t index, bool last ) const {
    das_invoke_function<void>::invoke
      <void *,void * const ,TypeInfo const  &,void * const ,uint32_t,bool>
        (__context__,nullptr,__funcCall__,
          self,ps,ti,pe,index,last);
  }
  __forceinline Func get_beforeDim ( void * self ) const {
    return getDasClassMethod(self,_das_class_method_offset[__fn_beforeDim]);
  }
  __forceinline void invoke_beforeDim ( Context * __context__, Func __funcCall__, void * self, void * const  ps, TypeInfo const  & ti ) const {
    das_invoke_function<void>::invoke
      <void *,void * const ,TypeInfo const  &>
        (__context__,nullptr,__funcCall__,
          self,ps,ti);
  }
  __forceinline Func get_afterDim ( void * self ) const {
    return getDasClassMethod(self,_das_class_method_offset[__fn_afterDim]);
  }
  __forceinline void invoke_afterDim ( Context * __context__, Func __funcCall__, void * self, void * const  ps, TypeInfo const  & ti ) const {
    das_invoke_function<void>::invoke
      <void *,void * const ,TypeInfo const  &>
        (__context__,nullptr,__funcCall__,
          self,ps,ti);
  }
  __forceinline Func get_beforeArray ( void * self ) const {
    return getDasClassMethod(self,_das_class_method_offset[__fn_beforeArray]);
  }
  __forceinline void invoke_beforeArray ( Context * __context__, Func __funcCall__, void * self, debugapi::DapiArray const  & pa, TypeInfo const  & ti ) const {
    das_invoke_function<void>::invoke
      <void *,debugapi::DapiArray const  &,TypeInfo const  &>
        (__context__,nullptr,__funcCall__,
          self,pa,ti);
  }
  __forceinline Func get_afterArray ( void * self ) const {
    return getDasClassMethod(self,_das_class_method_offset[__fn_afterArray]);
  }
  __forceinline void invoke_afterArray ( Context * __context__, Func __funcCall__, void * self, debugapi::DapiArray const  & pa, TypeInfo const  & ti ) const {
    das_invoke_function<void>::invoke
      <void *,debugapi::DapiArray const  &,TypeInfo const  &>
        (__context__,nullptr,__funcCall__,
          self,pa,ti);
  }
  __forceinline Func get_beforeTable ( void * self ) const {
    return getDasClassMethod(self,_das_class_method_offset[__fn_beforeTable]);
  }
  __forceinline void invoke_beforeTable ( Context * __context__, Func __funcCall__, void * self, debugapi::DapiTable const  & pa, TypeInfo const  & ti ) const {
    das_invoke_function<void>::invoke
      <void *,debugapi::DapiTable const  &,TypeInfo const  &>
        (__context__,nullptr,__funcCall__,
          self,pa,ti);
  }
  __forceinline Func get_beforeTableKey ( void * self ) const {
    return getDasClassMethod(self,_das_class_method_offset[__fn_beforeTableKey]);
  }
  __forceinline void invoke_beforeTableKey ( Context * __context__, Func __funcCall__, void * self, debugapi::DapiTable const  & pa, TypeInfo const  & ti, void * const  pk, TypeInfo const  & ki, uint32_t index, bool last ) const {
    das_invoke_function<void>::invoke
      <void *,debugapi::DapiTable const  &,TypeInfo const  &,void * const ,TypeInfo const  &,uint32_t,bool>
        (__context__,nullptr,__funcCall__,
          self,pa,ti,pk,ki,index,last);
  }
  __forceinline Func get_afterTableKey ( void * self ) const {
    return getDasClassMethod(self,_das_class_method_offset[__fn_afterTableKey]);
  }
  __forceinline void invoke_afterTableKey ( Context * __context__, Func __funcCall__, void * self, debugapi::DapiTable const  & pa, TypeInfo const  & ti, void * const  pk, TypeInfo const  & ki, uint32_t index, bool last ) const {
    das_invoke_function<void>::invoke
      <void *,debugapi::DapiTable const  &,TypeInfo const  &,void * const ,TypeInfo const  &,uint32_t,bool>
        (__context__,nullptr,__funcCall__,
          self,pa,ti,pk,ki,index,last);
  }
  __forceinline Func get_beforeTableValue ( void * self ) const {
    return getDasClassMethod(self,_das_class_method_offset[__fn_beforeTableValue]);
  }
  __forceinline void invoke_beforeTableValue ( Context * __context__, Func __funcCall__, void * self, debugapi::DapiTable const  & pa, TypeInfo const  & ti, void * const  pv, TypeInfo const  & kv, uint32_t index, bool last ) const {
    das_invoke_function<void>::invoke
      <void *,debugapi::DapiTable const  &,TypeInfo const  &,void * const ,TypeInfo const  &,uint32_t,bool>
        (__context__,nullptr,__funcCall__,
          self,pa,ti,pv,kv,index,last);
  }
  __forceinline Func get_afterTableValue ( void * self ) const {
    return getDasClassMethod(self,_das_class_method_offset[__fn_afterTableValue]);
  }
  __forceinline void invoke_afterTableValue ( Context * __context__, Func __funcCall__, void * self, debugapi::DapiTable const  & pa, TypeInfo const  & ti, void * const  pv, TypeInfo const  & kv, uint32_t index, bool last ) const {
    das_invoke_function<void>::invoke
      <void *,debugapi::DapiTable const  &,TypeInfo const  &,void * const ,TypeInfo const  &,uint32_t,bool>
        (__context__,nullptr,__funcCall__,
          self,pa,ti,pv,kv,index,last);
  }
  __forceinline Func get_afterTable ( void * self ) const {
    return getDasClassMethod(self,_das_class_method_offset[__fn_afterTable]);
  }
  __forceinline void invoke_afterTable ( Context * __context__, Func __funcCall__, void * self, debugapi::DapiTable const  & pa, TypeInfo const  & ti ) const {
    das_invoke_function<void>::invoke
      <void *,debugapi::DapiTable const  &,TypeInfo const  &>
        (__context__,nullptr,__funcCall__,
          self,pa,ti);
  }
  __forceinline Func get_beforeRef ( void * self ) const {
    return getDasClassMethod(self,_das_class_method_offset[__fn_beforeRef]);
  }
  __forceinline void invoke_beforeRef ( Context * __context__, Func __funcCall__, void * self, void * const  ps, TypeInfo const  & ti ) const {
    das_invoke_function<void>::invoke
      <void *,void * const ,TypeInfo const  &>
        (__context__,nullptr,__funcCall__,
          self,ps,ti);
  }
  __forceinline Func get_afterRef ( void * self ) const {
    return getDasClassMethod(self,_das_class_method_offset[__fn_afterRef]);
  }
  __forceinline void invoke_afterRef ( Context * __context__, Func __funcCall__, void * self, void * const  ps, TypeInfo const  & ti ) const {
    das_invoke_function<void>::invoke
      <void *,void * const ,TypeInfo const  &>
        (__context__,nullptr,__funcCall__,
          self,ps,ti);
  }
  __forceinline Func get_beforePtr ( void * self ) const {
    return getDasClassMethod(self,_das_class_method_offset[__fn_beforePtr]);
  }
  __forceinline void invoke_beforePtr ( Context * __context__, Func __funcCall__, void * self, void * const  ps, TypeInfo const  & ti ) const {
    das_invoke_function<void>::invoke
      <void *,void * const ,TypeInfo const  &>
        (__context__,nullptr,__funcCall__,
          self,ps,ti);
  }
  __forceinline Func get_afterPtr ( void * self ) const {
    return getDasClassMethod(self,_das_class_method_offset[__fn_afterPtr]);
  }
  __forceinline void invoke_afterPtr ( Context * __context__, Func __funcCall__, void * self, void * const  ps, TypeInfo const  & ti ) const {
    das_invoke_function<void>::invoke
      <void *,void * const ,TypeInfo const  &>
        (__context__,nullptr,__funcCall__,
          self,ps,ti);
  }
  __forceinline Func get_beforeHandle ( void * self ) const {
    return getDasClassMethod(self,_das_class_method_offset[__fn_beforeHandle]);
  }
  __forceinline void invoke_beforeHandle ( Context * __context__, Func __funcCall__, void * self, void * const  ps, TypeInfo const  & ti ) const {
    das_invoke_function<void>::invoke
      <void *,void * const ,TypeInfo const  &>
        (__context__,nullptr,__funcCall__,
          self,ps,ti);
  }
  __forceinline Func get_afterHandle ( void * self ) const {
    return getDasClassMethod(self,_das_class_method_offset[__fn_afterHandle]);
  }
  __forceinline void invoke_afterHandle ( Context * __context__, Func __funcCall__, void * self, void * const  ps, TypeInfo const  & ti ) const {
    das_invoke_function<void>::invoke
      <void *,void * const ,TypeInfo const  &>
        (__context__,nullptr,__funcCall__,
          self,ps,ti);
  }
  __forceinline Func get_beforeLambda ( void * self ) const {
    return getDasClassMethod(self,_das_class_method_offset[__fn_beforeLambda]);
  }
  __forceinline void invoke_beforeLambda ( Context * __context__, Func __funcCall__, void * self, debugapi::DapiLambda & value, TypeInfo const  & ti ) const {
    das_invoke_function<void>::invoke
      <void *,debugapi::DapiLambda &,TypeInfo const  &>
        (__context__,nullptr,__funcCall__,
          self,value,ti);
  }
  __forceinline Func get_afterLambda ( void * self ) const {
    return getDasClassMethod(self,_das_class_method_offset[__fn_afterLambda]);
  }
  __forceinline void invoke_afterLambda ( Context * __context__, Func __funcCall__, void * self, debugapi::DapiLambda & value, TypeInfo const  & ti ) const {
    das_invoke_function<void>::invoke
      <void *,debugapi::DapiLambda &,TypeInfo const  &>
        (__context__,nullptr,__funcCall__,
          self,value,ti);
  }
  __forceinline Func get_beforeIterator ( void * self ) const {
    return getDasClassMethod(self,_das_class_method_offset[__fn_beforeIterator]);
  }
  __forceinline void invoke_beforeIterator ( Context * __context__, Func __funcCall__, void * self, debugapi::DapiSequence & value, TypeInfo const  & ti ) const {
    das_invoke_function<void>::invoke
      <void *,debugapi::DapiSequence &,TypeInfo const  &>
        (__context__,nullptr,__funcCall__,
          self,value,ti);
  }
  __forceinline Func get_afterIterator ( void * self ) const {
    return getDasClassMethod(self,_das_class_method_offset[__fn_afterIterator]);
  }
  __forceinline void invoke_afterIterator ( Context * __context__, Func __funcCall__, void * self, debugapi::DapiSequence & value, TypeInfo const  & ti ) const {
    das_invoke_function<void>::invoke
      <void *,debugapi::DapiSequence &,TypeInfo const  &>
        (__context__,nullptr,__funcCall__,
          self,value,ti);
  }
  __forceinline Func get_Null ( void * self ) const {
    return getDasClassMethod(self,_das_class_method_offset[__fn_Null]);
  }
  __forceinline void invoke_Null ( Context * __context__, Func __funcCall__, void * self, TypeInfo const  & ti ) const {
    das_invoke_function<void>::invoke
      <void *,TypeInfo const  &>
        (__context__,nullptr,__funcCall__,
          self,ti);
  }
  __forceinline Func get_VoidPtr ( void * self ) const {
    return getDasClassMethod(self,_das_class_method_offset[__fn_VoidPtr]);
  }
  __forceinline void invoke_VoidPtr ( Context * __context__, Func __funcCall__, void * self, void * & value ) const {
    das_invoke_function<void>::invoke
      <void *,void * &>
        (__context__,nullptr,__funcCall__,
          self,value);
  }
  __forceinline Func get_Bool ( void * self ) const {
    return getDasClassMethod(self,_das_class_method_offset[__fn_Bool]);
  }
  __forceinline void invoke_Bool ( Context * __context__, Func __funcCall__, void * self, bool & value ) const {
    das_invoke_function<void>::invoke
      <void *,bool &>
        (__context__,nullptr,__funcCall__,
          self,value);
  }
  __forceinline Func get_Int8 ( void * self ) const {
    return getDasClassMethod(self,_das_class_method_offset[__fn_Int8]);
  }
  __forceinline void invoke_Int8 ( Context * __context__, Func __funcCall__, void * self, int8_t & value ) const {
    das_invoke_function<void>::invoke
      <void *,int8_t &>
        (__context__,nullptr,__funcCall__,
          self,value);
  }
  __forceinline Func get_UInt8 ( void * self ) const {
    return getDasClassMethod(self,_das_class_method_offset[__fn_UInt8]);
  }
  __forceinline void invoke_UInt8 ( Context * __context__, Func __funcCall__, void * self, uint8_t & value ) const {
    das_invoke_function<void>::invoke
      <void *,uint8_t &>
        (__context__,nullptr,__funcCall__,
          self,value);
  }
  __forceinline Func get_Int16 ( void * self ) const {
    return getDasClassMethod(self,_das_class_method_offset[__fn_Int16]);
  }
  __forceinline void invoke_Int16 ( Context * __context__, Func __funcCall__, void * self, int16_t & value ) const {
    das_invoke_function<void>::invoke
      <void *,int16_t &>
        (__context__,nullptr,__funcCall__,
          self,value);
  }
  __forceinline Func get_UInt16 ( void * self ) const {
    return getDasClassMethod(self,_das_class_method_offset[__fn_UInt16]);
  }
  __forceinline void invoke_UInt16 ( Context * __context__, Func __funcCall__, void * self, uint16_t & value ) const {
    das_invoke_function<void>::invoke
      <void *,uint16_t &>
        (__context__,nullptr,__funcCall__,
          self,value);
  }
  __forceinline Func get_Int64 ( void * self ) const {
    return getDasClassMethod(self,_das_class_method_offset[__fn_Int64]);
  }
  __forceinline void invoke_Int64 ( Context * __context__, Func __funcCall__, void * self, int64_t & value ) const {
    das_invoke_function<void>::invoke
      <void *,int64_t &>
        (__context__,nullptr,__funcCall__,
          self,value);
  }
  __forceinline Func get_UInt64 ( void * self ) const {
    return getDasClassMethod(self,_das_class_method_offset[__fn_UInt64]);
  }
  __forceinline void invoke_UInt64 ( Context * __context__, Func __funcCall__, void * self, uint64_t & value ) const {
    das_invoke_function<void>::invoke
      <void *,uint64_t &>
        (__context__,nullptr,__funcCall__,
          self,value);
  }
  __forceinline Func get_String ( void * self ) const {
    return getDasClassMethod(self,_das_class_method_offset[__fn_String]);
  }
  __forceinline void invoke_String ( Context * __context__, Func __funcCall__, void * self, char * & value ) const {
    das_invoke_function<void>::invoke
      <void *,char * &>
        (__context__,nullptr,__funcCall__,
          self,value);
  }
  __forceinline Func get_Double ( void * self ) const {
    return getDasClassMethod(self,_das_class_method_offset[__fn_Double]);
  }
  __forceinline void invoke_Double ( Context * __context__, Func __funcCall__, void * self, double & value ) const {
    das_invoke_function<void>::invoke
      <void *,double &>
        (__context__,nullptr,__funcCall__,
          self,value);
  }
  __forceinline Func get_Float ( void * self ) const {
    return getDasClassMethod(self,_das_class_method_offset[__fn_Float]);
  }
  __forceinline void invoke_Float ( Context * __context__, Func __funcCall__, void * self, float & value ) const {
    das_invoke_function<void>::invoke
      <void *,float &>
        (__context__,nullptr,__funcCall__,
          self,value);
  }
  __forceinline Func get_Int ( void * self ) const {
    return getDasClassMethod(self,_das_class_method_offset[__fn_Int]);
  }
  __forceinline void invoke_Int ( Context * __context__, Func __funcCall__, void * self, int32_t & value ) const {
    das_invoke_function<void>::invoke
      <void *,int32_t &>
        (__context__,nullptr,__funcCall__,
          self,value);
  }
  __forceinline Func get_UInt ( void * self ) const {
    return getDasClassMethod(self,_das_class_method_offset[__fn_UInt]);
  }
  __forceinline void invoke_UInt ( Context * __context__, Func __funcCall__, void * self, uint32_t & value ) const {
    das_invoke_function<void>::invoke
      <void *,uint32_t &>
        (__context__,nullptr,__funcCall__,
          self,value);
  }
  __forceinline Func get_Bitfield ( void * self ) const {
    return getDasClassMethod(self,_das_class_method_offset[__fn_Bitfield]);
  }
  __forceinline void invoke_Bitfield ( Context * __context__, Func __funcCall__, void * self, uint32_t & value, TypeInfo const  & ti ) const {
    das_invoke_function<void>::invoke
      <void *,uint32_t &,TypeInfo const  &>
        (__context__,nullptr,__funcCall__,
          self,value,ti);
  }
  __forceinline Func get_Int2 ( void * self ) const {
    return getDasClassMethod(self,_das_class_method_offset[__fn_Int2]);
  }
  __forceinline void invoke_Int2 ( Context * __context__, Func __funcCall__, void * self, int2 & value ) const {
    das_invoke_function<void>::invoke
      <void *,int2 &>
        (__context__,nullptr,__funcCall__,
          self,value);
  }
  __forceinline Func get_Int3 ( void * self ) const {
    return getDasClassMethod(self,_das_class_method_offset[__fn_Int3]);
  }
  __forceinline void invoke_Int3 ( Context * __context__, Func __funcCall__, void * self, int3 & value ) const {
    das_invoke_function<void>::invoke
      <void *,int3 &>
        (__context__,nullptr,__funcCall__,
          self,value);
  }
  __forceinline Func get_Int4 ( void * self ) const {
    return getDasClassMethod(self,_das_class_method_offset[__fn_Int4]);
  }
  __forceinline void invoke_Int4 ( Context * __context__, Func __funcCall__, void * self, int4 & value ) const {
    das_invoke_function<void>::invoke
      <void *,int4 &>
        (__context__,nullptr,__funcCall__,
          self,value);
  }
  __forceinline Func get_UInt2 ( void * self ) const {
    return getDasClassMethod(self,_das_class_method_offset[__fn_UInt2]);
  }
  __forceinline void invoke_UInt2 ( Context * __context__, Func __funcCall__, void * self, uint2 & value ) const {
    das_invoke_function<void>::invoke
      <void *,uint2 &>
        (__context__,nullptr,__funcCall__,
          self,value);
  }
  __forceinline Func get_UInt3 ( void * self ) const {
    return getDasClassMethod(self,_das_class_method_offset[__fn_UInt3]);
  }
  __forceinline void invoke_UInt3 ( Context * __context__, Func __funcCall__, void * self, uint3 & value ) const {
    das_invoke_function<void>::invoke
      <void *,uint3 &>
        (__context__,nullptr,__funcCall__,
          self,value);
  }
  __forceinline Func get_UInt4 ( void * self ) const {
    return getDasClassMethod(self,_das_class_method_offset[__fn_UInt4]);
  }
  __forceinline void invoke_UInt4 ( Context * __context__, Func __funcCall__, void * self, uint4 & value ) const {
    das_invoke_function<void>::invoke
      <void *,uint4 &>
        (__context__,nullptr,__funcCall__,
          self,value);
  }
  __forceinline Func get_Float2 ( void * self ) const {
    return getDasClassMethod(self,_das_class_method_offset[__fn_Float2]);
  }
  __forceinline void invoke_Float2 ( Context * __context__, Func __funcCall__, void * self, float2 & value ) const {
    das_invoke_function<void>::invoke
      <void *,float2 &>
        (__context__,nullptr,__funcCall__,
          self,value);
  }
  __forceinline Func get_Float3 ( void * self ) const {
    return getDasClassMethod(self,_das_class_method_offset[__fn_Float3]);
  }
  __forceinline void invoke_Float3 ( Context * __context__, Func __funcCall__, void * self, float3 & value ) const {
    das_invoke_function<void>::invoke
      <void *,float3 &>
        (__context__,nullptr,__funcCall__,
          self,value);
  }
  __forceinline Func get_Float4 ( void * self ) const {
    return getDasClassMethod(self,_das_class_method_offset[__fn_Float4]);
  }
  __forceinline void invoke_Float4 ( Context * __context__, Func __funcCall__, void * self, float4 & value ) const {
    das_invoke_function<void>::invoke
      <void *,float4 &>
        (__context__,nullptr,__funcCall__,
          self,value);
  }
  __forceinline Func get_Range ( void * self ) const {
    return getDasClassMethod(self,_das_class_method_offset[__fn_Range]);
  }
  __forceinline void invoke_Range ( Context * __context__, Func __funcCall__, void * self, range & value ) const {
    das_invoke_function<void>::invoke
      <void *,range &>
        (__context__,nullptr,__funcCall__,
          self,value);
  }
  __forceinline Func get_URange ( void * self ) const {
    return getDasClassMethod(self,_das_class_method_offset[__fn_URange]);
  }
  __forceinline void invoke_URange ( Context * __context__, Func __funcCall__, void * self, urange & value ) const {
    das_invoke_function<void>::invoke
      <void *,urange &>
        (__context__,nullptr,__funcCall__,
          self,value);
  }
  __forceinline Func get_Range64 ( void * self ) const {
    return getDasClassMethod(self,_das_class_method_offset[__fn_Range64]);
  }
  __forceinline void invoke_Range64 ( Context * __context__, Func __funcCall__, void * self, range64 & value ) const {
    das_invoke_function<void>::invoke
      <void *,range64 &>
        (__context__,nullptr,__funcCall__,
          self,value);
  }
  __forceinline Func get_URange64 ( void * self ) const {
    return getDasClassMethod(self,_das_class_method_offset[__fn_URange64]);
  }
  __forceinline void invoke_URange64 ( Context * __context__, Func __funcCall__, void * self, urange64 & value ) const {
    das_invoke_function<void>::invoke
      <void *,urange64 &>
        (__context__,nullptr,__funcCall__,
          self,value);
  }
  __forceinline Func get_WalkBlock ( void * self ) const {
    return getDasClassMethod(self,_das_class_method_offset[__fn_WalkBlock]);
  }
  __forceinline void invoke_WalkBlock ( Context * __context__, Func __funcCall__, void * self, debugapi::DapiBlock & value ) const {
    das_invoke_function<void>::invoke
      <void *,debugapi::DapiBlock &>
        (__context__,nullptr,__funcCall__,
          self,value);
  }
  __forceinline Func get_WalkFunction ( void * self ) const {
    return getDasClassMethod(self,_das_class_method_offset[__fn_WalkFunction]);
  }
  __forceinline void invoke_WalkFunction ( Context * __context__, Func __funcCall__, void * self, debugapi::DapiFunc & value ) const {
    das_invoke_function<void>::invoke
      <void *,debugapi::DapiFunc &>
        (__context__,nullptr,__funcCall__,
          self,value);
  }
  __forceinline Func get_WalkEnumeration ( void * self ) const {
    return getDasClassMethod(self,_das_class_method_offset[__fn_WalkEnumeration]);
  }
  __forceinline void invoke_WalkEnumeration ( Context * __context__, Func __funcCall__, void * self, int32_t & value, EnumInfo const  & ei ) const {
    das_invoke_function<void>::invoke
      <void *,int32_t &,EnumInfo const  &>
        (__context__,nullptr,__funcCall__,
          self,value,ei);
  }
  __forceinline Func get_WalkEnumeration8 ( void * self ) const {
    return getDasClassMethod(self,_das_class_method_offset[__fn_WalkEnumeration8]);
  }
  __forceinline void invoke_WalkEnumeration8 ( Context * __context__, Func __funcCall__, void * self, int8_t & value, EnumInfo const  & ei ) const {
    das_invoke_function<void>::invoke
      <void *,int8_t &,EnumInfo const  &>
        (__context__,nullptr,__funcCall__,
          self,value,ei);
  }
  __forceinline Func get_WalkEnumeration16 ( void * self ) const {
    return getDasClassMethod(self,_das_class_method_offset[__fn_WalkEnumeration16]);
  }
  __forceinline void invoke_WalkEnumeration16 ( Context * __context__, Func __funcCall__, void * self, int16_t & value, EnumInfo const  & ei ) const {
    das_invoke_function<void>::invoke
      <void *,int16_t &,EnumInfo const  &>
        (__context__,nullptr,__funcCall__,
          self,value,ei);
  }
  __forceinline Func get_FakeContext ( void * self ) const {
    return getDasClassMethod(self,_das_class_method_offset[__fn_FakeContext]);
  }
  __forceinline void invoke_FakeContext ( Context * __context__, Func __funcCall__, void * self, Context & value ) const {
    das_invoke_function<void>::invoke
      <void *,Context &>
        (__context__,nullptr,__funcCall__,
          self,value);
  }
  __forceinline Func get_InvalidData ( void * self ) const {
    return getDasClassMethod(self,_das_class_method_offset[__fn_InvalidData]);
  }
  __forceinline void invoke_InvalidData ( Context * __context__, Func __funcCall__, void * self ) const {
    das_invoke_function<void>::invoke
      <void *>
        (__context__,nullptr,__funcCall__,
          self);
  }
};

class DapiDebugAgent_Adapter {
protected:
  enum {
    __fn_onInstall = 0,
    __fn_onUninstall = 1,
    __fn_onCreateContext = 2,
    __fn_onDestroyContext = 3,
    __fn_onSimulateContext = 4,
    __fn_onSingleStep = 5,
    __fn_onInstrument = 6,
    __fn_onInstrumentFunction = 7,
    __fn_onBreakpoint = 8,
    __fn_onVariable = 9,
    __fn_onBreakpointsReset = 10,
    __fn_onTick = 11,
    __fn_onCollect = 12,
    __fn_onLog = 13,
  };
protected:
  int _das_class_method_offset[14];
public:
  DapiDebugAgent_Adapter ( const StructInfo * info ) {
      _das_class_method_offset[__fn_onInstall] = adapt_field_offset("onInstall",info);
      _das_class_method_offset[__fn_onUninstall] = adapt_field_offset("onUninstall",info);
      _das_class_method_offset[__fn_onCreateContext] = adapt_field_offset("onCreateContext",info);
      _das_class_method_offset[__fn_onDestroyContext] = adapt_field_offset("onDestroyContext",info);
      _das_class_method_offset[__fn_onSimulateContext] = adapt_field_offset("onSimulateContext",info);
      _das_class_method_offset[__fn_onSingleStep] = adapt_field_offset("onSingleStep",info);
      _das_class_method_offset[__fn_onInstrument] = adapt_field_offset("onInstrument",info);
      _das_class_method_offset[__fn_onInstrumentFunction] = adapt_field_offset("onInstrumentFunction",info);
      _das_class_method_offset[__fn_onBreakpoint] = adapt_field_offset("onBreakpoint",info);
      _das_class_method_offset[__fn_onVariable] = adapt_field_offset("onVariable",info);
      _das_class_method_offset[__fn_onBreakpointsReset] = adapt_field_offset("onBreakpointsReset",info);
      _das_class_method_offset[__fn_onTick] = adapt_field_offset("onTick",info);
      _das_class_method_offset[__fn_onCollect] = adapt_field_offset("onCollect",info);
      _das_class_method_offset[__fn_onLog] = adapt_field_offset("onLog",info);
  }
  __forceinline Func get_onInstall ( void * self ) const {
    return getDasClassMethod(self,_das_class_method_offset[__fn_onInstall]);
  }
  __forceinline void invoke_onInstall ( Context * __context__, Func __funcCall__, void * self, DebugAgent * const  agent ) const {
    das_invoke_function<void>::invoke
      <void *,DebugAgent * const >
        (__context__,nullptr,__funcCall__,
          self,agent);
  }
  __forceinline Func get_onUninstall ( void * self ) const {
    return getDasClassMethod(self,_das_class_method_offset[__fn_onUninstall]);
  }
  __forceinline void invoke_onUninstall ( Context * __context__, Func __funcCall__, void * self, DebugAgent * const  agent ) const {
    das_invoke_function<void>::invoke
      <void *,DebugAgent * const >
        (__context__,nullptr,__funcCall__,
          self,agent);
  }
  __forceinline Func get_onCreateContext ( void * self ) const {
    return getDasClassMethod(self,_das_class_method_offset[__fn_onCreateContext]);
  }
  __forceinline void invoke_onCreateContext ( Context * __context__, Func __funcCall__, void * self, Context & ctx ) const {
    das_invoke_function<void>::invoke
      <void *,Context &>
        (__context__,nullptr,__funcCall__,
          self,ctx);
  }
  __forceinline Func get_onDestroyContext ( void * self ) const {
    return getDasClassMethod(self,_das_class_method_offset[__fn_onDestroyContext]);
  }
  __forceinline void invoke_onDestroyContext ( Context * __context__, Func __funcCall__, void * self, Context & ctx ) const {
    das_invoke_function<void>::invoke
      <void *,Context &>
        (__context__,nullptr,__funcCall__,
          self,ctx);
  }
  __forceinline Func get_onSimulateContext ( void * self ) const {
    return getDasClassMethod(self,_das_class_method_offset[__fn_onSimulateContext]);
  }
  __forceinline void invoke_onSimulateContext ( Context * __context__, Func __funcCall__, void * self, Context & ctx ) const {
    das_invoke_function<void>::invoke
      <void *,Context &>
        (__context__,nullptr,__funcCall__,
          self,ctx);
  }
  __forceinline Func get_onSingleStep ( void * self ) const {
    return getDasClassMethod(self,_das_class_method_offset[__fn_onSingleStep]);
  }
  __forceinline void invoke_onSingleStep ( Context * __context__, Func __funcCall__, void * self, Context & ctx, LineInfo const  & at ) const {
    das_invoke_function<void>::invoke
      <void *,Context &,LineInfo const  &>
        (__context__,nullptr,__funcCall__,
          self,ctx,at);
  }
  __forceinline Func get_onInstrument ( void * self ) const {
    return getDasClassMethod(self,_das_class_method_offset[__fn_onInstrument]);
  }
  __forceinline void invoke_onInstrument ( Context * __context__, Func __funcCall__, void * self, Context & ctx, LineInfo const  & at ) const {
    das_invoke_function<void>::invoke
      <void *,Context &,LineInfo const  &>
        (__context__,nullptr,__funcCall__,
          self,ctx,at);
  }
  __forceinline Func get_onInstrumentFunction ( void * self ) const {
    return getDasClassMethod(self,_das_class_method_offset[__fn_onInstrumentFunction]);
  }
  __forceinline void invoke_onInstrumentFunction ( Context * __context__, Func __funcCall__, void * self, Context & ctx, SimFunction * const  fn, bool entering, uint64_t userData ) const {
    das_invoke_function<void>::invoke
      <void *,Context &,SimFunction * const ,bool,uint64_t>
        (__context__,nullptr,__funcCall__,
          self,ctx,fn,entering,userData);
  }
  __forceinline Func get_onBreakpoint ( void * self ) const {
    return getDasClassMethod(self,_das_class_method_offset[__fn_onBreakpoint]);
  }
  __forceinline void invoke_onBreakpoint ( Context * __context__, Func __funcCall__, void * self, Context & ctx, LineInfo const  & at, char * const  reason, char * const  text ) const {
    das_invoke_function<void>::invoke
      <void *,Context &,LineInfo const  &,char * const ,char * const >
        (__context__,nullptr,__funcCall__,
          self,ctx,at,reason,text);
  }
  __forceinline Func get_onVariable ( void * self ) const {
    return getDasClassMethod(self,_das_class_method_offset[__fn_onVariable]);
  }
  __forceinline void invoke_onVariable ( Context * __context__, Func __funcCall__, void * self, Context & ctx, char * const  category, char * const  name, TypeInfo const  & info, void * const  data ) const {
    das_invoke_function<void>::invoke
      <void *,Context &,char * const ,char * const ,TypeInfo const  &,void * const >
        (__context__,nullptr,__funcCall__,
          self,ctx,category,name,info,data);
  }
  __forceinline Func get_onBreakpointsReset ( void * self ) const {
    return getDasClassMethod(self,_das_class_method_offset[__fn_onBreakpointsReset]);
  }
  __forceinline void invoke_onBreakpointsReset ( Context * __context__, Func __funcCall__, void * self, char * const  file, int32_t breakpointsNum ) const {
    das_invoke_function<void>::invoke
      <void *,char * const ,int32_t>
        (__context__,nullptr,__funcCall__,
          self,file,breakpointsNum);
  }
  __forceinline Func get_onTick ( void * self ) const {
    return getDasClassMethod(self,_das_class_method_offset[__fn_onTick]);
  }
  __forceinline void invoke_onTick ( Context * __context__, Func __funcCall__, void * self ) const {
    das_invoke_function<void>::invoke
      <void *>
        (__context__,nullptr,__funcCall__,
          self);
  }
  __forceinline Func get_onCollect ( void * self ) const {
    return getDasClassMethod(self,_das_class_method_offset[__fn_onCollect]);
  }
  __forceinline void invoke_onCollect ( Context * __context__, Func __funcCall__, void * self, Context & ctx, LineInfo const  & at ) const {
    das_invoke_function<void>::invoke
      <void *,Context &,LineInfo const  &>
        (__context__,nullptr,__funcCall__,
          self,ctx,at);
  }
  __forceinline Func get_onLog ( void * self ) const {
    return getDasClassMethod(self,_das_class_method_offset[__fn_onLog]);
  }
  __forceinline bool invoke_onLog ( Context * __context__, Func __funcCall__, void * self, Context * const  context, LineInfo const  * const  at, int32_t level, char * const  text ) const {
    return das_invoke_function<bool>::invoke
      <void *,Context * const ,LineInfo const  * const ,int32_t,char * const >
        (__context__,nullptr,__funcCall__,
          self,context,at,level,text);
  }
};

class DapiStackWalker_Adapter {
protected:
  enum {
    __fn_canWalkArguments = 0,
    __fn_canWalkVariables = 1,
    __fn_canWalkOutOfScopeVariables = 2,
    __fn_onBeforeCall = 3,
    __fn_onCallAOT = 4,
    __fn_onCallAt = 5,
    __fn_onCall = 6,
    __fn_onAfterPrologue = 7,
    __fn_onArgument = 8,
    __fn_onBeforeVariables = 9,
    __fn_onVariable = 10,
    __fn_onAfterCall = 11,
  };
protected:
  int _das_class_method_offset[12];
public:
  DapiStackWalker_Adapter ( const StructInfo * info ) {
      _das_class_method_offset[__fn_canWalkArguments] = adapt_field_offset("canWalkArguments",info);
      _das_class_method_offset[__fn_canWalkVariables] = adapt_field_offset("canWalkVariables",info);
      _das_class_method_offset[__fn_canWalkOutOfScopeVariables] = adapt_field_offset("canWalkOutOfScopeVariables",info);
      _das_class_method_offset[__fn_onBeforeCall] = adapt_field_offset("onBeforeCall",info);
      _das_class_method_offset[__fn_onCallAOT] = adapt_field_offset("onCallAOT",info);
      _das_class_method_offset[__fn_onCallAt] = adapt_field_offset("onCallAt",info);
      _das_class_method_offset[__fn_onCall] = adapt_field_offset("onCall",info);
      _das_class_method_offset[__fn_onAfterPrologue] = adapt_field_offset("onAfterPrologue",info);
      _das_class_method_offset[__fn_onArgument] = adapt_field_offset("onArgument",info);
      _das_class_method_offset[__fn_onBeforeVariables] = adapt_field_offset("onBeforeVariables",info);
      _das_class_method_offset[__fn_onVariable] = adapt_field_offset("onVariable",info);
      _das_class_method_offset[__fn_onAfterCall] = adapt_field_offset("onAfterCall",info);
  }
  __forceinline Func get_canWalkArguments ( void * self ) const {
    return getDasClassMethod(self,_das_class_method_offset[__fn_canWalkArguments]);
  }
  __forceinline bool invoke_canWalkArguments ( Context * __context__, Func __funcCall__, void * self ) const {
    return das_invoke_function<bool>::invoke
      <void *>
        (__context__,nullptr,__funcCall__,
          self);
  }
  __forceinline Func get_canWalkVariables ( void * self ) const {
    return getDasClassMethod(self,_das_class_method_offset[__fn_canWalkVariables]);
  }
  __forceinline bool invoke_canWalkVariables ( Context * __context__, Func __funcCall__, void * self ) const {
    return das_invoke_function<bool>::invoke
      <void *>
        (__context__,nullptr,__funcCall__,
          self);
  }
  __forceinline Func get_canWalkOutOfScopeVariables ( void * self ) const {
    return getDasClassMethod(self,_das_class_method_offset[__fn_canWalkOutOfScopeVariables]);
  }
  __forceinline bool invoke_canWalkOutOfScopeVariables ( Context * __context__, Func __funcCall__, void * self ) const {
    return das_invoke_function<bool>::invoke
      <void *>
        (__context__,nullptr,__funcCall__,
          self);
  }
  __forceinline Func get_onBeforeCall ( void * self ) const {
    return getDasClassMethod(self,_das_class_method_offset[__fn_onBeforeCall]);
  }
  __forceinline void invoke_onBeforeCall ( Context * __context__, Func __funcCall__, void * self, Prologue const  & pp, void * const  sp ) const {
    das_invoke_function<void>::invoke
      <void *,Prologue const  &,void * const >
        (__context__,nullptr,__funcCall__,
          self,pp,sp);
  }
  __forceinline Func get_onCallAOT ( void * self ) const {
    return getDasClassMethod(self,_das_class_method_offset[__fn_onCallAOT]);
  }
  __forceinline void invoke_onCallAOT ( Context * __context__, Func __funcCall__, void * self, Prologue const  & pp, char * const  fileName ) const {
    das_invoke_function<void>::invoke
      <void *,Prologue const  &,char * const >
        (__context__,nullptr,__funcCall__,
          self,pp,fileName);
  }
  __forceinline Func get_onCallAt ( void * self ) const {
    return getDasClassMethod(self,_das_class_method_offset[__fn_onCallAt]);
  }
  __forceinline void invoke_onCallAt ( Context * __context__, Func __funcCall__, void * self, Prologue const  & pp, FuncInfo const  & info, LineInfo const  & at ) const {
    das_invoke_function<void>::invoke
      <void *,Prologue const  &,FuncInfo const  &,LineInfo const  &>
        (__context__,nullptr,__funcCall__,
          self,pp,info,at);
  }
  __forceinline Func get_onCall ( void * self ) const {
    return getDasClassMethod(self,_das_class_method_offset[__fn_onCall]);
  }
  __forceinline void invoke_onCall ( Context * __context__, Func __funcCall__, void * self, Prologue const  & pp, FuncInfo const  & info ) const {
    das_invoke_function<void>::invoke
      <void *,Prologue const  &,FuncInfo const  &>
        (__context__,nullptr,__funcCall__,
          self,pp,info);
  }
  __forceinline Func get_onAfterPrologue ( void * self ) const {
    return getDasClassMethod(self,_das_class_method_offset[__fn_onAfterPrologue]);
  }
  __forceinline void invoke_onAfterPrologue ( Context * __context__, Func __funcCall__, void * self, Prologue const  & pp, void * const  sp ) const {
    das_invoke_function<void>::invoke
      <void *,Prologue const  &,void * const >
        (__context__,nullptr,__funcCall__,
          self,pp,sp);
  }
  __forceinline Func get_onArgument ( void * self ) const {
    return getDasClassMethod(self,_das_class_method_offset[__fn_onArgument]);
  }
  __forceinline void invoke_onArgument ( Context * __context__, Func __funcCall__, void * self, FuncInfo const  & info, int32_t index, VarInfo const  & vinfo, float4 arg ) const {
    das_invoke_function<void>::invoke
      <void *,FuncInfo const  &,int32_t,VarInfo const  &,float4>
        (__context__,nullptr,__funcCall__,
          self,info,index,vinfo,arg);
  }
  __forceinline Func get_onBeforeVariables ( void * self ) const {
    return getDasClassMethod(self,_das_class_method_offset[__fn_onBeforeVariables]);
  }
  __forceinline void invoke_onBeforeVariables ( Context * __context__, Func __funcCall__, void * self ) const {
    das_invoke_function<void>::invoke
      <void *>
        (__context__,nullptr,__funcCall__,
          self);
  }
  __forceinline Func get_onVariable ( void * self ) const {
    return getDasClassMethod(self,_das_class_method_offset[__fn_onVariable]);
  }
  __forceinline void invoke_onVariable ( Context * __context__, Func __funcCall__, void * self, FuncInfo const  & inf, LocalVariableInfo const  & vinfo, void * const  arg, bool inScope ) const {
    das_invoke_function<void>::invoke
      <void *,FuncInfo const  &,LocalVariableInfo const  &,void * const ,bool>
        (__context__,nullptr,__funcCall__,
          self,inf,vinfo,arg,inScope);
  }
  __forceinline Func get_onAfterCall ( void * self ) const {
    return getDasClassMethod(self,_das_class_method_offset[__fn_onAfterCall]);
  }
  __forceinline bool invoke_onAfterCall ( Context * __context__, Func __funcCall__, void * self, Prologue const  & pp ) const {
    return das_invoke_function<bool>::invoke
      <void *,Prologue const  &>
        (__context__,nullptr,__funcCall__,
          self,pp);
  }
};

