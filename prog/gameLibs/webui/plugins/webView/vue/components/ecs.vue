<script setup>
const { watch, defineAsyncComponent } = Vue;
const { ecsService } = Vue.services;

const ECSEntities = defineAsyncComponent(() => Vue.helpers.loadModule('./vue/components/ecs-entities.vue'));
const ECSComponents = defineAsyncComponent(() => Vue.helpers.loadModule('./vue/components/ecs-components.vue'));
const ECSSystems = defineAsyncComponent(() => Vue.helpers.loadModule('./vue/components/ecs-systems.vue'));
const ECSQueries = defineAsyncComponent(() => Vue.helpers.loadModule('./vue/components/ecs-queries.vue'));
const ECSEvents = defineAsyncComponent(() => Vue.helpers.loadModule('./vue/components/ecs-events.vue'));
const ECSTemplates = defineAsyncComponent(() => Vue.helpers.loadModule('./vue/components/ecs-templates.vue'));
const ECSDynamicQuery = defineAsyncComponent(() => Vue.helpers.loadModule('./vue/components/ecs-dynamic-query.vue'));

// console.log('ECS', ecsService.entities);
// watch(ecsService.entities, (newVal, oldVal) => {
//   console.log('ECS W', newVal);
//   // data.value = newVal;
//   // clusterize.update(newVal);
// });

function updateFilter(event) {
  const val = event.target.value.toLowerCase();
  ecsService.filterEntities = val;
  // this.table.offset = 0;
}

function updateFilterByEid(event) {
  const val = event.target.value.toLowerCase();
  ecsService.filterEntitiesByEid = val;
  // this.table.offset = 0;
}

function updateFilterComponents(event) {
  const val = event.target.value.toLowerCase();
  ecsService.filterComponents = val;
  // this.componentsTable.offset = 0;
}

function updateFilterTemplates(event) {
  const val = event.target.value.toLowerCase();
  ecsService.filterTemplates = val;
  // this.templatesTable.offset = 0;
}

function updateFilterSystems(event) {
  const val = event.target.value.toLowerCase();
  ecsService.filterSystems = val;
  // this.systemsTable.offset = 0;
}

function updateFilterQueries(event) {
  const val = event.target.value.toLowerCase();
  ecsService.filterQueries = val;
  // this.queriesTable.offset = 0;
}

function updateFilterEvents(event) {
  const val = event.target.value.toLowerCase();
  ecsService.filterEvents = val;
  // this.eventsTable.offset = 0;
}
</script>

<style>
.clickable {
  cursor: pointer;
}

span.clickable {
  min-height: 20px;
  display: block;
}

.ecs-item-type {
  font-size: 8pt;
  font-weight: bold;
}

.ecs-item-es {
  color: #12A;
}

.ecs-item-te {
  color: #919;
}

.ecs-info {
  font-weight: bold;
  font-size: 10px;
  position: absolute;
  margin-top: 16px;
}

.btn-in-components-row {
  padding: 2px !important;
  font-size: 10px !important;
}

.modal-body {
  overflow-x: scroll;
}

.modal-lg {
  max-width: 1280px !important;
}

#modal-edit-dynamic-query .modal-lg {
  max-width: 800px !important;
}

.scroller {
  height: 100%;
}

.scroller-row {
  height: 32px;
}

.scroller-row .row {
  height: 32px;
  line-height: 32px;
}

.scroller-row-odd {
  background-color: rgba(0, 0, 0, .05);
}

.scroller-row-even {
  background-color: rgba(255, 255, 255, .05);
}

table tr {
  line-height: 32px;
}
</style>

<template>
  <b-tabs content-class="mt-3">
    <b-tab title="Entities" active>
      <div class="contrainer-fluid mt-2">
        <div class="row">
          <div class="col-sm-1">
            <button type="button" class="btn btn-sm btn-outline-primary align-middle"
              @click="ecsService.fetchEntities()">Fetch Enities</button>
          </div>
          <div class="col-sm-3">
            <input type="text" class="form-control form-control-sm" placeholder="Type to filter by template..."
              @keyup="updateFilter($event)" :value="ecsService.filterEntities" />
          </div>
          <div class="col-sm-3">
            <input type="text" class="form-control form-control-sm" placeholder="Type to filter by eid..."
              @keyup="updateFilterByEid($event)" :value="ecsService.filterEntitiesByEid" />
          </div>
        </div>
      </div>

      <ECSEntities :entities="ecsService.entities" />
    </b-tab>
    <b-tab title="Components">
      <div class="contrainer-fluid mt-2">
        <div class="row">
          <div class="col-sm-3">
            <input type="text" class="form-control form-control-sm" placeholder="Type to filter..."
              @keyup="updateFilterComponents($event)" :value="ecsService.filterComponents" />
          </div>
        </div>
      </div>

      <ECSComponents :components="ecsService.components" />
    </b-tab>
    <b-tab title="Systems">
      <div class="contrainer-fluid mt-2">
        <div class="row">
          <div class="col-sm-3">
            <input type="text" class="form-control form-control-sm" placeholder="Type to filter..."
              @keyup="updateFilterSystems($event)" :value="ecsService.filterSystems" />
          </div>
        </div>
      </div>

      <ECSSystems :systems="ecsService.systems" />
    </b-tab>
    <b-tab title="Queries">
      <div class="contrainer-fluid mt-2">
        <div class="row">
          <div class="col-sm-3">
            <input type="text" class="form-control form-control-sm" placeholder="Type to filter..."
              @keyup="updateFilterQueries($event)" :value="ecsService.filterQueries" />
          </div>
        </div>
      </div>

      <ECSQueries :queries="ecsService.queries" />
    </b-tab>
    <b-tab title="Events">
      <div class="contrainer-fluid mt-2">
        <div class="row">
          <div class="col-sm-3">
            <input type="text" class="form-control form-control-sm" placeholder="Type to filter..."
              @keyup="updateFilterEvents($event)" :value="ecsService.filterEvents" />
          </div>
        </div>
      </div>

      <ECSEvents :events="ecsService.events" />
    </b-tab>
    <b-tab title="Templates">
      <div class="contrainer-fluid mt-2">
        <div class="row">
          <div class="col-sm-3">
            <input type="text" class="form-control form-control-sm" placeholder="Type to filter..."
              @keyup="updateFilterTemplates($event)" :value="ecsService.filterTemplates" />
          </div>
        </div>
      </div>

      <ECSTemplates :templates="ecsService.templates" />
    </b-tab>
    <b-tab title="Dynamic Query">
      <ECSDynamicQuery />
    </b-tab>
  </b-tabs>
</template>