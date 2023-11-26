<script setup>
const { ref, computed, watch, defineProps } = Vue;
const { sortValues } = Vue.helpers;
const { ecsService } = Vue.services;

const props = defineProps({
  events: Array,
});

const curEventId = ref(-1);

const curEvent = computed(() => {
  return curEventId.value >= 0 ? props.events[curEventId.value] : null;
});

watch(curEvent, (entity) => {
  console.log('curEvent', entity);
  if (entity) {
    Vue.myApp.$bvModal.show('modal-event-details');
  } else {
    Vue.myApp.$bvModal.hide('modal-event-details');
  }
});

function openRow(row) {
  console.log('openRow', row);

  curEventId.value = row.index;
}

function onCloseRow() {
  console.log('onCloseRow');
  curEventId.value = -1;
}
</script>

<style></style>

<template>
  <b-modal id="modal-event-details" centered size="lg" ok-only @hidden="onCloseRow">
    <template #modal-title>
      <h5>{{ curEvent.name }}</h5>
    </template>

    <div class="d-flex flex-row">
      <div>
        <strong class="d-block">Systems</strong>
        <table class="table table-sm table-bordered table-hover" style="table-layout: fixed;">
          <thead>
            <th>Name</th>
          </thead>
          <tbody>
            <tr v-for="s in curEvent._systems">
              <td>
                <div class="text-truncate"><span class="ecs-info">{{ s.tags }}</span>{{ s.name }}</div>
              </td>
            </tr>
          </tbody>
        </table>
      </div>
    </div>
  </b-modal>

  <div class="container-fluid m-0 border-bottom" style="padding-right: 32px;">
    <div class="row">
      <SortableColumn @sort="sortValues(events, 'name')" columnTitle="Name" columnClass="col-sm-10 p-0" />
      <SortableColumn @sort="sortValues(events, 'size')" columnTitle="Size" columnClass="col-sm-1 p-0 text-center" />
      <SortableColumn @sort="sortValues(events, 'systemsCount', v => v._systems.length)" columnTitle="Systems count"
        columnClass="col-sm-1 p-0 text-center" />
    </div>
  </div>

  <RecycleScroller class="scroller" :items="events" :item-size="32" key-field="id">
    <template #default="props">
      <div
        :class="['container-fluid', 'm-0', 'scroller-row', 'border-bottom', props.index % 2 == 0 ? 'scroller-row-even' : 'scroller-row-odd']">
        <div class="row clickable" @click="openRow(props)">
          <div class="col-sm-10 p-0">{{ props.item.name }}</div>
          <div class="col-sm-1 p-0 text-center">{{ props.item.size }}</div>
          <div class="col-sm-1 p-0 text-center">{{ props.item._systems.length }}</div>
        </div>
      </div>
    </template>
  </RecycleScroller>
</template>