// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

namespace gameproj
{
//! [mandatory] game codename
extern const char *game_codename();
//! [mandatory] filepath to load main vromfs (it must contain at least settings)
extern const char *main_vromfs_fpath();
//! [mandatory] mount dir for main fromfs
extern const char *main_vromfs_mount_dir();
//! [mandatory] window game title, used before localizations are inited
extern const char *game_window_title();
//! [mandatory] userdocs folder name
extern const char *game_user_dir_name();
//! [mandatory] app name for telemetry
extern const char *game_telemetry_name();
//! [optional] YUP name for consistency selfcheck and vromfs updater
extern const char *game_yup_name();

//! [mandatory] settings to be accessed using ::dgs_get_settings()
static inline const char *settings_fpath() { return "config/settings.blk"; }
//! [mandatory] config BLK filepath prefix (e.g. GAME.config) without .blk suffix
extern const char *config_name_prefix();
//! [mandatory] settings to be accessed using ::dgs_get_game_params()
static inline const char *params_fpath() { return "config/gameparams.blk"; }
//! [optional] ES order configuration (can be set as es_order:t="" in settings.blk if not used)
static inline const char *es_order_fpath() { return "config/es_order.blk"; }

//! [mandatory] default statsd URL to be used when stats_url:t= not set
extern const char *default_statsd_url();
//! [mandatory] statsd key
extern const char *statsd_key();
//! [mandatory] default eventlog project name to be used when eventLog{project:t= not set
extern const char *default_eventlog_project();
//! [mandatory] eventlog agent name
extern const char *eventlog_agent();

//! [mandatory] public SHA256 key to be used for checking signed vromfs and other data
extern const unsigned char *public_key_DER();
//! [mandatory] length of public SHA256 key
extern unsigned public_key_DER_len();
//! [mandatory] length of partners public SHA256 keys
unsigned partner_public_keys_count();
//! [mandatory] public SHA256 key to be used for checking partners signed vromfs and other data
extern const unsigned char *partner_public_key_DER(int i);
//! [mandatory] length of partner public SHA256 key
extern unsigned partner_public_key_DER_len(int i);

//! [optional] additional code may be implemented by project if UseGameCallbacks = yes in jamfile
extern void before_init_video();
//! [optional] additional code may be implemented by project if UseGameCallbacks = yes in jamfile
extern void init_before_main_loop();
//! [optional] additional code may be implemented by project if UseGameCallbacks = yes in jamfile
extern void update_before_dagor_work_cycle();
//! [optional] additional code may be implemented by project if UseGameCallbacks = yes in jamfile
extern void post_shutdown_handler();
//! [optional] additional code may be implemented by project if UseGameCallbacks = yes in jamfile
extern void reset_game_resources();


//! [mandatory] is this build a dll for hosted server instance
extern bool is_hosted_server_instance();

} // namespace gameproj
