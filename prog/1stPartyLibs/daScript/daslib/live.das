options no_unused_block_arguments = false
options no_unused_function_arguments = false
options indenting = 4
options strict_smart_pointers = true

options multiple_contexts

module live shared public

require strings
require daslib/rtti
require daslib/decs
require daslib/static_let
require debugapi
require fio

var private appPtr : smart_ptr<Context>
var private appFile = "app.das"
var private appDir  = get_das_root() + "/modules/dasGlfw/framework"
var private appLive = false

var private appTime : table<string;clock>
var private watchTime = get_clock()

var private liveFunctionLookup : table<string; bool>

var public decsLiveData : array<uint8>

var private liveContext : smart_ptr<Context>

def public get_live_context : smart_ptr<Context>
    //! get live context. WARNING: this does not increase reference count, no need for delete
    //! intended use is invoke_in_context(get_live_context()) <| @ ...
    unsafe
        return liveContext  // we don't clone here?

[export]
def saveLiveContext
    to_log(LOG_TRACE, "LIVE: saveLiveContext\n");
    decsLiveData <- mem_archive_save(decsState)

[export]
def restoreLiveContext
    to_log(LOG_TRACE, "LIVE: restoreLiveContext\n");
    if length(decsLiveData) > 0
        decsLiveData |> mem_archive_load(decsState)

[finalize]
def private go_offline
    to_log(LOG_TRACE, "LIVE: go_offline\n");
    appPtr := null

def public is_live
    return appLive

def public is_app_live
    return appPtr != null

def public go_main
    to_log(LOG_TRACE, "LIVE: go_main\n");
    delete decsLiveData
    appLive = false
    unsafe
        var ctx <- reinterpret<smart_ptr<Context>> addr(this_context())
        appPtr := ctx

def public go_live ( appf, appd : string )
    to_log(LOG_TRACE, "LIVE: go_life appFile={appf} appDir={appd}\n");
    appLive = true
    this_context().name := "Live context"
    appFile = appf
    appDir = appd
    delete decsLiveData
    recompile()

def private live_lookup(fn)
    if appPtr==null
        return false
    if !liveFunctionLookup |> key_exists(fn)
        liveFunctionLookup[fn] = *appPtr |> has_function(fn)
    return liveFunctionLookup[fn]

def public invoke_live ( fn:string )
    if live_lookup(fn)
        try
            unsafe
                invoke_in_context(appPtr, fn)
        recover
            to_log(LOG_ERROR, "exception during '{fn}()'\n{appPtr.last_exception}\n{describe(appPtr.exceptionAt)}\n")
            appPtr := null

def public invoke_live ( fn:string; a1 )
    if live_lookup(fn)
        try
            unsafe
                invoke_in_context(appPtr, fn, a1)
        recover
            to_log(LOG_ERROR, "exception during '{fn}({a1})'\n{appPtr.last_exception}\n{describe(appPtr.exceptionAt)}\n")
            appPtr := null

def public invoke_live ( fn:string; a1; a2 )
    if live_lookup(fn)
        try
            unsafe
                invoke_in_context(appPtr, fn, a1, a2)
        recover
            to_log(LOG_ERROR, "exception during '{fn}({a1},{a2})'\n{appPtr.last_exception}\n{describe(appPtr.exceptionAt)}\n")
            appPtr := null

def public invoke_live ( fn:string; a1; a2; a3 )
    if live_lookup(fn)
        try
            unsafe
                invoke_in_context(appPtr, fn, a1, a2, a3)
        recover
            to_log(LOG_ERROR, "exception during '{fn}({a1},{a2},{a3})'\n{appPtr.last_exception}\n{describe(appPtr.exceptionAt)}\n")
            appPtr := null

def public invoke_live ( fn:string; a1; a2; a3; a4 )
    if live_lookup(fn)
        try
            unsafe
                invoke_in_context(appPtr, fn, a1, a2, a3, a4)
        recover
            to_log(LOG_ERROR, "exception during '{fn}({a1},{a2},{a3},{a4})'\n{appPtr.last_exception}\n{describe(appPtr.exceptionAt)}\n")
            appPtr := null

