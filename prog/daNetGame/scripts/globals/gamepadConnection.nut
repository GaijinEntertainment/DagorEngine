from "eventbus" import eventbus_subscribe

function register_for_input_devices_count_change(callback) {
  eventbus_subscribe("input.devices_num_changed", function(counts) {
    callback?(counts)
  })
}


return freeze({
  register_for_input_devices_count_change
})
