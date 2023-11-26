import { streamService } from './core/services.js';
import { createMyApp, app, status, SortableColumn } from './app.js';

export function main() {
  Vue.component('vue-bootstrap-typeahead', VueBootstrapTypeahead);
  Vue.component('RecycleScroller', VueVirtualScroller.RecycleScroller);
  Vue.component('color-picker', VColor);
  Vue.component('vue-multiselect', VueMultiselect.Multiselect);
  Vue.component('SortableColumn', SortableColumn);

  createMyApp();

  console.log('App', app);
  console.log('App', app.$bvModal);
  console.log('Vue', Vue.$bvModal);

  setInterval(() => {
    let prevStatus = status;
    let curStatus = streamService.isConnected() ? "Online" : "Offline";
    if (prevStatus !== curStatus) {
      status.value = curStatus;
    }
  }, 500);
}
