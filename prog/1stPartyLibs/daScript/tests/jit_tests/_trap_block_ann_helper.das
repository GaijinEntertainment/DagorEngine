options gen2

module _trap_block_ann_helper public

require daslib/ast public
require daslib/ast_boost

//! The payload a `[data_block]` block carries. A host puts a real per-block pointer here (dagor's
//! ECS stores the baked query description); the test only needs a recognizable nonzero value.
let public DATA_BLOCK_PAYLOAD = 0xDA5CDA7Aul
let public DATA_BLOCK_SID = 0xB10CDA7Aul

[block_macro(name="data_block")]
class DataBlockAnnotation : AstBlockAnnotation {
    def override apply(var blk : ExprBlock?; var group : ModuleGroup;
                       args : AnnotationArgumentList; var errors : das_string) : bool {
        return true
    }
    def override finish(var blk : ExprBlock?; var group : ModuleGroup;
                        args, progArgs : AnnotationArgumentList; var errors : das_string) : bool {
        blk.annotationData = DATA_BLOCK_PAYLOAD
        blk.annotationDataSid = DATA_BLOCK_SID
        return true
    }
}
