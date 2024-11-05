Vue.component('das_compile_errors_count', {
  data: function () {
    return {
      errorsCount: -2,
    }
  },
  mounted() {
    this.getErrorsCount()
  },
  methods: {
    getErrorsCount() {
      let me = this
      jrpc.call('das.compile_errors_count').then(function (data) { me.errorsCount = data })
    }
  },
  template: `
    <div style="cursor: pointer;" v-on:click="getErrorsCount()">
      Dascript errors: {{ errorsCount == -2 ? "n/a" : errorsCount == -1 ? "unknown" : errorsCount }}
    </div>
  `
})
