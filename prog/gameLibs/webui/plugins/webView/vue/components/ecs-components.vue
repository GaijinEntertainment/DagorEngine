<script setup>
const { ref, computed, watch, defineProps } = Vue;
const { sortValues } = Vue.helpers;
const { ecsService } = Vue.services;

const props = defineProps({
  components: Array,
});

const curComponentId = ref(-1);

const curComponent = computed(() => {
  return curComponentId.value >= 0 ? props.components[curComponentId.value] : null;
});

watch(curComponent, (entity) => {
  console.log('curComponent', entity);
  if (entity) {
    Vue.myApp.$bvModal.show('modal-component');
  } else {
    Vue.myApp.$bvModal.hide('modal-component');
  }
});

function openRow(row) {
  console.log('openRow', row);

  curComponentId.value = row.index;
}

function onCloseRow() {
  console.log('onCloseRow');
  curComponentId.value = -1;
}
</script>

<style></style>

<template>
  <b-modal id="modal-component" centered size="lg" ok-only @hidden="onCloseRow">
    <template #modal-title>
      <h5>{{ curComponent.name }}</h5>
    </template>

    <div class="d-flex flex-row">
      <div>
        <strong class="d-block">Templates</strong>
        <table class="table table-sm table-bordered table-hover" style="table-layout: fixed;">
          <colgroup>
            <col style="width: 50%;" />
            <col style="width: 50%;" />
          </colgroup>
          <thead>
            <th>In Template</th>
            <th>From Template</th>
          </thead>
          <tbody>
            <tr v-for="t in curComponent._templatesFn()">
              <td>
                <div class="text-nowrap text-truncate" :title="ecsService.templates[t.templateId].name">{{
                  ecsService.templates[t.templateId].name }}</div>
              </td>
              <td>
                <div class="text-nowrap text-truncate" :title="ecsService.templates[t.sourceTemplateId].name">{{
                  ecsService.templates[t.sourceTemplateId].name }}</div>
              </td>
            </tr>
          </tbody>
        </table>
      </div>

      <div class="d-flex flex-column">
        <div class="ml-3">
          <strong class="d-block">Systems</strong>
          <table class="table table-sm table-bordered table-hover">
            <thead>
              <th>Name</th>
            </thead>
            <tbody>
              <tr v-for="s in curComponent._systems">
                <td>
                  <div class="text-truncate pb-1" :title="s.system.name"><span class="ecs-info"> {{ s.mode }}</span>{{
                    s.system.name }}</div>
                </td>
              </tr>
            </tbody>
          </table>
        </div>

        <div class="ml-3">
          <strong class="d-block">Queries</strong>
          <table class="table table-sm table-bordered table-hover">
            <thead>
              <th>Name</th>
            </thead>
            <tbody>
              <tr v-for="q in curComponent._queries">
                <td>
                  <div class="text-truncate pb-1" :title="q.query.name"><span class="ecs-info"> {{ q.mode }}</span>{{
                    q.query.name }}</div>
                </td>
              </tr>
            </tbody>
          </table>
        </div>
      </div>
    </div>
  </b-modal>

  <div class="container-fluid m-0 border-bottom" style="padding-right: 32px;">
    <div class="row">
      <SortableColumn @sort="sortValues(components, 'name')" columnTitle="Name" columnClass="col-sm-5 p-0"/>
      <SortableColumn @sort="sortValues(components, 'type')" columnTitle="Type" columnClass="col-sm-4 p-0"/>
      <SortableColumn @sort="sortValues(components, 'size')" columnTitle="Size" columnClass="col-sm-1 p-0 text-center"/>
      <SortableColumn @sort="sortValues(components, 'systemsCount', v => v._systems.length)" columnTitle="Systems count" columnClass="col-sm-1 p-0 text-center"/>
      <SortableColumn @sort="sortValues(components, 'queriesCount', v => v._queries.length)" columnTitle="Queries count" columnClass="col-sm-1 p-0 text-center"/>
    </div>
  </div>

  <RecycleScroller class="scroller" :items="components" :item-size="32" key-field="id">
    <template #default="props">
      <div
        :class="['container-fluid', 'm-0', 'scroller-row', 'border-bottom', props.index % 2 == 0 ? 'scroller-row-even' : 'scroller-row-odd']">
        <div class="row clickable" @click="openRow(props)">
          <div class="col-sm-5 p-0">{{ props.item.name }}</div>
          <div class="col-sm-4 p-0">{{ props.item.type }}</div>
          <div class="col-sm-1 p-0 text-center">{{ props.item.size }}</div>
          <div class="col-sm-1 p-0 text-center">{{ props.item._systems.length }}</div>
          <div class="col-sm-1 p-0 text-center">{{ props.item._queries.length }}</div>
        </div>
      </div>
    </template>
  </RecycleScroller>
</template>