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
      }
    }
  },
  mounted() {
    jrpc.on("ai_log.mind", (eid, data) => this.$set(this.mindset, eid, data))
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
      }
    },
    requestBehTree(eid) {
      jrpc.notification('ai_log.set_beh_tree_debug', [eid, true])
      setTimeout(() => jrpc.call('ai_log.get_beh_tree', eid).then((data) => {
        this.updateBehTree(eid, transform(data))
      }), 100)
    },
    collapseTree(eid, value) {
      let etree = this.$refs["bt_" + eid][0]
      let children = etree.findAll({}, true)
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
      let etree = this.$refs["bt_" + eid][0]
      let children = etree.findAll({}, true)
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
    }
  },
  template: `
    <div class="aiLogContainer">
      <div>
        <el-button size="mini" v-on:click="getBots()">Refresh bot list</el-button>
        <el-button size="mini" v-on:click="spectate(0)">Cancel spectating</el-button>
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
              <el-button size="mini" v-on:click="collapseTree(behTreeEids[index], true)">Collapse All</el-button>
              <el-button size="mini" v-on:click="collapseTree(behTreeEids[index], false)">Expand All</el-button>
              <el-button size="mini" v-on:click="resetTree(behTreeEids[index])">Reset info</el-button>
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
            <div :class="getNodeClass(node)" style="max-width: 600px; overflow-wrap: break-word; display: flex; flex-wrap: wrap;">
              {{ node.text }} {{ getNodeStatus(node) }}{{ node.data.isReaction ? "(" + node.data.reactionName + ")" : "" }}
              {{ Object.keys(node.data.describe || {}).length > 0 ? " | " : "" }}
              <span v-for="it in node.data.describe">{{it[0]}}: {{it[1]}} | </span>
            </div>
          </div>
        </tree>
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
