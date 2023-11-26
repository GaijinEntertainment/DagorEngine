<script setup>
const { ref, computed, watch, defineProps, defineComponent } = Vue;
const { formatValue, compToString, shouldOpenEditor, getAttrValue, startEdit, cancelEdit, saveEdit, updateValue, sortValues } = Vue.helpers;
const { ecsService } = Vue.services;

const props = defineProps({
  entities: Array,
});

const SaveEditButton = defineComponent({
  props: {
  },
  emit: ['clickSave', 'clickCancel'],
  methods: {
    clickSave() {
      this.$emit('clickSave');
    },
    clickCancel() {
      this.$emit('clickCancel');
    },
  },
  template: `
    <div class="col-sm p-0 d-flex d-flex-row justify-content-around align-items-center">
      <button type="button" class="btn btn-sm btn-outline-primary" @click="clickSave()"><i
          class="fa fa-check" aria-hidden="true"></i></button>
      <button type="button" class="btn btn-sm btn-outline-danger" @click="clickCancel()"><i
          class="fa fa-times" aria-hidden="true"></i></button>
    </div>
  `,
});

const curEntityId = ref(-1);
const curSystem = ref(null);
const curQuery = ref(null);
const curEntityComponent = ref(null);

const curEntity = computed(() => {
  return curEntityId.value >= 0 ? props.entities[curEntityId.value] : null;
});

watch(curEntity, (entity) => {
  console.log('curEntity', entity);
  if (entity) {
    Vue.myApp.$bvModal.show('modal-entity');
  } else {
    Vue.myApp.$bvModal.hide('modal-entity');
  }
});

const curCompJson = computed({
  get() { return compToString(curEntityComponent.value.value, true); },
  set(val) {
    try {
      const jsonVal = JSON.parse(val);
      updateValue({}, curEntity, jsonVal, curEntityComponent.value);
    } catch (e) {
    }
  }
});

function openRow(row) {
  console.log('openRow', row);
  ecsService.fetchEntityAttributes(row.item.eid).then((entity) => {
    console.log('onFetchEntityAttributes', entity);
    curEntityId.value = row.index;
  });
}

function onCloseRow() {
  console.log('onCloseRow');
  curEntityId.value = -1;
}

function clickSystem(sys) {
  console.log('clickSystem', sys);
  curSystem.value = sys;
  Vue.myApp.$bvModal.show('modal-system');
}

function clickQuery(q) {
  console.log('clickQuery', q);
  curQuery.value = q;
  Vue.myApp.$bvModal.show('modal-query');
}

function openAttr(row, attr, content) {
  if (shouldOpenEditor(attr) || attr.type === 'E3DCOLOR') {
    curEntityComponent.value = attr;
    Vue.myApp.$bvModal.show('modal-components-values');
  }
}
</script>

<style></style>