def public watch_files
    var clk = get_clock()
    if (clk - watchTime) <= 0.0lf
        return false
    watchTime = clk
    var any = false
    dir(appDir) <| $ (fname)
        if !(ends_with(fname, ".das") || ends_with(fname, ".das_project"))
            return
        let fileName = "{appDir}\/{fname}"
        var st : FStat
        if stat(fileName,st)
            if st.is_reg
                if appTime |> key_exists(fname)
                    if appTime[fname] != st.mtime
                        any = true
                appTime[fname] = st.mtime
    return any

def private set_new_context ( var ptr : smart_ptr<Context>; full_restart:bool = false )
    to_log(LOG_TRACE, "LIVE: set new context {intptr(get_ptr(ptr))}\n")
    invoke_live("shutdown")
    if appPtr != null
        unsafe
            // fixup decs if its there
            var pvar = get_context_global_variable(appPtr, "decsState")
            if pvar != null
                var pstate : array<uint8> ? = unsafe(reinterpret<array<uint8>?> get_context_global_variable(appPtr, "decsLiveData"))
                assert(pstate!=null)
                var css <- @ <|
                    saveLiveContext()
                invoke_in_context(appPtr, css)
                decsLiveData := *pstate
                delete css
    appPtr := ptr
    delete liveFunctionLookup
    if appPtr != null
        appPtr.alwaysStackWalkOnException = true
        unsafe
            // fixup decs if its there
            var pvar = get_context_global_variable(appPtr, "decsState")
            if pvar != null
                var pstate : array<uint8> ? = unsafe(reinterpret<array<uint8>?> get_context_global_variable(appPtr, "decsLiveData"))
                assert(pstate!=null)
                var css <- @ <| [[&decsState,&decsLiveData]] ( data : array<uint8># )
                    decsLiveData := data
                    restoreLiveContext()
                    delete decsLiveData
                invoke_in_context(appPtr, css, decsLiveData)
                delete css
            // fix-up live context, if its there
            var pctx_var = get_context_global_variable(appPtr, "liveContext")
            if pctx_var != null
                var lContext <- reinterpret<smart_ptr<Context>> addr(this_context())
                var css <- @ <| [[&decsState]] ( state : DecsState? )
                    unsafe
                        liveContext <- lContext
                invoke_in_context(appPtr, css, addr(decsState))
                delete css
        invoke_live("initialize", full_restart)

def public recompile(full_restart:bool=false)
    to_log(LOG_TRACE, "LIVE: recompile full_restart={full_restart}\n")
    var inscope access <- make_file_access("")
    using <| $(var mg:ModuleGroup)
        using <| $(var cop:CodeOfPolicies)
            cop.threadlock_context = true
            compile_file("{appDir}/{appFile}",access,unsafe(addr(mg)),cop) <| $(ok,program,issues)
                if ok
                    simulate(program) <| $ ( sok; context; serrors )
                        if sok
                            // TODO: beep print("reloaded...\n")
                            to_log(LOG_TRACE, "LIVE: reloaded\n")
                            set_new_context(context,full_restart)
                        else
                            to_log(LOG_ERROR, "LIVE: {appFile} failed to simulate:\n{issues}\n")
                            set_new_context([[smart_ptr<Context>]],full_restart)
                else
                    to_log(LOG_ERROR, "LIVE: {appFile} failed to compile:\n{issues}\n")
                    set_new_context([[smart_ptr<Context>]],full_restart)

def public live_keypressed ( keycode, scancode, action, mods : int )
    invoke_live("keypressed", keycode, scancode, action, mods )
    if is_live()
        if scancode==19 && action==1 && mods==2
            recompile(true)

def public live_charpressed ( charcode : uint )
    invoke_live("charpressed", charcode )

def public live_mousemoved ( x, y : double )
    invoke_live("mousemoved", x, y )

def public live_mouseenter ( yes : bool )
    invoke_live("mouseenter", yes)

def public live_mousebutton ( button, action, mods : int )
    invoke_live("mousebutton", button, action, mods )

def public live_mousescroll ( x, y : double )
    invoke_live("mousescroll", x, y )

def public live_update ( appTime : double )
    static_let <|
        var t = 0.0lf
    let dt = float(appTime - t)
    t = appTime
    invoke_live("act", dt)
