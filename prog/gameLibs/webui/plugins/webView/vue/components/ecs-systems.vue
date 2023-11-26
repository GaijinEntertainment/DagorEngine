<script setup>
const { ref, computed, watch, defineProps } = Vue;
const { sortValues } = Vue.helpers;
const { ecsService } = Vue.services;

const props = defineProps({
  systems: Array,
});

const curSystemId = ref(-1);

const curSystem = computed(() => {
  return curSystemId.value >= 0 ? props.systems[curSystemId.value] : null;
});

watch(curSystem, (entity) => {
  console.log('curSystem', entity);
  if (entity) {
    Vue.myApp.$bvModal.show('modal-system-details');
  } else {
    Vue.myApp.$bvModal.hide('modal-system-details');
  }
});

function openRow(row) {
  console.log('openRow', row);

  curSystemId.value = row.index;
}

function onCloseRow() {
  console.log('onCloseRow');
  curSystemId.value = -1;
}
</script>

<style></style>

<template>
  <b-modal id="modal-system-details" centered size="lg" ok-only @hidden="onCloseRow">
    <template #modal-title>
      <h5>{{ curSystem.name }}</h5>
    </template>

    <div class="d-flex flex-row">
      <div>
        <strong class="d-block">Components</strong>
        <table class="table table-sm table-bordered table-hover" style="table-layout: fixed;">
          <thead>
            <th>Name</th>
            <th>Type</th>
          </thead>
          <tbody>
            <tr v-if="curSystem.componentsRW.length > 0">
              <td colspan="2"><strong>Read-Write</strong></td>
            </tr>
            <tr v-for="c in curSystem.componentsRW">
              <td>
                <div class="text-nowrap text-truncate" :title="c.name" :class="{ 'pb-1': c.optional }">
                  <span v-if="c.optional" class="ecs-info">optional</span>
                  {{ c.name }}
                </div>
              </td>
              <td>
                <div class="text-nowrap text-truncate" :title="c.type">{{ c.type }}</div>
              </td>
            </tr>
            <tr v-if="curSystem.componentsRO.length > 0">
              <td colspan="2"><strong>Read-Only</strong></td>
            </tr>
            <tr v-for="c in curSystem.componentsRO">
              <td>
                <div class="text-nowrap text-truncate" :title="c.name" :class="{ 'pb-1': c.optional }">
                  <span v-if="c.optional" class="ecs-info">optional</span>
                  {{ c.name }}
                </div>
              </td>
              <td>
                <div class="text-nowrap text-truncate" :title="c.type">{{ c.type }}</div>
              </td>
            </tr>
            <tr v-if="curSystem.componentsRQ.length > 0">
              <td colspan="2"><strong>Required</strong></td>
            </tr>
            <tr v-for="c in curSystem.componentsRQ">
              <td>
                <div class="text-nowrap text-truncate" :title="c.name" :class="{ 'pb-1': c.optional }">
                  <span v-if="c.optional" class="ecs-info">optional</span>
                  {{ c.name }}
                </div>
              </td>
              <td>
                <div class="text-nowrap text-truncate" :title="c.type">{{ c.type }}</div>
              </td>
            </tr>
            <tr v-if="curSystem.componentsNO.length > 0">
              <td colspan="2"><strong>Required Not</strong></td>
            </tr>
            <tr v-for="c in curSystem.componentsNO">
              <td>
                <div class="text-nowrap text-truncate" :title="c.name" :class="{ 'pb-1': c.optional }">
                  <span v-if="c.optional" class="ecs-info">optional</span>
                  {{ c.name }}
                </div>
              </td>
              <td>
                <div class="text-nowrap text-truncate" :title="c.type">{{ c.type }}</div>
              </td>
            </tr>
          </tbody>
        </table>
      </div>

      <div class="ml-3">
        <strong class="d-block">Templates</strong>
        <table class="table table-sm table-bordered table-hover" style="table-layout: fixed;">
          <thead>
            <th>Name</th>
            <th>Count</th>
          </thead>
          <tbody>
            <tr v-for="t in curSystem._templateNames">
              <td>
                <div class="text-nowrap text-truncate" :title="t">{{ t }}</div>
              </td>
              <td>
                <div class="text-nowrap text-truncate" :title="curSystem._templates[t]">{{ curSystem._templates[t] }}
                </div>
              </td>
            </tr>
          </tbody>
        </table>
      </div>

      <div class="ml-3">
        <strong class="d-block">Events</strong>
        <table class="table table-sm table-bordered table-hover" style="table-layout: fixed;">
          <thead>
            <th>Name</th>
          </thead>
          <tbody>
            <tr v-for="e in curSystem.events">
              <td>
                <div class="text-nowrap text-truncate" :title="e">{{ e }}</div>
              </td>
            </tr>
          </tbody>
        </table>
      </div>
    </div>
  </b-modal>

  <div class="container-fluid m-0 border-bottom" style="padding-right: 32px;">
    <div class="row">
      <SortableColumn @sort="sortValues(systems, 'name')" columnTitle="Name" columnClass="col-sm-8 p-0" />
      <SortableColumn @sort="sortValues(systems, 'entitiesCount')" columnTitle="Entities count"
        columnClass="col-sm-1 p-0 text-center" />
      <SortableColumn @sort="sortValues(systems, 'eventsCount', v => v.events.length)" columnTitle="Events count"
        columnClass="col-sm-1 p-0 text-center" />
      <SortableColumn
        @sort="sortValues(systems, 'eventsCount', v => v.componentsRW.length + v.componentsRO.length + v.componentsRQ.length + v.componentsNO.length)"
        columnTitle="Components count" columnClass="col-sm-2 p-0 text-center" />
    </div>
  </div>

  <RecycleScroller class="scroller" :items="systems" :item-size="32" key-field="id">
    <template #default="props">
      <div
        :class="['container-fluid', 'm-0', 'scroller-row', 'border-bottom', props.index % 2 == 0 ? 'scroller-row-even' : 'scroller-row-odd']">
        <div class="row clickable" @click="openRow(props)">
          <div class="col-sm-8 p-0">{{ props.item.name }}</div>
          <div class="col-sm-1 p-0 text-center">{{ props.item.entitiesCount }}</div>
          <div class="col-sm-1 p-0 text-center">{{ props.item.events.length }}</div>
          <div class="col-sm-2 p-0 text-center">
            <div class="container-fluid">
              <div class="row">
                <div class="col-sm p-0 text-left"><b>RW:</b> {{ props.item.componentsRW.length }}</div>
                <div class="col-sm p-0 text-left"><b>RO:</b> {{ props.item.componentsRO.length }}</div>
                <div class="col-sm p-0 text-left"><b>RQ:</b> {{ props.item.componentsRQ.length }}</div>
                <div class="col-sm p-0 text-left"><b>NO:</b> {{ props.item.componentsNO.length }}</div>
              </div>
            </div>
          </div>
        </div>
      </div>
    </template>
  </RecycleScroller>
</template>