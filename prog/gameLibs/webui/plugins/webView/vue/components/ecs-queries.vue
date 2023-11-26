<script setup>
const { ref, computed, watch, defineProps } = Vue;
const { sortValues } = Vue.helpers;
const { ecsService } = Vue.services;

const props = defineProps({
  queries: Array,
});

const curQueryId = ref(-1);

const curQuery = computed(() => {
  return curQueryId.value >= 0 ? props.queries[curQueryId.value] : null;
});

watch(curQuery, (entity) => {
  console.log('curQuery', entity);
  if (entity) {
    Vue.myApp.$bvModal.show('modal-query-details');
  } else {
    Vue.myApp.$bvModal.hide('modal-query-details');
  }
});

function openRow(row) {
  console.log('openRow', row);

  curQueryId.value = row.index;
}

function onCloseRow() {
  console.log('onCloseRow');
  curQueryId.value = -1;
}
</script>

<style></style>

<template>
  <b-modal id="modal-query-details" centered size="lg" ok-only @hidden="onCloseRow">
    <template #modal-title>
      <h5>{{ curQuery.name }}</h5>
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
            <tr v-if="curQuery.componentsRW.length > 0">
              <td colspan="2"><strong>Read-Write</strong></td>
            </tr>
            <tr v-for="c in curQuery.componentsRW">
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
            <tr v-if="curQuery.componentsRO.length > 0">
              <td colspan="2"><strong>Read-Only</strong></td>
            </tr>
            <tr v-for="c in curQuery.componentsRO">
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
            <tr v-if="curQuery.componentsRQ.length > 0">
              <td colspan="2"><strong>Required</strong></td>
            </tr>
            <tr v-for="c in curQuery.componentsRQ">
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
            <tr v-if="curQuery.componentsNO.length > 0">
              <td colspan="2"><strong>Required Not</strong></td>
            </tr>
            <tr v-for="c in curQuery.componentsNO">
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
            <tr v-for="t in curQuery._templateNames">
              <td>
                <div class="text-nowrap text-truncate" :title="t">{{ t }}</div>
              </td>
              <td>
                <div class="text-nowrap text-truncate" :title="curQuery._templates[t]">{{ curQuery._templates[t] }}
                </div>
              </td>
            </tr>
          </tbody>
        </table>
      </div>
    </div>
  </b-modal>

  <div class="container-fluid m-0 border-bottom" style="padding-right: 32px;">
    <div class="row">
      <SortableColumn @sort="sortValues(queries, 'name')" columnTitle="Name" columnClass="col-sm-9 p-0" />
      <SortableColumn @sort="sortValues(queries, 'entitiesCount')" columnTitle="Entities count"
        columnClass="col-sm-1 p-0 text-center" />
      <SortableColumn
        @sort="sortValues(queries, 'eventsCount', v => v.componentsRW.length + v.componentsRO.length + v.componentsRQ.length + v.componentsNO.length)"
        columnTitle="Components count" columnClass="col-sm-2 p-0 text-center" />
    </div>
  </div>

  <RecycleScroller class="scroller" :items="queries" :item-size="32" key-field="id">
    <template #default="props">
      <div
        :class="['container-fluid', 'm-0', 'scroller-row', 'border-bottom', props.index % 2 == 0 ? 'scroller-row-even' : 'scroller-row-odd']">
        <div class="row clickable" @click="openRow(props)">
          <div class="col-sm-9 p-0">{{ props.item.name }}</div>
          <div class="col-sm-1 p-0 text-center">{{ props.item.entitiesCount }}</div>
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