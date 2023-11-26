<script setup>
const { ref, computed, watch, defineComponent } = Vue;
const { formatValue, compToString, shouldOpenEditor, getAttrValue, startEdit, cancelEdit, saveEdit, updateValue, sortValues } = Vue.helpers;
const { ecsService } = Vue.services;

const dynQueryCompsRO = ref([]);
const dynQueryCompsRQ = ref([]);
const dynQueryCompsNO = ref([]);
const dynQueryFilter = ref('');

watch(ecsService.components, (components) => {
  const q = ecsService.dynamicQuery;
  console.log('>>>>', q);

  for (let c of q.comps_ro) {
    const comp = components.find(c2 => c2.name === c);
    if (comp) {
      dynQueryCompsRO.value.push(comp);
    }
  }

  for (let c of q.comps_rq) {
    const comp = components.find(c2 => c2.name === c);
    if (comp) {
      dynQueryCompsRQ.value.push(comp);
    }
  }

  for (let c of q.comps_no) {
    const comp = components.find(c2 => c2.name === c);
    if (comp) {
      dynQueryCompsNO.value.push(comp);
    }
  }
});

function saveDynamicQuery() {
  console.log('saveDynamicQuery', dynQueryCompsRO.value, dynQueryCompsRQ.value, dynQueryCompsNO.value, dynQueryFilter.value);

  ecsService.dynamicQuery = {
    comps_ro: Object.values(dynQueryCompsRO.value.map(c => c.name)),
    comps_rq: Object.values(dynQueryCompsRQ.value.map(c => c.name)),
    comps_no: Object.values(dynQueryCompsNO.value.map(c => c.name)),
  };

  ecsService.dynamicQueryFilter.value = dynQueryFilter.value;

  ecsService.saveDynamicQuery();
  ecsService.performDynamicQuery();
}

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

const curDynamicEntityId = ref(-1);
const curDynamicEntityComponent = ref(null);

const curDynamicEntity = computed(() => {
  return curDynamicEntityId.value >= 0 ? ecsService.dynamicQueryEntities[curDynamicEntityId.value] : null;
});

watch(curDynamicEntity, (entity) => {
  console.log('curDynamicEntity', entity);
  if (entity) {
    Vue.myApp.$bvModal.show('modal-dynamic-entity');
  } else {
    Vue.myApp.$bvModal.hide('modal-dynamic-entity');
  }
});

const curCompJson = computed({
  get() { return compToString(curDynamicEntityComponent.value.value, true); },
  set(val) {
    try {
      const jsonVal = JSON.parse(val);
      updateValue({}, curDynamicEntity, jsonVal, curDynamicEntityComponent.value);
    } catch (e) {
    }
  }
});

function openRow(row) {
  console.log('openRow', row);
  curDynamicEntityId.value = row.index;
}

function onCloseRow() {
  console.log('onCloseRow');
  curDynamicEntityId.value = -1;
}

function openAttr(row, attr, content) {
  if (shouldOpenEditor(attr) || attr.type === 'E3DCOLOR') {
    curDynamicEntityComponent.value = attr;
    Vue.myApp.$bvModal.show('modal-dynamic-components-values');
  }
}
</script>

<style></style>