<template>
  <b-modal id="modal-system" centered size="lg" ok-only>
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

  <b-modal id="modal-query" centered size="lg" ok-only>
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

  <b-modal id="modal-components-values" centered size="lg" @ok="saveEdit(curEntity, curEntityComponent)">
    <template #modal-title>
      <h5>{{ curEntityComponent.name }}</h5>
    </template>

    <textarea class="form-control form-control" style="height: 400px;" v-model="curCompJson"></textarea>

    <template #modal-footer="{ ok, cancel, hide }">
      <b-button size="sm" variant="success" @click="ok()">
        Save
      </b-button>
      <b-button size="sm" variant="danger" @click="cancel()">
        Cancel
      </b-button>
    </template>
  </b-modal>

  <b-modal id="modal-entity" centered size="lg" ok-only @hidden="onCloseRow">
    <template #modal-title>
      <h5>{{ curEntity.eid }}: {{ curEntity.template }}</h5>
    </template>
    <div class="d-flex flex-row">
      <div style="min-width: 800px;">
        <strong class="d-block">Components ({{ curEntity.components.length }})</strong>
        <table class="table table-sm table-bordered table-hover" style="table-layout: fixed;">
          <colgroup>
            <col style="width: 55%;" />
            <col style="width: 15%;" />
            <col style="width: 30%;" />
          </colgroup>
          <thead>
            <th>Name</th>
            <th>Type</th>
            <th>Value</th>
          </thead>
          <tbody>
            <tr v-for="attr in curEntity.components">
              <td>
                <div class="text-nowrap text-truncate"
                  :class="{ 'pb-1': attr.isTracked || attr.isReplicated || attr.isDontReplicate }">
                  <button type="button" class="btn btn-sm btn-outline-primary btn-in-components-row mr-1"
                    @click="ecsService.fetchEntityAttributes(curEntity.eid);"><i class="fa fa-refresh"
                      aria-hidden="true"></i></button>
                  <span v-if="attr.isTracked || attr.isReplicated || attr.isDontReplicate" class="ecs-info">
                    <span v-if="attr.isTracked" class="pr-1">tracked</span>
                    <span v-if="attr.isReplicated" class="pr-1">replicated</span>
                    <span v-if="attr.isDontReplicate" class="pr-1">do not replicate</span>
                  </span>
                  <span :title="attr.name">{{ attr.name }}</span>
                </div>
              </td>
              <template v-if="shouldOpenEditor(attr)">
                <td>
                  <div class="text-truncate"><span :title="attr.type">{{ attr.type }}</span></div>
                </td>
                <td class="clickable" @click="openAttr(curEntity, attr, ecsAttribute)">[Click to see value]</td>
              </template>
              <template v-if="!shouldOpenEditor(attr)">
                <td>
                  <div class="text-truncate"><span :title="attr.type">{{ attr.type }}</span></div>
                </td>
                <td>
                  <div class="contrainer" v-if="attr._edit">
                    <div class="row m-0">
                      <color-picker v-if="attr.type === 'E3DCOLOR'" :color="attr._color"
                        @change="updateValue($event, curEntity, $event.rgba, attr)"></color-picker>
                      <template v-else-if="attr.type === 'bool'">
                        <div class="col-sm p-0">
                          <input autofocus class="form-control form-control-sm form-check-input" v-model="attr.value"
                            type="checkbox" @change="updateValue($event, curEntity, $event.target.checked, attr)" />
                        </div>
                      </template>
                      <template v-else-if="attr.type === 'float'">
                        <div class="col-sm p-0">
                          <input autofocus class="form-control form-control-sm" v-model="attr.value.r"
                            @keyup="updateValue($event, curEntity, $event.target.value, attr, 'r')" />
                        </div>
                      </template>
                      <template v-else-if="attr.type === 'ecs::EntityId'">
                        <div class="col-sm p-0">
                          <input autofocus class="form-control form-control-sm" v-model="attr.value.eid"
                            @keyup="updateValue($event, curEntity, $event.target.value, attr, 'eid')" />
                        </div>
                      </template>
                      <template v-else-if="attr.type === 'int'">
                        <div class="col-sm p-0">
                          <input autofocus class="form-control form-control-sm" v-model="attr.value.i"
                            @keyup="updateValue($event, curEntity, $event.target.value, attr, 'i')" />
                        </div>
                      </template>
                      <template v-else-if="attr.type === 'Point2'">
                        <div class="col-sm p-0">
                          <input autofocus class="form-control form-control-sm" v-model="attr.value.x"
                            @keyup="updateValue($event, curEntity, $event.target.value, attr, 'x')" />
                        </div>
                        <div class="col-sm p-0">
                          <input autofocus class="form-control form-control-sm" v-model="attr.value.y"
                            @keyup="updateValue($event, curEntity, $event.target.value, attr, 'y')" />
                        </div>
                      </template>
                      <template v-else-if="attr.type === 'Point3'">
                        <div class="col-sm p-0">
                          <input autofocus class="form-control form-control-sm" v-model="attr.value.x"
                            @keyup="updateValue($event, curEntity, $event.target.value, attr, 'x')" />
                        </div>
                        <div class="col-sm p-0">
                          <input autofocus class="form-control form-control-sm" v-model="attr.value.y"
                            @keyup="updateValue($event, curEntity, $event.target.value, attr, 'y')" />
                        </div>
                        <div class="col-sm p-0">
                          <input autofocus class="form-control form-control-sm" v-model="attr.value.z"
                            @keyup="updateValue($event, curEntity, $event.target.value, attr, 'z')" />
                        </div>
                      </template>
                      <template v-else-if="attr.type === 'Point4'">
                        <div class="col-sm p-0">
                          <input autofocus class="form-control form-control-sm" v-model="attr.value.x"
                            @keyup="updateValue($event, curEntity, $event.target.value, attr, 'x')" />
                        </div>
                        <div class="col-sm p-0">
                          <input autofocus class="form-control form-control-sm" v-model="attr.value.y"
                            @keyup="updateValue($event, curEntity, $event.target.value, attr, 'y')" />
                        </div>
                        <div class="col-sm p-0">
                          <input autofocus class="form-control form-control-sm" v-model="attr.value.z"
                            @keyup="updateValue($event, curEntity, $event.target.value, attr, 'z')" />
                        </div>
                        <div class="col-sm p-0">
                          <input autofocus class="form-control form-control-sm" v-model="attr.value.w"
                            @keyup="updateValue($event, curEntity, $event.target.value, attr, 'w')" />
                        </div>
                      </template>
                      <template v-else-if="attr.type === 'TMatrix'">
                        <div class="col-sm p-0">
                          <div class="row m-0">
                            <div class="col-sm p-0">
                              <input autofocus class="form-control form-control-sm" v-model="attr.value[0]"
                                @keyup="updateValue($event, curEntity, $event.target.value, attr, 0)" />
                            </div>
                            <div class="col-sm p-0">
                              <input autofocus class="form-control form-control-sm" v-model="attr.value[1]"
                                @keyup="updateValue($event, curEntity, $event.target.value, attr, 1)" />
                            </div>
                            <div class="col-sm p-0">
                              <input autofocus class="form-control form-control-sm" v-model="attr.value[2]"
                                @keyup="updateValue($event, curEntity, $event.target.value, attr, 2)" />
                            </div>
                          </div>
                          <div class="row m-0">
                            <div class="col-sm p-0">
                              <input autofocus class="form-control form-control-sm" v-model="attr.value[3]"
                                @keyup="updateValue($event, curEntity, $event.target.value, attr, 3)" />
                            </div>
                            <div class="col-sm p-0">
                              <input autofocus class="form-control form-control-sm" v-model="attr.value[4]"
                                @keyup="updateValue($event, curEntity, $event.target.value, attr, 4)" />
                            </div>
                            <div class="col-sm p-0">
                              <input autofocus class="form-control form-control-sm" v-model="attr.value[5]"
                                @keyup="updateValue($event, curEntity, $event.target.value, attr, 5)" />
                            </div>
                          </div>
                          <div class="row m-0">
                            <div class="col-sm p-0">
                              <input autofocus class="form-control form-control-sm" v-model="attr.value[6]"
                                @keyup="updateValue($event, curEntity, $event.target.value, attr, 6)" />
                            </div>
                            <div class="col-sm p-0">
                              <input autofocus class="form-control form-control-sm" v-model="attr.value[7]"
                                @keyup="updateValue($event, curEntity, $event.target.value, attr, 7)" />
                            </div>
                            <div class="col-sm p-0">
                              <input autofocus class="form-control form-control-sm" v-model="attr.value[8]"
                                @keyup="updateValue($event, curEntity, $event.target.value, attr, 8)" />
                            </div>
                          </div>
                          <div class="row m-0">
                            <div class="col-sm p-0">
                              <input autofocus class="form-control form-control-sm" v-model="attr.value[9]"
                                @keyup="updateValue($event, curEntity, $event.target.value, attr, 9)" />
                            </div>
                            <div class="col-sm p-0">
                              <input autofocus class="form-control form-control-sm" v-model="attr.value[10]"
                                @keyup="updateValue($event, curEntity, $event.target.value, attr, 10)" />
                            </div>
                            <div class="col-sm p-0">
                              <input autofocus class="form-control form-control-sm" v-model="attr.value[11]"
                                @keyup="updateValue($event, curEntity, $event.target.value, attr, 11)" />
                            </div>
                          </div>
                          <div class="row m-0">
                            <SaveEditButton @clickSave="saveEdit(curEntity, attr)" @clickCancel="cancelEdit(attr)" />
                          </div>
                        </div>
                      </template>
                      <input v-else autofocus class="form-control form-control-sm" v-model="attr.value"
                        @change="updateValue($event, curEntity, $event.target.value, attr)" />
                      <SaveEditButton v-if="attr.type !== 'TMatrix'" @clickSave="saveEdit(curEntity, attr)"
                        @clickCancel="cancelEdit(attr)" />
                    </div>
                  </div>
                  <span v-if="!attr._edit" @click="startEdit(attr)" class="clickable">
                    <template v-if="attr.type === 'E3DCOLOR'"><span style="border: 1px solid black;"
                        :style="{ 'color': getAttrValue(attr), 'background-color': getAttrValue(attr) }">{{
                          getAttrValue(attr) }}</span></template>
                    <template v-else>
                      <div class="text-truncate" style="white-space: pre;">{{ formatValue(attr.value) }}</div>
                    </template>
                  </span>
                </td>
              </template>
            </tr>
          </tbody>
        </table>
      </div>

      <div class="ml-3">
        <strong class="d-block">Systems ({{ curEntity._systemsFn().length }})</strong>
        <div v-for="s in curEntity._systemsFn()" class="clickable" @click="clickSystem(s)">
          {{ s.name }}
        </div>
        <strong class="d-block">Queries ({{ curEntity._queriesFn().length }})</strong>
        <div v-for="s in curEntity._queriesFn()" class="clickable" @click="clickQuery(s)">
          {{ s.name }}
        </div>
      </div>
    </div>
  </b-modal>

  <div class="container-fluid m-0 border-bottom" style="padding-right: 32px;">
    <div class="row">
      <SortableColumn @sort="sortValues(entities, 'eid')" columnTitle="eid" columnClass="col-sm-1 p-0"/>
      <SortableColumn @sort="sortValues(entities, 'template')" columnTitle="Template" columnClass="col-sm-9 p-0"/>
      <SortableColumn @sort="sortValues(entities, 'systemsCount', v => v._systemsFn().length)" columnTitle="Systems count" columnClass="col-sm-1 p-0 text-center"/>
      <SortableColumn @sort="sortValues(entities, 'queriesCount', v => v._queriesFn().length)" columnTitle="Queries count" columnClass="col-sm-1 p-0 text-center"/>
    </div>
  </div>

  <RecycleScroller class="scroller" :items="entities" :item-size="32" key-field="eid">
    <template #default="props">
      <div
        :class="['container-fluid', 'm-0', 'scroller-row', 'border-bottom', props.index % 2 == 0 ? 'scroller-row-even' : 'scroller-row-odd']">
        <div class="row clickable" @click="openRow(props)">
          <div class="col-sm-1 p-0">{{ props.item.eid }}</div>
          <div class="col-sm-9 p-0 text-truncate" :title="props.item.template">{{ props.item.template }}</div>
          <div class="col-sm-1 p-0 text-center">{{ props.item._systemsFn().length }}</div>
          <div class="col-sm-1 p-0 text-center">{{ props.item._queriesFn().length }}</div>
        </div>
      </div>
    </template>
  </RecycleScroller>
</template>