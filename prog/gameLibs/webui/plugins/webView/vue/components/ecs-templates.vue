<script setup>
const { ref, computed, watch, defineProps, defineComponent } = Vue;
const { formatValue, shouldOpenEditor, getAttrValue, compToString, sortValues } = Vue.helpers;
const { ecsService } = Vue.services;

const props = defineProps({
  templates: Array,
});

const curTemplateComponent = ref(null);
const curTemplateSystem = ref(null);
const curTemplateQuery = ref(null);

const curCompJson = computed(() => compToString(curTemplateComponent.value.value, true));

function openTemplateComp(attr) {
  if (shouldOpenEditor(attr) || attr.type === 'E3DCOLOR') {
    curTemplateComponent.value = attr;
    Vue.myApp.$bvModal.show('modal-template-values');
  }
}

function clickTemplateSystem(sys) {
  console.log('clickTemplateSystem', sys);
  curTemplateSystem.value = sys;
  Vue.myApp.$bvModal.show('modal-template-system');
}

function clickTemplateQuery(q) {
  console.log('clickTemplateQuery', q);
  curTemplateQuery.value = q;
  Vue.myApp.$bvModal.show('modal-template-query');
}

const TemplateParents = defineComponent({
  name: 'TemplateParents',
  props: {
    parents: Object,
  },
  methods: {
    openTemplateComp(attr) {
      openTemplateComp(attr);
    },
    shouldOpenEditor(attr) {
      return shouldOpenEditor(attr);
    },
    formatValue(value) {
      return formatValue(value);
    },
    getAttrValue(attr) {
      return getAttrValue(attr);
    },
  },
  template: `
    <template v-for="parent in parents">
      <tr>
        <td colspan="2"><strong>{{ parent.name }}</strong></td>
      </tr>
      <tr v-for="parentAttr in parent._template.components">
        <td>
          <div class="text-nowrap text-truncate"
            :class="{ 'pb-1': parentAttr.isTracked || parentAttr.isReplicated || parentAttr.isDontReplicate }">
            <span v-if="parentAttr.isTracked || parentAttr.isReplicated || parentAttr.isDontReplicate" class="ecs-info">
              <span v-if="parentAttr.isTracked" class="pr-1">tracked</span>
              <span v-if="parentAttr.isReplicated" class="pr-1">replicated</span>
              <span v-if="parentAttr.isDontReplicate" class="pr-1">do not replicate</span>
            </span>
            <span :title="parentAttr.name">{{ parentAttr.name }}</span>
          </div>
        </td>
        <td>
          <div class="text-truncate"><span :title="parentAttr.type">{{ parentAttr.type }}</span></div>
        </td>
        <template v-if="shouldOpenEditor(parentAttr)">
          <td @click="openTemplateComp(parentAttr)" class="clickable">[Click to see value]</td>
        </template>
        <template v-else>
          <td>
            <template v-if="parentAttr.type === 'E3DCOLOR'">
              <span style="border: 1px solid black;" :style="{'color': getAttrValue(parentAttr), 'background-color': getAttrValue(parentAttr)}">{{ getAttrValue(parentAttr) }}</span>
              {{ formatValue(parentAttr.value) }}
            </template>
            <template v-else><div class="text-truncate" style="white-space: pre;">{{ formatValue(parentAttr.value) }}</div></template>
            
          </td>
        </template>
      </tr>
      <template v-if="parent._template.parents">
        <TemplateParents :parents="parent._template.parents" />
      </template>
    </template>
  `,
});

const curTemplateId = ref(-1);

const curTemplate = computed(() => {
  return curTemplateId.value >= 0 ? props.templates[curTemplateId.value] : null;
});

watch(curTemplate, (entity) => {
  console.log('curTemplate', entity);
  if (entity) {
    Vue.myApp.$bvModal.show('modal-template-details');
  } else {
    Vue.myApp.$bvModal.hide('modal-template-details');
  }
});

function openRow(row) {
  console.log('openRow', row);

  curTemplateId.value = row.index;
}