<template>
  <b-modal id="modal-dynamic-components-values" centered size="lg"
    @ok="saveEdit(curDynamicEntity, curDynamicEntityComponent)">
    <template #modal-title>
      <h5>{{ curDynamicEntityComponent.name }}</h5>
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

  <b-modal id="modal-dynamic-entity" centered size="lg" ok-only @hidden="onCloseRow">
    <template #modal-title>
      <h5>{{ curDynamicEntity.eid }}: {{ curDynamicEntity.template }}</h5>
    </template>
    <div style="padding-left:35px;padding-right:35px;" class="d-flex flex-row">
      <div>
        <strong class="d-block">Components ({{ curDynamicEntity.components.length }})</strong>
        <table class="table table-sm table-bordered table-hover">
          <thead>
            <th>Name</th>
            <th>Type</th>
            <th>Value</th>
          </thead>
          <tbody>
            <tr v-for="attr in curDynamicEntity.components">
              <td>{{ attr.name }}
                <span v-if="attr.isTracked" class="ecs-info"> tracked</span>
                <span v-if="attr.isReplicated" class="ecs-info"> replicated</span>
                <span v-if="attr.isDontReplicate" class="ecs-info"> do not replicate</span>
                <button type="button" class="btn btn-sm btn-outline-primary btn-in-components-row"
                  @click="ecsService.fetchEntityAttributes(curDynamicEntity.eid);"><i class="fa fa-refresh"
                    aria-hidden="true"></i></button>
              </td>
              <template v-if="shouldOpenEditor(attr)">
                <td>
                  <div class="text-truncate"><span :title="attr.type">{{ attr.type }}</span></div>
                </td>
                <td class="clickable" @click="openAttr(curDynamicEntity, attr, ecsAttribute)">[Click to see value]</td>
              </template>
              <template v-if="!shouldOpenEditor(attr)">
                <td>
                  <div class="text-truncate"><span :title="attr.type">{{ attr.type }}</span></div>
                </td>
                <td>
                  <div class="contrainer" v-if="attr._edit">
                    <div class="row m-0">
                      <color-picker v-if="attr.type === 'E3DCOLOR'" :color="attr._color"
                        @change="updateValue($event, curDynamicEntity, $event.rgba, attr)"></color-picker>
                      <template v-else-if="attr.type === 'bool'">
                        <div class="col-sm p-0">
                          <input autofocus class="form-control form-control-sm form-check-input" v-model="attr.value"
                            type="checkbox"
                            @change="updateValue($event, curDynamicEntity, $event.target.checked, attr)" />
                        </div>
                      </template>
                      <template v-else-if="attr.type === 'float'">
                        <div class="col-sm p-0">
                          <input autofocus class="form-control form-control-sm" v-model="attr.value.r"
                            @keyup="updateValue($event, curDynamicEntity, $event.target.value, attr, 'r')" />
                        </div>
                      </template>
                      <template v-else-if="attr.type === 'ecs::EntityId'">
                        <div class="col-sm p-0">
                          <input autofocus class="form-control form-control-sm" v-model="attr.value.eid"
                            @keyup="updateValue($event, curDynamicEntity, $event.target.value, attr, 'eid')" />
                        </div>
                      </template>
                      <template v-else-if="attr.type === 'int'">
                        <div class="col-sm p-0">
                          <input autofocus class="form-control form-control-sm" v-model="attr.value.i"
                            @keyup="updateValue($event, curDynamicEntity, $event.target.value, attr, 'i')" />
                        </div>
                      </template>
                      <template v-else-if="attr.type === 'Point2'">
                        <div class="col-sm p-0">
                          <input autofocus class="form-control form-control-sm" v-model="attr.value.x"
                            @keyup="updateValue($event, curDynamicEntity, $event.target.value, attr, 'x')" />
                        </div>
                        <div class="col-sm p-0">
                          <input autofocus class="form-control form-control-sm" v-model="attr.value.y"
                            @keyup="updateValue($event, curDynamicEntity, $event.target.value, attr, 'y')" />
                        </div>
                      </template>
                      <template v-else-if="attr.type === 'Point3'">
                        <div class="col-sm p-0">
                          <input autofocus class="form-control form-control-sm" v-model="attr.value.x"
                            @keyup="updateValue($event, curDynamicEntity, $event.target.value, attr, 'x')" />
                        </div>
                        <div class="col-sm p-0">
                          <input autofocus class="form-control form-control-sm" v-model="attr.value.y"
                            @keyup="updateValue($event, curDynamicEntity, $event.target.value, attr, 'y')" />
                        </div>
                        <div class="col-sm p-0">
                          <input autofocus class="form-control form-control-sm" v-model="attr.value.z"
                            @keyup="updateValue($event, curDynamicEntity, $event.target.value, attr, 'z')" />
                        </div>
                      </template>
                      <template v-else-if="attr.type === 'Point4'">
                        <div class="col-sm p-0">
                          <input autofocus class="form-control form-control-sm" v-model="attr.value.x"
                            @keyup="updateValue($event, curDynamicEntity, $event.target.value, attr, 'x')" />
                        </div>
                        <div class="col-sm p-0">
                          <input autofocus class="form-control form-control-sm" v-model="attr.value.y"
                            @keyup="updateValue($event, curDynamicEntity, $event.target.value, attr, 'y')" />
                        </div>
                        <div class="col-sm p-0">
                          <input autofocus class="form-control form-control-sm" v-model="attr.value.z"
                            @keyup="updateValue($event, curDynamicEntity, $event.target.value, attr, 'z')" />
                        </div>
                        <div class="col-sm p-0">
                          <input autofocus class="form-control form-control-sm" v-model="attr.value.w"
                            @keyup="updateValue($event, curDynamicEntity, $event.target.value, attr, 'w')" />
                        </div>
                      </template>
                      <template v-else-if="attr.type === 'TMatrix'">
                        <div class="col-sm p-0">
                          <div class="row m-0">
                            <div class="col-sm p-0">
                              <input autofocus class="form-control form-control-sm" v-model="attr.value[0]"
                                @keyup="updateValue($event, curDynamicEntity, $event.target.value, attr, 0)" />
                            </div>
                            <div class="col-sm p-0">
                              <input autofocus class="form-control form-control-sm" v-model="attr.value[1]"
                                @keyup="updateValue($event, curDynamicEntity, $event.target.value, attr, 1)" />
                            </div>
                            <div class="col-sm p-0">
                              <input autofocus class="form-control form-control-sm" v-model="attr.value[2]"
                                @keyup="updateValue($event, curDynamicEntity, $event.target.value, attr, 2)" />
                            </div>
                          </div>
                          <div class="row m-0">
                            <div class="col-sm p-0">
                              <input autofocus class="form-control form-control-sm" v-model="attr.value[3]"
                                @keyup="updateValue($event, curDynamicEntity, $event.target.value, attr, 3)" />
                            </div>
                            <div class="col-sm p-0">
                              <input autofocus class="form-control form-control-sm" v-model="attr.value[4]"
                                @keyup="updateValue($event, curDynamicEntity, $event.target.value, attr, 4)" />
                            </div>
                            <div class="col-sm p-0">
                              <input autofocus class="form-control form-control-sm" v-model="attr.value[5]"
                                @keyup="updateValue($event, curDynamicEntity, $event.target.value, attr, 5)" />
                            </div>
                          </div>
                          <div class="row m-0">
                            <div class="col-sm p-0">
                              <input autofocus class="form-control form-control-sm" v-model="attr.value[6]"
                                @keyup="updateValue($event, curDynamicEntity, $event.target.value, attr, 6)" />
                            </div>
                            <div class="col-sm p-0">
                              <input autofocus class="form-control form-control-sm" v-model="attr.value[7]"
                                @keyup="updateValue($event, curDynamicEntity, $event.target.value, attr, 7)" />
                            </div>
                            <div class="col-sm p-0">
                              <input autofocus class="form-control form-control-sm" v-model="attr.value[8]"
                                @keyup="updateValue($event, curDynamicEntity, $event.target.value, attr, 8)" />
                            </div>
                          </div>
                          <div class="row m-0">
                            <div class="col-sm p-0">
                              <input autofocus class="form-control form-control-sm" v-model="attr.value[9]"
                                @keyup="updateValue($event, curDynamicEntity, $event.target.value, attr, 9)" />
                            </div>
                            <div class="col-sm p-0">
                              <input autofocus class="form-control form-control-sm" v-model="attr.value[10]"
                                @keyup="updateValue($event, curDynamicEntity, $event.target.value, attr, 10)" />
                            </div>
                            <div class="col-sm p-0">
                              <input autofocus class="form-control form-control-sm" v-model="attr.value[11]"
                                @keyup="updateValue($event, curDynamicEntity, $event.target.value, attr, 11)" />
                            </div>
                          </div>
                          <div class="row m-0">
                            <SaveEditButton @clickSave="saveEdit(curDynamicEntity, attr)"
                              @clickCancel="cancelEdit(attr)" />
                          </div>
                        </div>
                      </template>
                      <input v-else autofocus class="form-control form-control-sm" v-model="attr.value"
                        @change="updateValue($event, curDynamicEntity, $event.target.value, attr)" />
                      <SaveEditButton v-if="attr.type !== 'TMatrix'" @clickSave="saveEdit(curDynamicEntity, attr)"
                        @clickCancel="cancelEdit(attr)" />
                    </div>
                  </div>
                  <span v-if="!attr._edit" @click="startEdit(attr)" class="clickable" style="white-space: pre;">
                    <template v-if="attr.type === 'E3DCOLOR'"><span style="border: 1px solid black;"
                        :style="{ 'color': getAttrValue(attr), 'background-color': getAttrValue(attr) }">{{
                          getAttrValue(attr) }}</span></template>
                    <template v-else>{{ formatValue(attr.value) }}</template>
                  </span>
                </td>
              </template>
            </tr>
          </tbody>
        </table>
      </div>
    </div>
  </b-modal>

  <b-modal id="modal-edit-dynamic-query" centered size="lg" @ok="saveDynamicQuery()">
    <template #modal-title>
      <h5>Dynamic Query</h5>
    </template>

    <div class="continer" style="min-height: 480px;">
      <div class="row">
        <div class="col-sm-2">Components</div>
        <div class="col-sm-10">
          <vue-multiselect v-model="dynQueryCompsRO" :options="ecsService.components" :multiple="true"
            :close-on-select="false" :clear-on-select="false" :preserve-search="true" placeholder="Components"
            label="name" track-by="name" :preselect-first="false">
            <template slot="selection" slot-scope="{ values, search, isOpen }"><span class="multiselect__single"
                v-if="values.length" v-show="!isOpen">{{ values.length }} options selected</span></template>
          </vue-multiselect>
        </div>

        <div class="col-sm-2">Required</div>
        <div class="col-sm-10">
          <vue-multiselect v-model="dynQueryCompsRQ" :options="ecsService.components" :multiple="true"
            :close-on-select="false" :clear-on-select="false" :preserve-search="true" placeholder="Required Components"
            label="name" track-by="name" :preselect-first="false">
            <template slot="selection" slot-scope="{ values, search, isOpen }"><span class="multiselect__single"
                v-if="values.length" v-show="!isOpen">{{ values.length }} options selected</span></template>
          </vue-multiselect>
        </div>

        <div class="col-sm-2">Required Not</div>
        <div class="col-sm-10">
          <vue-multiselect v-model="dynQueryCompsNO" :options="ecsService.components" :multiple="true"
            :close-on-select="false" :clear-on-select="false" :preserve-search="true"
            placeholder="Required Not Components" label="name" track-by="name" :preselect-first="false">
            <template slot="selection" slot-scope="{ values, search, isOpen }"><span class="multiselect__single"
                v-if="values.length" v-show="!isOpen">{{ values.length }} options selected</span></template>
          </vue-multiselect>
        </div>

        <div class="col-sm-2">Filter</div>
        <div class="col-sm-10">
          <b-form-input v-model="dynQueryFilter" size="sm" placeholder="Example: eq(foo, -1)"></b-form-input>
        </div>
      </div>
    </div>

    <template #modal-footer="{ ok, cancel, hide }">
      <b-button size="sm" variant="success" @click="ok()">
        Save
      </b-button>
      <b-button size="sm" variant="danger" @click="cancel()">
        Cancel
      </b-button>
    </template>
  </b-modal>

  <div class="contrainer-fluid mt-2">
    <div class="row">
      <div class="col-sm-3">
        <button type="button" class="btn btn-sm btn-outline-primary align-middle"
          @click="ecsService.performDynamicQuery()">Perform Query</button>
        <b-button v-b-modal.modal-edit-dynamic-query variant="outline-primary" size="sm">Edit Query</b-button>
      </div>
    </div>
  </div>

  <div class="container-fluid m-0 border-bottom" style="padding-right: 32px;">
    <div class="row">
      <SortableColumn @sort="sortValues(ecsService.dynamicQueryEntities, 'eid')" columnTitle="eid"
        columnClass="col-sm p-0" />
      <SortableColumn @sort="sortValues(ecsService.dynamicQueryEntities, 'template')" columnTitle="Template"
        columnClass="col-sm p-0" columnStyle="max-width: 350px;" />
      <SortableColumn v-for="c in ecsService.dynamicQueryColumns"
        @sort="sortValues(ecsService.dynamicQueryEntities, `_${c}`)" :columnTitle="c"
        columnClass="col-sm p-0 text-center" />
    </div>
  </div>

  <RecycleScroller class="scroller" :items="ecsService.dynamicQueryEntities" :item-size="32" key-field="eid">
    <template #default="props">
      <div
        :class="['container-fluid', 'm-0', 'scroller-row', 'border-bottom', props.index % 2 == 0 ? 'scroller-row-even' : 'scroller-row-odd']">
        <div class="row clickable" @click="openRow(props)">
          <div class="col-sm p-0 border-right">{{ props.item.eid }}</div>
          <div class="col-sm p-0 border-right" style="max-width: 350px;">
            <div class="text-truncate" :title="props.item.template">{{ props.item.template }}</div>
          </div>
          <div v-for="c in ecsService.dynamicQueryColumns" class="col-sm p-0 text-truncate text-center border-right">
            <span>{{ props.item[`_${c}`] }}</span>
          </div>
        </div>
      </div>
    </template>
  </RecycleScroller>
</template>