options no_aot = true
module logerr_once shared

require ast
require rtti
require daslib/ast_boost
require daslib/templates_boost
require DagorSystem public


[tag_function(logerr_once_tag), unused_argument(message)]
def logerr_once(message : string) {}

// convert logerr_once(message) to
//  var
//      __logerr_once_I = true
//  if __logerr_once_I
//      __logerr_once_I = false
//      logerr(message)
[tag_function_macro(tag="logerr_once_tag")]
class AssertOnceMacro : AstFunctionAnnotation
  def override transform(var call : smart_ptr<ExprCallFunc>; var errors : das_string) : ExpressionPtr
    let once_name = make_unique_private_name("__logerr_once", call.at)
    if !add_global_var(compiling_module(), once_name, call.at) <| quote(true)
      errors := "can't add global variable {once_name}"
      return <- [[ExpressionPtr]]

    return <- qmacro_block() <|
      if $i(once_name)
        $i(once_name) = false
        logerr($e(call.arguments[0]))
