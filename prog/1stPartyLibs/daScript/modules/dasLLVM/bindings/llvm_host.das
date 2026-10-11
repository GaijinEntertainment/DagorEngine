require dasbind public
options gen2
module llvm_host shared

//! the host queries the jit plan folds into its cache key - the only LLVM the DLL-cache hit path
//! touches, so the plan requires this and not the full llvm_func binding
[extern(cdecl, name="LLVMDisposeMessage", library="LLVM.dll")]
def LLVMDisposeMessage(Message : string implicit) : void {}
[extern(cdecl, name="LLVMGetDefaultTargetTriple", library="LLVM.dll")]
def LLVMGetDefaultTargetTriple() : string {}
[extern(cdecl, name="LLVMGetHostCPUName", library="LLVM.dll")]
def LLVMGetHostCPUName() : string {}
[extern(cdecl, name="LLVMGetHostCPUFeatures", library="LLVM.dll")]
def LLVMGetHostCPUFeatures() : string {}
[extern(cdecl, name="LLVMLoadLibraryPermanently", library="LLVM.dll")]
def LLVMLoadLibraryPermanently(Filename : string implicit) : int {}
[extern(cdecl, name="LLVMSearchForAddressOfSymbol", library="LLVM.dll")]
def LLVMSearchForAddressOfSymbol(symbolName : string implicit) : void? {}