function onCloseRow() {
  console.log('onCloseRow');
  curTemplateId.value = -1;
}
</script>

<style></style>

<template>
  <b-modal id="modal-template-system" centered size="lg" ok-only>
    <template #modal-title>
      <h5>{{ curTemplateSystem.name }}</h5>
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
            <tr v-if="curTemplateSystem.componentsRW.length > 0">
              <td colspan="2"><strong>Read-Write</strong></td>
            </tr>
            <tr v-for="c in curTemplateSystem.componentsRW">
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
            <tr v-if="curTemplateSystem.componentsRO.length > 0">
              <td colspan="2"><strong>Read-Only</strong></td>
            </tr>
            <tr v-for="c in curTemplateSystem.componentsRO">
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
            <tr v-if="curTemplateSystem.componentsRQ.length > 0">
              <td colspan="2"><strong>Required</strong></td>
            </tr>
            <tr v-for="c in curTemplateSystem.componentsRQ">
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
            <tr v-if="curTemplateSystem.componentsNO.length > 0">
              <td colspan="2"><strong>Required Not</strong></td>
            </tr>
            <tr v-for="c in curTemplateSystem.componentsNO">
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
            <tr v-for="t in curTemplateSystem._templateNames">
              <td>
                <div class="text-nowrap text-truncate" :title="t">{{ t }}</div>
              </td>
              <td>
                <div class="text-nowrap text-truncate" :title="curTemplateSystem._templates[t]">{{
                  curTemplateSystem._templates[t] }}
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
            <tr v-for="e in curTemplateSystem.events">
              <td>
                <div class="text-nowrap text-truncate" :title="e">{{ e }}</div>
              </td>
            </tr>
          </tbody>
        </table>
      </div>
    </div>
  </b-modal>

  <b-modal id="modal-template-query" centered size="lg" ok-only>
    <template #modal-title>
      <h5>{{ curTemplateQuery.name }}</h5>
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
            <tr v-if="curTemplateQuery.componentsRW.length > 0">
              <td colspan="2"><strong>Read-Write</strong></td>
            </tr>
            <tr v-for="c in curTemplateQuery.componentsRW">
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
            <tr v-if="curTemplateQuery.componentsRO.length > 0">
              <td colspan="2"><strong>Read-Only</strong></td>
            </tr>
            <tr v-for="c in curTemplateQuery.componentsRO">
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
            <tr v-if="curTemplateQuery.componentsRQ.length > 0">
              <td colspan="2"><strong>Required</strong></td>
            </tr>
            <tr v-for="c in curTemplateQuery.componentsRQ">
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
            <tr v-if="curTemplateQuery.componentsNO.length > 0">
              <td colspan="2"><strong>Required Not</strong></td>
            </tr>
            <tr v-for="c in curTemplateQuery.componentsNO">
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
            <tr v-for="t in curTemplateQuery._templateNames">
              <td>
                <div class="text-nowrap text-truncate" :title="t">{{ t }}</div>
              </td>
              <td>
                <div class="text-nowrap text-truncate" :title="curTemplateQuery._templates[t]">{{
                  curTemplateQuery._templates[t] }}
                </div>
              </td>
            </tr>
          </tbody>
        </table>
      </div>
    </div>
  </b-modal>

  <b-modal id="modal-template-values" centered size="lg" ok-only>
    <template #modal-title>
      <h5>{{ curTemplateComponent.name }}</h5>
    </template>

    <pre>{{ curCompJson }}</pre>
  </b-modal>

  <b-modal id="modal-template-details" centered size="lg" ok-only @hidden="onCloseRow">
    <template #modal-title>
      <h5>{{ curTemplate.name }}</h5>
    </template>

    <div class="d-flex flex-row">
      <div>
        <strong class="d-block">Components</strong>
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
            <tr>
              <td colspan="3"><strong>{{ curTemplate.name }}</strong></td>
            </tr>
            <tr v-for="attr in curTemplate.components">
              <td>
                <div class="text-nowrap text-truncate"
                  :class="{ 'pb-1': attr.isTracked || attr.isReplicated || attr.isDontReplicate }">
                  <span v-if="attr.isTracked || attr.isReplicated || attr.isDontReplicate" class="ecs-info">
                    <span v-if="attr.isTracked" class="pr-1">tracked</span>
                    <span v-if="attr.isReplicated" class="pr-1">replicated</span>
                    <span v-if="attr.isDontReplicate" class="pr-1">do not replicate</span>
                  </span>
                  <span :title="attr.name">{{ attr.name }}</span>
                </div>
              </td>
              <td>
                <div class="text-truncate"><span :title="attr.type">{{ attr.type }}</span></div>
              </td>
              <template v-if="shouldOpenEditor(attr)">
                <td @click="openTemplateComp(attr)" class="clickable">[Click to see value]</td>
              </template>
              <template v-else>
                <td>
                  <template v-if="attr.type === 'E3DCOLOR'">
                    <span style="border: 1px solid black;"
                      :style="{ 'color': getAttrValue(attr), 'background-color': getAttrValue(attr) }">{{
                        getAttrValue(attr) }}</span>
                    {{ formatValue(attr.value) }}
                  </template>
                  <template v-else><div class="text-truncate" style="white-space: pre;">{{ formatValue(attr.value) }}</div></template>
                </td>
              </template>
            </tr>

            <TemplateParents :parents="curTemplate.parents" />
          </tbody>
        </table>
      </div>
      <div class="ml-3">
        <strong class="d-block">Systems ({{ curTemplate._systems.length }})</strong>
        <div v-for="s in curTemplate._systems" class="clickable" @click="clickTemplateSystem(s)">
          {{ s.name }}
        </div>
        <strong class="d-block">Queries ({{ curTemplate._queries.length }})</strong>
        <div v-for="s in curTemplate._queries" class="clickable" @click="clickTemplateQuery(s)">
          {{ s.name }}
        </div>
      </div>
    </div>
  </b-modal>

  <div class="container-fluid m-0 border-bottom" style="padding-right: 32px;">
    <div class="row">
      <SortableColumn @sort="sortValues(templates, 'name')" columnTitle="Name" columnClass="col-sm-7 p-0" />
      <SortableColumn @sort="sortValues(templates, '_componentsCount')" columnTitle="Components count" columnClass="col-sm-1 p-0 text-center" />
      <SortableColumn @sort="sortValues(templates, '_trackedCount')" columnTitle="Tracked count" columnClass="col-sm-1 p-0 text-center" />
      <SortableColumn @sort="sortValues(templates, '_replicatedCount')" columnTitle="Replicated count" columnClass="col-sm-1 p-0 text-center" />
      <SortableColumn @sort="sortValues(templates, 'systemsCount', v => v._systems.length)" columnTitle="Systems count" columnClass="col-sm-1 p-0 text-center"/>
      <SortableColumn @sort="sortValues(templates, 'queriesCount', v => v._queries.length)" columnTitle="Queries count" columnClass="col-sm-1 p-0 text-center"/>
    </div>
  </div>

  <RecycleScroller class="scroller" :items="templates" :item-size="32" key-field="id">
    <template #default="props">
      <div
        :class="['container-fluid', 'm-0', 'scroller-row', 'border-bottom', props.index % 2 == 0 ? 'scroller-row-even' : 'scroller-row-odd']">
        <div class="row clickable" @click="openRow(props)">
          <div class="col-sm-7 p-0">{{ props.item.name }}</div>
          <div class="col-sm-1 p-0 text-center">{{ props.item._componentsCount }}</div>
          <div class="col-sm-1 p-0 text-center">{{ props.item._trackedCount }}</div>
          <div class="col-sm-1 p-0 text-center">{{ props.item._replicatedCount }}</div>
          <div class="col-sm-1 p-0 text-center">{{ props.item._systems.length }}</div>
          <div class="col-sm-1 p-0 text-center">{{ props.item._queries.length }}</div>
        </div>
      </div>
    </template>
  </RecycleScroller>
</template>