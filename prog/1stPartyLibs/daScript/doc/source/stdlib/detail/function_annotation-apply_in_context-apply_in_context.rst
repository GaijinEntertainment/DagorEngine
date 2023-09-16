[apply_in_context] function annotation.
Function is modified, so that it is called in the debug agent context, specified in the annotation.
If specified context is not insalled, panic is called.

For example::
 [apply_in_context(opengl_cache)]
 def public cache_font(name:string implicit) : Font?
     ...
 ...
 let font = cache_font("Arial") // call invoked in the "opengl_cache" debug agent context
