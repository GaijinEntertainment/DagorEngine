//expect:w258

// -file:param-hides-param

::requestUpdateEventLb <- require("sq3_sa_test").requestUpdateEventLb
::add_bg_task_cb <- require("sq3_sa_test").add_bg_task_cb
::handleLbRequest <- require("sq3_sa_test").handleLbRequest
::requestEventLbSelfRow <- require("sq3_sa_test").requestEventLbSelfRow
::handleLbSelfRowRequest <- require("sq3_sa_test").handleLbSelfRowRequest
::array_shift <- require("sq3_sa_test").array_shift


::Aaa <- class {
  function requestUpdateEventLb(_) {}
  canRequestEventLb = false
  leaderboardsRequestStack = []

  function updateEventLb(requestData, id) {
    local requestAction = (@(requestData, id) function () {
      local taskId = ::requestUpdateEventLb(requestData)
      if (taskId < 0)
        return

      this.canRequestEventLb = false
      ::add_bg_task_cb(taskId, (@(requestData, id) function() {
        ::handleLbRequest(requestData, id)

        if (this.leaderboardsRequestStack.len())
          ::array_shift(this.leaderboardsRequestStack).fn()
        else
          this.canRequestEventLb = true
      })(requestData, id).bindenv(this))
    })(requestData, id).bindenv(this)

    if (this.canRequestEventLb)
      return requestAction()

    if (id)
      foreach (index, request in this.leaderboardsRequestStack)
        if (id == request) {
          this.leaderboardsRequestStack.remove(index)
          break
        }

    this.leaderboardsRequestStack.append({fn = requestAction, id = id})
  }



  function updateEventLbSelfRow(requestData, id) {
    local requestAction = (@(requestData, id) function () {
      local taskId = ::requestEventLbSelfRow(requestData)
      if (taskId < 0)
        return

      this.canRequestEventLb = false
      ::add_bg_task_cb(taskId, (@(requestData, id) function() {
        ::handleLbSelfRowRequest(requestData, id)

        if (this.leaderboardsRequestStack.len())
          ::array_shift(this.leaderboardsRequestStack).fn()
        else
          this.canRequestEventLb = true
      })(requestData, id).bindenv(this))
    })(requestData, id).bindenv(this)

    if (this.canRequestEventLb)
      return requestAction()

    if (id)
      foreach (index, request in this.leaderboardsRequestStack)
        if (id == request) {
          this.leaderboardsRequestStack.remove(index)
          break
        }

    this.leaderboardsRequestStack.append({fn = requestAction, id = id})
  }
}