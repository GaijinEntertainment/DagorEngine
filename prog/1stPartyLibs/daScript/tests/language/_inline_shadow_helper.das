options gen2
options indenting = 4
module _inline_shadow_helper public
require daslib/ast public
require daslib/ast_boost

// Marks the annotated block's arguments can_shadow, which is what a host ECS does for query blocks
// (dagor: gameLibs/ecs/scripts/das/das_es.cpp sets arg->can_shadow on every [es] block argument) and
// what daslib/linq_boost does for its generated blocks.
[block_macro(name="shadowable")]
class ShadowableBlock : AstBlockAnnotation {
    def override apply(var blk : ExprBlock?; var group : ModuleGroup;
                       args : AnnotationArgumentList; var errors : das_string) : bool {
        for (a in blk.arguments) {
            a.flags.can_shadow = true
        }
        return true
    }
    def override finish(var blk : ExprBlock?; var group : ModuleGroup;
                        args, progArgs : AnnotationArgumentList; var errors : das_string) : bool {
        return true
    }
}
