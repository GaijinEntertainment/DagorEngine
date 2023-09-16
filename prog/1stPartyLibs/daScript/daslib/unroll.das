options indenting = 4
options no_unused_block_arguments = false
options no_unused_function_arguments = false
options no_aot = true
options strict_smart_pointers = true

module unroll shared private

require ast
require rtti
require daslib/ast_boost
require daslib/templates_boost

[tag_function(unroll_tag)]
def public unroll ( blk : block )
    //! Unrolls the for loop (with fixed range)
    invoke(blk)

[tag_function_macro(tag="unroll_tag")]
class UnrollMacro : AstFunctionAnnotation
    //! This macro implements loop unrolling in the form of `unroll` function.
    //! Unroll function expects block with the single for loop in it.
    //! Moveover only range for is supported, and only with the fixed range.
    //! For example:::
    //!
    //!     var n : float4[9]
    //!     unroll <|   // contents of the loop will be replaced with 9 image load instructions.
    //!         for i in range(9)
    //!             n[i] = imageLoad(c_bloom_htex, xy + int2(0,i-4))
    def override transform ( var call : smart_ptr<ExprCallFunc>; var errors : das_string ) : ExpressionPtr
        assert(call.arguments[0] is ExprMakeBlock)
        unsafe
            var mblk = reinterpret<ExprMakeBlock?> call.arguments[0]
            var blk = reinterpret<ExprBlock?> mblk._block
            if blk.finalList |> length != 0
                errors := "not expecting finally section in the unroll"
                return [[ExpressionPtr]]
            if blk.list |> length != 1 || !(blk.list[0] is ExprFor)
                errors := "expecting unroll <| for ..."
                return [[ExpressionPtr]]
            let efor = reinterpret<ExprFor?> blk.list[0]
            if !(efor.sources[0] is ExprConstRange)
                errors := "can only unroll for loop with range"
                return [[ExpressionPtr]]
            let euc = reinterpret<ExprConstRange?> efor.sources[0]
            let van = "{efor.iterators[0]}"
            var nblk <- new [[ExprBlock() at=call.at]]
            for i in euc.value
                var iblk <- clone_expression(efor.body)
                remove_deref(van, iblk)
                var rules : Template
                rules |> replaceVariable(van) <| new [[ExprConstInt() at=call.at, value=i]]
                rules |> apply_template(call.at,iblk,false)
                nblk.list |> emplace(iblk)
            return nblk

