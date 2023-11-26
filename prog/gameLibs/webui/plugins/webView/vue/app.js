const { createApp, ref, reactive, computed, defineComponent, defineAsyncComponent } = Vue;

import helpers from "./core/helpers.js";
import services from "./core/services.js";

const plugins = {
  install() {
    Vue.helpers = helpers;
    Vue.services = services;
    Vue.prototype.$helpers = helpers;
    Vue.prototype.$services = services;
  }
}
Vue.use(plugins);

export const status = ref("Offline");
export const statusClass = computed(() => {
  return status.value.toLowerCase() !== 'online' ? 'offline' : 'online';
});

export const SortableColumn = defineComponent({
  props: {
    columnTitle: String,
    columnClass: String | Object,
    columnStyle: String | Object,
  },
  setup(props) {
    const sorted = ref('');

    function sortValues() {
      if (sorted.value === 'up') {
        sorted.value = 'down';
      } else if (sorted.value === 'down') {
        sorted.value = '';
      } else {
        sorted.value = 'up';
      }
      this.$emit('sort');
    }

    return {
      sorted,
      sortValues,
    }
  },
  template: `
    <div class="clickable" :class="columnClass" :style="columnStyle" @click="sortValues()">
      <b>{{ columnTitle }}</b>
      <i v-if="sorted === 'up'" class="fa fa-arrow-up"></i>
      <i v-if="sorted === 'down'" class="fa fa-arrow-down"></i>
    </div>
  `
});

const MyApp = {
  setup() {
    return {
      status,
      statusClass,
    }
  },
  components: {
    'editor': defineAsyncComponent(() => Vue.helpers.loadModule('./vue/components/editor.vue'))
  }
}

export var app = null;

export function createMyApp() {
  app = createApp(MyApp).mount('#app');

  Vue.myApp = app;
}