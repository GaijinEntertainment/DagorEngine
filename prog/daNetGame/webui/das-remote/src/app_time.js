Vue.component('app_time', {
  data: function () {
    return {
      timespeed: -1,
      presets: [0, 0.01, 0.1, 0.25, 0.5, 1, 2, 4, 20]
    }
  },
  mounted() {
    this.getTimespeed()
  },
  methods: {
    getTimespeed() {
      jrpc.call('app.get_timespeed').then(data => this.timespeed = data)
    },
    setTimespeed(speed) {
      jrpc.call('app.set_timespeed', speed).then(data => this.timespeed = data)
    },
    togglePause() {
      jrpc.call('app.toggle_pause').then(data => this.timespeed = data)
    },
  },
  template: `
    <div>
      <span style="cursor: pointer;" v-on:click="getTimespeed()">Timespeed: {{ timespeed.toFixed(2) }}</span>
      <el-button size="mini" v-on:click="togglePause()">Pause</el-button>
      <el-button-group>
        <el-button size="mini" v-for="speed in presets" v-on:click="setTimespeed(speed)" v-bind:key="speed">
          {{speed + "x" }}
        </el-button>
      </el-button-group>
    </div>
  `
})
