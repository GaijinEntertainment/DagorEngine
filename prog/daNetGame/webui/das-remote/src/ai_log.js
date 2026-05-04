const ER_EXIT = -2
const ER_NONE = -1
const ER_RUNNING = 0
const ER_SUCCESS = 1
const ER_FAILED = 2

const ERR_NONE = -1
const ERR_SKIP = 0
const ERR_ABORT = 1
const ERR_REACT = 2

const NODE_SYMBOLS = {
  sequencer: " → ",
  selector: " ? ",
  orderedSelector: " ?? ",
  parallel: " || ",
  reaction: " 🗲 ",
  repeat: " ↻ ",
  repeatUntilFail: " ↻ ",
}

Vue.component('ai_log', {
  data: function () {
    return {
      botsList: [], // array of [eid, template, debug, track_mind]
      mindset: {},
      behTrees: [],
      behTreesCache: [], // array of {id: behTreeRef}, to skip multiple find/findAll calls
      behTreeEids: [],
      lastUpdateTime: -1,
      treeOptions: {
        checkbox: true,
        autoCheckChildren: false,
      },
      botActionsHistory: [],
      historySize: 50,
      showActionsHistory: false,
      logFilter: '',
      lastUpdateTimeByEid: {},
      snapshotTrees: [],
      snapshotTreeIds: [],
      expandedJsonEntries: {},
    }
  },
  mounted() {
    jrpc.on("ai_log.mind", (eid, data) => {
      this.addJsonLog('ai_log.mind', { eid, data })
      this.$set(this.mindset, eid, data)
    })
    jrpc.on("ai_log.beh_tree_update_result", this.handleBehTreeUpdateNotif)
    jrpc.on("ai_log.beh_tree_reset", this.handleBehTreeResetNotif)
    jrpc.notification('ai_log.reset_beh_tree_debug_for_all')
    this.getBots()
  },
  methods: {
    uncheckNode(eid, node) {
      jrpc.notification('ai_log.set_force_result', [eid, node.data.id, node.data.isReaction ? ERR_SKIP : ER_FAILED])
    },
    checkNode(eid, node) {
      jrpc.notification('ai_log.set_force_result', [eid, node.data.id, -1])
    },
    getBots() {
      let me = this
      jrpc.call('ai_log.get_bots').then(data => {
        data.sort((a, b) => a[0] - b[0])
        //push selected bot to the beginning
        for (let i = 0; i < data.length; i++){
          if (data[i][1].indexOf("daeditor_selected") != -1){
            data.splice(0, 0, data[i])
            data.splice(i + 1, 1)
            break;
          }
        }
        me.botsList = data
      })
    },
    spectate(eid) {
      jrpc.notification('camera.spectate', eid)
    },
    setDebug(eid, value) {
      jrpc.call('ai_log.beh_tree_debug', [eid, value]).then(this.updateBotData)
    },
    trackMind(eid, track) {
      jrpc.call('ai_log.track_mind', [eid, track]).then(this.updateBotData)
    },
    updateBotData(data) {
      let eid = data[0]
      for (let idx = 0; idx < this.botsList.length; idx++)
        if (this.botsList[idx][0] == eid) {
          this.botsList.splice(idx, 1, data)
          break
        }
    },
    updateBehTree(eid, tree) {
      let index = this.behTreeEids.indexOf(eid)
      if (index >= 0) {
        this.behTrees.splice(index, 1, tree)
        this.behTreesCache.splice(index, 1, {})
      }
      else {
        this.behTrees.push(tree)
        this.behTreeEids.push(eid)
        this.behTreesCache.push({})
      }
    },
    removeBehTree(eid) {
      let index = this.behTreeEids.indexOf(eid)
      if (index >= 0) {
        jrpc.notification('ai_log.set_beh_tree_debug', [this.behTreeEids[index], false])
        this.behTrees.splice(index, 1)
        this.behTreeEids.splice(index, 1)
        this.behTreesCache.splice(index, 1)
        delete this.lastUpdateTimeByEid[eid]
      }
    },
    requestBehTree(eid) {
      jrpc.notification('ai_log.set_beh_tree_debug', [eid, true])
      setTimeout(() => jrpc.call('ai_log.get_beh_tree', eid).then((data) => {
        this.updateBehTree(eid, transform(data))
      }), 100)
    },
    collapseTree(refName, value) {
      let etree = this.$refs[refName]
      if (!etree || !etree[0]) return
      let children = etree[0].findAll({}, true)
      if (value)
        children.collapse();
      else
        children.expand()
    },
    toggleBTEnabled(eid) {
      jrpc.notification('ai_log.toggle_beh_tree_enabled', [eid])
    },
    resetBT(eid) {
      jrpc.notification('ai_log.force_recalculate_tree', [eid])
    },
    destroyAgent(eid) {
      let me = this
      jrpc.notification('ai_log.destroy_entity', [eid])
      setTimeout(() => {me.removeBehTree(eid); me.getBots()}, 100)
    },
    handleBehTreeUpdateNotif(eid, time, nodes) {
      this.addJsonLog('ai_log.beh_tree_update_result', { eid, time, nodes })

      if (time > this.lastUpdateTime)
        this.lastUpdateTime = time
      let treeIndex = this.behTreeEids.indexOf(eid)
      if (treeIndex == -1)
        return
      let etree = this.$refs["bt_" + eid][0]

      let nodesCache = this.behTreesCache[treeIndex]
      for (const idx in nodesCache) {
        Vue.set(nodesCache[idx].data, 'isLastRunning', false)
      }

      var lastRunningNode = -1
      var lastRunningReaction = -1
      for (it in nodes) {
        let key = parseInt(it)
        lastRunningNode = key > lastRunningNode && nodes[key].updateResult == ER_RUNNING ? key : lastRunningNode
        lastRunningReaction = key > lastRunningReaction && nodes[key].reactResult == ERR_REACT ? key : lastRunningReaction
      }

      for (it in nodes) {
        let key = parseInt(it)
        let inCache = key in nodesCache
        let enode = inCache ? nodesCache[key] : etree.find({ data: { id: key } }, false)[0]
        if (!inCache)
          nodesCache[key] = enode
        let data = enode.data
        Vue.set(data, 'isLastRunning', data.id == lastRunningNode || data.id == lastRunningReaction) // unused???
        let updateData = nodes[it]
        Vue.set(data, 'lastVisit', time) // unused?

        if (data.lastUpdateResult != ER_RUNNING && updateData.updateResult == ER_RUNNING)
          Vue.set(data, 'startTime', time) // TODO: move to c++

        if (data.isReaction)
          Vue.set(data, 'lastReactResult', updateData.reactResult)
        else
          Vue.set(data, 'lastUpdateResult', updateData.updateResult)

        Vue.set(data, 'describe', updateData.describe)
        Vue.set(data, 'forceResult', updateData.forceResult)

        if (updateData.updateResult == ER_RUNNING) {
          if (!data.isReaction) {
            if (enode.collapsed())
              enode.expand()
          }
        }
      }

      //TODO:
      //for (key in tree)
      //  if (tree[key].meta.lastUpdateResult != ER_RUNNING)
      //    this.resetTree(tree)
    },
    handleBehTreeResetNotif(eid, nodeId) {
      //TODO:
      //let treeIndex = this.behTreeEids.indexOf(eid)
      //if (treeIndex != -1) {
      //  this.resetTree(this.behTrees[treeIndex])
      //}
    },
    resetTree(eid) {
      let etree = this.$refs["bt_" + eid]
      if (!etree || !etree[0]) return
      let children = etree[0].findAll({}, true)
      for (const node of children) {
        Vue.set(node.data, 'startTime', -1)
        Vue.set(node.data, 'lastVisit', -1)
        Vue.set(node.data, 'lastUpdateResult', -1)
        Vue.set(node.data, 'lastReactResult', -1)
        Vue.set(node.data, 'isLastRunning', false)
        Vue.set(node.data, 'forceResult', -1)
        Vue.set(node.data, 'describe', {})
      }
    },
    behTreeRef(index) {
      return "bt_" + this.behTreeEids[index]
    },
    getNodeClass(node) {
      const data = node.data
      return {
        beh_node: true,
        runningNode: data.lastUpdateResult == ER_RUNNING,
        succededNode: "lastUpdateResult" in data ? data.lastUpdateResult == ER_SUCCESS : data.lastReactResult == ERR_REACT,
        failedNode: data.lastUpdateResult == ER_FAILED,
        exitNode: data.lastUpdateResult == ER_EXIT,
        lastRunningNode: data.isLastRunning
      }
    },
    getNodeStatus(node) {
      const data = node.data
      if (data.isLastRunning && data.lastUpdateResult == ER_RUNNING) {
        if (!data.isReaction) {
          let runningTime = this.lastUpdateTime - data.startTime
          return runningTime.toFixed(2)
        }
        else {
          let runningTime = this.lastUpdateTime - data.startTime
          return runningTime.toFixed(2)
        }
      }
      return ""
    },
    makeBreakable(template) {
      let parser = new DOMParser(); //In order to use special characters like &thinsp; (thin space)
      let dom = parser.parseFromString('<!doctype html><body>' + template.replace(/([+])/g,"+&thinsp;"), 'text/html');
      return dom.body.textContent;
    },
    addJsonLog(type, data) {
      const eid = data.eid
      const currentTime = data.time

      if (eid && currentTime !== undefined) {
        const lastTime = this.lastUpdateTimeByEid[eid]
        if (lastTime !== undefined && lastTime === currentTime) {
          return
        }
        this.lastUpdateTimeByEid[eid] = currentTime
      }

      const timestamp = currentTime !== undefined ? currentTime.toFixed(6) : new Date().toLocaleTimeString()
      this.botActionsHistory.unshift({ timestamp, type, data })
      const overflowHistory = this.botActionsHistory.length > this.historySize
      if (overflowHistory) {
        this.botActionsHistory.pop()
      }
    },

    clearJsonLog() {
      this.botActionsHistory = []
      this.lastUpdateTimeByEid = {}
      this.expandedJsonEntries = {}
    },

    toggleJsonView(idx) {
      this.$set(this.expandedJsonEntries, idx, !this.expandedJsonEntries[idx])
    },

    copyJsonToClipboard(entry) {
      const text = JSON.stringify(entry.data, null, 2)
      navigator.clipboard.writeText(text).then(() => {
        this.$message({ message: 'JSON copied to clipboard', type: 'success', duration: 1000 })
      })
    },

    showSnapshotTree(entry) {
      const eid = entry.data.eid
      const time = entry.timestamp
      const snapshotId = `${eid}@${time}`

      if (this.snapshotTreeIds.indexOf(snapshotId) !== -1) {
        this.$message({ message: 'This snapshot is already shown', type: 'info', duration: 1000 })
        return
      }

      const treeIndex = this.behTreeEids.indexOf(eid)
      console.log('Looking for eid:', eid, 'in behTreeEids:', this.behTreeEids, 'found index:', treeIndex)

      if (treeIndex === -1) {
        this.$message({ message: `Tree structure not available for eid ${eid}. Open live BT first.`, type: 'warning', duration: 2000 })
        return
      }

      const originalTree = this.behTrees[treeIndex]
      const tree = JSON.parse(JSON.stringify(originalTree))

      const snapshotNodes = entry.data.nodes
      this.applySnapshotToTree(tree, snapshotNodes, entry.data.time)

      tree.isSnapshot = true
      tree.snapshotTime = time
      tree.snapshotEid = eid

      this.snapshotTrees.push(tree)
      this.snapshotTreeIds.push(snapshotId)
    },
    applySnapshotToTree(node, snapshotNodes, time) {
      if (node.data && node.data.id !== undefined) {
        const nodeId = String(node.data.id)
        const snapshotData = snapshotNodes[nodeId]
        if (snapshotData) {
          Vue.set(node.data, 'lastUpdateResult', snapshotData.updateResult)
          Vue.set(node.data, 'lastReactResult', snapshotData.reactResult)
          Vue.set(node.data, 'describe', snapshotData.describe)
          Vue.set(node.data, 'forceResult', snapshotData.forceResult)
          Vue.set(node.data, 'lastVisit', time)
        }
      }

      if (node.children && node.children.length > 0) {
        for (let child of node.children) {
          this.applySnapshotToTree(child, snapshotNodes, time)
        }
      }
    },

    removeSnapshotTree(snapshotId) {
      const index = this.snapshotTreeIds.indexOf(snapshotId)
      if (index >= 0) {
        this.snapshotTrees.splice(index, 1)
        this.snapshotTreeIds.splice(index, 1)
      }
    },

    filteredJsonLogs() {
      if (!this.logFilter) return this.botActionsHistory
      return this.botActionsHistory.filter(entry => {
        const eid = entry.data.eid
        return eid && String(eid).includes(this.logFilter)
      })
    },
    resultToString(result, isReaction) {
      if (isReaction) {
        switch (result) {
          case ERR_REACT: return 'REACT'
          case ERR_ABORT: return 'ABORT'
          case ERR_SKIP: return 'SKIP'
          default: return ''
        }
      }
      switch (result) {
        case ER_RUNNING: return 'RUNNING'
        case ER_SUCCESS: return 'SUCCESS'
        case ER_FAILED: return 'FAILED'
        case ER_EXIT: return 'EXIT'
        default: return ''
      }
    },
    describeToString(describe) {
      if (!describe) return ''
      const entries = Array.isArray(describe) ? describe : Object.values(describe)
      if (entries.length === 0) return ''
      return ' (' + entries.map(it => `${it[0]}=${it[1]}`).join(', ') + ')'
    },
    nodeData(node, cache) {
      const rawData = node.data || {}
      const cached = cache ? cache[rawData.id] : null
      return cached ? cached.data : rawData
    },
    treeToText(node, depth, hasAnyResults, cache) {
      const data = this.nodeData(node, cache)
      const result = data.isReaction ? data.lastReactResult : data.lastUpdateResult
      if (hasAnyResults && (result === undefined || result === ER_NONE)) return ''
      const indent = '\t'.repeat(depth)
      const typeName = node.typeName || ''
      const name = node.name && node.name !== typeName ? ` [${node.name}]` : ''
      let resultStr = this.resultToString(result, data.isReaction)
      if (!data.isReaction && result === ER_RUNNING && data.startTime >= 0)
        resultStr += ` ${(this.lastUpdateTime - data.startTime).toFixed(2)}s`
      const describeStr = this.describeToString(data.describe)
      let text = `${indent}${typeName}${name}${resultStr ? ' ' + resultStr : ''}${describeStr}\n`
      if (node.children)
        for (const child of node.children)
          text += this.treeToText(child, depth + 1, hasAnyResults, cache)
      return text
    },
    treeHasAnyResults(node, cache) {
      const data = this.nodeData(node, cache)
      const result = data.isReaction ? data.lastReactResult : data.lastUpdateResult
      if (result !== undefined && result !== ER_NONE) return true
      if (node.children)
        for (const child of node.children)
          if (this.treeHasAnyResults(child, cache)) return true
      return false
    },
    copyToClipboard(text) {
      const ta = document.createElement('textarea')
      ta.value = text
      ta.style.position = 'fixed'
      ta.style.opacity = '0'
      document.body.appendChild(ta)
      ta.select()
      document.execCommand('copy')
      document.body.removeChild(ta)
    },
    copyTreeAsText(index) {
      const tree = this.behTrees[index]
      const cache = this.behTreesCache[index]
      const hasResults = this.treeHasAnyResults(tree, cache)
      this.copyToClipboard(hasResults ? this.treeToText(tree, 0, true, cache).trimEnd() : 'EMPTY TREE DATA')
    },
    copySnapshotAsText(index) {
      const tree = this.snapshotTrees[index]
      const hasResults = this.treeHasAnyResults(tree, null)
      this.copyToClipboard(hasResults ? this.treeToText(tree, 0, true, null).trimEnd() : 'EMPTY TREE DATA')
    },
  },

  computed: {
    filteredLogs() {
      if (!this.logFilter) return this.botActionsHistory
      return this.botActionsHistory.filter(entry => {
        const eid = entry.data.eid
        return eid && String(eid).includes(this.logFilter)
      })
    }
  },

  template: `
    <div class="aiLogContainer">
      <div>
        <el-button size="mini" v-on:click="getBots()">Refresh bot list</el-button>
        <el-button size="mini" v-on:click="spectate(0)">Cancel spectating</el-button>
        <el-button size="mini" v-on:click="showActionsHistory = !showActionsHistory">{{ showActionsHistory ? 'Hide history' : 'Show history' }}</el-button>
        <ul>
          <li v-for="[eid, template, debug, track_mind] in botsList">
            <div class="templateName">{{ eid }}: {{ makeBreakable(template) }}</div>
            <el-button-group>
              <el-button size="mini" v-on:click="trackMind(eid, !track_mind)">{{ track_mind ? "Untrack" : "Track" }}</el-button>
              <el-button size="mini" v-on:click="setDebug(eid, !debug)">{{ debug ? "Undebug" : "Debug" }}</el-button>
              <el-button size="mini" v-on:click="spectate(eid)">Spectate</el-button>
              <el-button size="mini" v-on:click="requestBehTree(eid)" :disabled="behTreeEids.indexOf(eid) != -1">Beh tree</el-button>
            </el-button-group>
            <p v-if="eid in mindset">{{ mindset[eid] }}</p>
          </li>
        </ul>
      </div>
      <div v-for="(behTree, index) in behTrees">
        <div class="behTreeControl">
          <div>
            <el-button-group>
              <el-button size="mini" v-on:click="collapseTree('bt_' + behTreeEids[index], true)">Collapse All</el-button>
              <el-button size="mini" v-on:click="collapseTree('bt_' + behTreeEids[index], false)">Expand All</el-button>
              <el-button size="mini" v-on:click="resetTree(behTreeEids[index])">Reset info</el-button>
              <el-button size="mini" v-on:click="copyTreeAsText(index)">Copy text</el-button>
              <el-button size="mini" v-on:click="removeBehTree(behTreeEids[index])">X</el-button>
            </el-button-group>
            <span>eid:{{behTreeEids[index]}}</span>
          </div>
          <div>
            <el-button-group>
              <el-button size="mini" v-on:click="toggleBTEnabled(behTreeEids[index])">Toggle enabled</el-button>
              <el-button size="mini" v-on:click="resetBT(behTreeEids[index])">Reset tree</el-button>
              <el-button size="mini" v-on:click="destroyAgent(behTreeEids[index])">destroyEntity</el-button>
            </el-button-group>
          </div>
        </div>
        <tree :ref="behTreeRef(index)" :data="[behTree]" :options="treeOptions"
            v-on:node:unchecked="uncheckNode(behTreeEids[index], $event)"
            v-on:node:checked="checkNode(behTreeEids[index], $event)">
          <div slot-scope="{ node }" class="node-container">
            <div :class="getNodeClass(node)" style="max-width: 600px; overflow-wrap: break-word; display: flex; flex-wrap: wrap; user-select: text;">
              {{ node.text }} {{ getNodeStatus(node) }}{{ node.data.isReaction ? "(" + node.data.reactionName + ")" : "" }}
              {{ Object.keys(node.data.describe || {}).length > 0 ? " | " : "" }}
              <span v-for="it in node.data.describe">{{it[0]}}: {{it[1]}} | </span>
            </div>
          </div>
        </tree>
      </div>

      <div v-for="(snapshotTree, index) in snapshotTrees">
        <div class="behTreeControl" style="background: #fff3cd; border: 2px solid #ffc107;">
          <div>
            <el-button-group>
              <el-button size="mini" v-on:click="collapseTree('snapshot_' + snapshotTreeIds[index], true)">Collapse All</el-button>
              <el-button size="mini" v-on:click="collapseTree('snapshot_' + snapshotTreeIds[index], false)">Expand All</el-button>
              <el-button size="mini" v-on:click="copySnapshotAsText(index)">Copy text</el-button>
              <el-button size="mini" v-on:click="removeSnapshotTree(snapshotTreeIds[index])">X</el-button>
            </el-button-group>
            <span style="color: #856404; font-weight: bold;">SNAPSHOT</span>
            <span>eid:{{snapshotTree.snapshotEid}} @ time:{{snapshotTree.snapshotTime}}</span>
          </div>
        </div>
        <tree :ref="'snapshot_' + snapshotTreeIds[index]" :data="[snapshotTree]" :options="treeOptions">
          <div slot-scope="{ node }" class="node-container">
            <div :class="getNodeClass(node)" style="max-width: 600px; overflow-wrap: break-word; display: flex; flex-wrap: wrap; user-select: text;">
              {{ node.text }} {{ getNodeStatus(node) }}{{ node.data.isReaction ? "(" + node.data.reactionName + ")" : "" }}
              {{ Object.keys(node.data.describe || {}).length > 0 ? " | " : "" }}
              <span v-for="it in node.data.describe">{{it[0]}}: {{it[1]}} | </span>
            </div>
          </div>
        </tree>
      </div>

      <div v-if="showActionsHistory" style="margin-top: 20px; border-top: 2px solid #ccc; padding-top: 10px;">
        <h3>JSON Log
          <el-button size="mini" v-on:click="clearJsonLog()">Clear</el-button>
          <el-input v-model="logFilter" placeholder="Filter by id" size="mini" style="width: 200px; margin-left: 10px;"></el-input>
          <span style="margin-left: 10px; color: #666;">Records: {{ filteredLogs.length }}/{{ botActionsHistory.length }}</span>
          <span style="margin-left: 10px;">History size:</span>
          <el-input-number v-model="historySize" :min="10" :max="1000" size="mini" style="width: 120px; margin-left: 5px;"></el-input-number>
        </h3>
        <div style="max-height: 400px; overflow-y: auto; background: #f5f5f5; padding: 10px; font-family: monospace; font-size: 12px;">
          <div v-for="(entry, idx) in filteredLogs" :key="idx" style="margin-bottom: 10px; border-bottom: 1px solid #ddd; padding-bottom: 5px;">
            <div style="color: #666; font-weight: bold;">
              [{{ entry.timestamp }}] eid: {{ entry.data.eid }}
              <el-button size="mini" v-on:click="showSnapshotTree(entry)" style="margin-left: 10px;">Show</el-button>
              <el-button size="mini" v-on:click="toggleJsonView(idx)" style="margin-left: 5px;">{{ expandedJsonEntries[idx] ? 'Hide JSON' : 'Show JSON' }}</el-button>
              <el-button size="mini" v-on:click="copyJsonToClipboard(entry)" style="margin-left: 5px;" v-if="expandedJsonEntries[idx]">Copy</el-button>
            </div>
            <pre v-if="expandedJsonEntries[idx]" style="background: white; padding: 5px; margin: 5px 0; overflow-x: auto; max-height: 300px; overflow-y: auto; border: 1px solid #e0e0e0;">{{ JSON.stringify(entry.data, null, 2) }}</pre>
          </div>
          <div v-if="botActionsHistory.length === 0" style="color: #999; text-align: center; padding: 20px;">
             log empty
          </div>
        </div>
      </div>
    </div>
  `
})


function transform(source) {
  var res = source;
  if ("id" in source) { //means it is a wrapper, the actual node = source.children[0]
    let name = source.children[0].name || source.children[0].typeName
    let isReaction = source.children[0].isReaction
    let reactionName = source.children[0].reactionName
    let describe = source.children[0].describe
    let prefix = name + (NODE_SYMBOLS[source.children[0].typeName] ?? "")
    res = Object.assign({}, source.children[0], {
      text: prefix,
      state: {
        checked: source.forceResult == -1
      },
      data: {
        id: source.id,
        prefix: prefix,
        startTime: -1,
        lastVisit: -1,
        lastUpdateResult: -1,
        lastReactResult: -1,
        isLastRunning: false,
        isReaction: isReaction,
        reactionName: reactionName,
        describe: describe,
        forceResult: source.forceResult,
      },
    })
  }
  if (res.children && res.children.length > 0) {
    var newChildren = []
    for (child of res.children) {
      newChildren.push(transform(child))
    }
    res.children = newChildren
  }
  return res
}
