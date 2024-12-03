let { get_setting_by_blk_path, set_setting_by_blk_path_and_save } = require("settings")

const masterServerUrl = "masterServerUrl"

let get_master_server_url = @() get_setting_by_blk_path(masterServerUrl) ?? ""
let set_master_server_url = @(v) set_setting_by_blk_path_and_save("masterServerUrl", type(v)=="string" ? v.strip() : "")

return {get_master_server_url, set_master_server_url}