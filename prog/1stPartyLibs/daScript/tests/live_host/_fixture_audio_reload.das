options gen2
options persistent_heap
options gc
options no_aot

require live/live_api
require live/live_vars
require live/audio_live
require audio/audio_boost
require live_host

//! The audio lifecycle a game runs under the live host: the system is created once, a reload,
//! a full reload and a reset keep it and refill the program's handle copy, the real shutdown
//! finalizes it. Every init pushes a command through the handles it holds, so a dead stream
//! throws here.

var asch : AudioSystemChannels
var @live inits = 0

[export]
def init() {
    let adopted = get_audio_command_stream() != null
    asch = audio_live_system()
    inits ++
    to_log(LOG_INFO, "fixture: init {inits} adopted={adopted}\n")
    set_global_volume(0.5)
}

[export]
def update() {
}

[export]
def shutdown() {
    audio_live_finalize(asch)
}
