<script setup>
const { ref, reactive, watch, computed, defineAsyncComponent } = Vue;
const { coreService, dmService } = Vue.services;
const { commands, commandDomains } = coreService;

const ecs = defineAsyncComponent(() => Vue.helpers.loadModule('./vue/components/ecs.vue'));
const dm = defineAsyncComponent(() => Vue.helpers.loadModule('./vue/components/dm.vue'));

const showCommands = ref(false);
const showCommandsClass = computed(() => {
  return showCommands.value ? 'fa-chevron-up' : 'fa-chevron-down';
});

const _currentTab = coreService.cookieService.get('_currentTab');
const currentTab = ref(_currentTab || 'Main');

const consoleCommand = ref('');
const consoleCommandQuery = ref('');
const consoleCommandNames = reactive([]);

const selectedTimeSpeed = ref(1.0);
const timeSpeedOptions = [
  { html: '<i class="fa fa-pause" aria-hidden="true"></i><span class="extender">_</span>', value: 0 },
  { text: '1/64', value: 0.015625 },
  { text: '1/16', value: 0.0625 },
  { text: '1/8', value: 0.125 },
  { text: '1/4', value: 0.25 },
  { text: '1/2', value: 0.5 },
  { html: '<i class="fa fa-play" aria-hidden="true"></i><span class="extender">_</span>', value: 1.0 },
  { text: '2', value: 2 },
  { text: '4', value: 4 },
  { text: '8', value: 8 },
  { text: '16', value: 16 },
];

function selectTab($event, tab) {
  console.log('selectTab', tab);
  $event.preventDefault();
  currentTab.value = tab;
  coreService.cookieService.put('_currentTab', tab);
}

function toogleCommands() {
  showCommands.value = !showCommands.value;
}

function setTimeSpeed(speed) {
  coreService.sendCommand('setCommonParam', { param: 'timespeed', value: speed });
}

function toggleCommand(c) {
  c._expanded = !c._expanded;
}

watch(consoleCommandQuery, Vue.helpers.debounce(async (query) => {
  consoleCommandNames.splice(0, consoleCommandNames.length);
  consoleCommandNames.push(...coreService.searchConsoleCommand(query));
}, 500));
</script>

<style>
.log textarea,
.log code {
  display: block;
  width: 100%;
  border: 0px;
  white-space: pre;
}

#log-stream {
  white-space: pre;
  height: 56%;
  display: block;
  overflow-y: scroll;
  font-size: 14px;
}

:host>>>tr.disabled,
a.disabled,
.edit-mode-disabled {
  opacity: 0.5;
}

.block-with-border {
  border: solid 1px black;
  padding: 10px;
  margin-top: -1px;
}

.fixed-panel-top {
  position: fixed !important;
  top: 0;
  z-index: 100;
}

.fixed-panel-right {
  position: fixed !important;
  right: 0;
  z-index: 100;
}

.menu-button {
  position: fixed;
  right: 0;
  top: 0;
  z-index: 200;
}

span.extender {
  visibility: hidden;
  width: 0px;
  display: inline-block;
}

:host>>>.unit-row {
  height: 36px;
}

:host>>>.card-block-no-padding .card-body {
  padding: 0;
}

:host>>>.fixed-panel-right .card-body {
  padding: 0;
}
</style>

<template>
  <nav class="d-block navbar fixed-panel-right navbar-light bg-light">
    <button type="button" class="btn btn-sm btn-outline-primary" @click="toogleCommands()" placement="left"
      container="body" ngbTooltip="Console and Commands"><i class="fa" :class="showCommandsClass"
        aria-hidden="true"></i></button>

    <div v-if="showCommands" class="mt-1 card-block-no-padding" style="width: 500px;">
      <div class="block-with-border w-100">
        <h5>Console</h5>
        <vue-bootstrap-typeahead inputClass="d-block w-100" v-model="consoleCommandQuery" :data="consoleCommandNames"
          @hit="consoleCommand = $event"></vue-bootstrap-typeahead>
        <button type="button" class="btn btn-sm btn-outline-primary"
          @click="coreService.runConsoleRaw(consoleCommand);">Send</button>
      </div>

      <div class="accordion" role="tablist">
        <b-card no-body class="mb-1" v-for="(domain, index) in commandDomains">
          <b-card-header header-tag="header" class="p-1" role="tab">
            <b-button block v-b-toggle="`accordion-right-${index}`" variant="info">{{ domain }}</b-button>
          </b-card-header>
          <b-collapse :id="`accordion-right-${index}`" accordion="my-accordion-right" role="tabpanel">
            <b-card-body>
              <div v-for="c in commands[domain]" class="block-with-border">
                <h6>{{ c.name }} <button type="button" class="btn btn-sm btn-outline-primary" @click="toggleCommand(c)"><i
                      class="fa fa-chevron-down" aria-hidden="true"></i></button></h6>
                <div v-if="c._expanded">
                  <label v-for="param in c.params" class="d-block">
                    {{ param.name }}:

                    <select v-if="param.type === 'dmPart'" v-model="param.value">
                      <option v-for="part in dmService.filterPartsByName(param.filterPartsByName)" :key="part.id"
                        :value="part.name">{{ part.name }}</option>
                    </select>

                    <select v-if="param.type === 'enum'" v-model="param.value">
                      <option v-for="v in param.values" :value="v">{{ v }}</option>
                    </select>

                    <input v-if="['r', 'i', 't'].indexOf(param.type) >= 0" v-model="param.value" type="text">
                    <input v-if="param.type === 'b'" v-model="param.value" type="checkbox">

                    <input v-if="param.type === 'p3'" v-model="param.value[0]" type="text" style="width: 50px">
                    <input v-if="param.type === 'p3'" v-model="param.value[1]" type="text" style="width: 50px">
                    <input v-if="param.type === 'p3'" v-model="param.value[2]" type="text" style="width: 50px">
                  </label>

                  <button type="button" class="btn btn-sm btn-outline-primary"
                    @click="coreService.buildAndSendCommand(c);">Send</button>
                </div>
              </div>
            </b-card-body>
          </b-collapse>
        </b-card>
      </div>
    </div>
  </nav>

  <nav class="navbar fixed-bottom navbar-light bg-light form-inline d-flex justify-content-start">
    <b-dropdown dropup class="d-inline" variant="outline-primary">
      <template #button-content>
        <i class="fa fa-bars" aria-hidden="true"></i>
      </template>

      <b-dropdown-item class="dropdown-item" @click="selectTab($event, 'Main')">Main</b-dropdown-item>
      <b-dropdown-item class="dropdown-item" @click="selectTab($event, 'DM')">DM</b-dropdown-item>
      <b-dropdown-item class="dropdown-item" @click="selectTab($event, 'ECS')">ECS</b-dropdown-item>
    </b-dropdown>

    <div class="ml-2">
      <b-form-group>
        <b-form-radio-group v-model="selectedTimeSpeed" :options="timeSpeedOptions" button-variant="outline-warning"
          size="sm" name="radio-btn-outline" @change="setTimeSpeed($event)" buttons></b-form-radio-group>
      </b-form-group>
    </div>
  </nav>

  <div class="container-fluid" style="padding-bottom: 64px;">
    <div class="container-fluid card-block-no-padding" v-if="currentTab === 'Main'">
      <div class="block-with-border">
        <h5>Console</h5>
        <vue-bootstrap-typeahead inputClass="d-block w-100" v-model="consoleCommandQuery" :data="consoleCommandNames"
          @hit="consoleCommand = $event"></vue-bootstrap-typeahead>
        <button type="button" class="btn btn-sm btn-outline-primary"
          @click="coreService.runConsoleRaw(consoleCommand);">Send</button>
      </div>

      <div class="accordion" role="tablist">
        <b-card no-body class="mb-1" v-for="(domain, index) in commandDomains">
          <b-card-header header-tag="header" class="p-1" role="tab">
            <b-button block v-b-toggle="`accordion-${index}`" variant="info">{{ domain }}</b-button>
          </b-card-header>
          <b-collapse :id="`accordion-${index}`" accordion="my-accordion" role="tabpanel">
            <b-card-body>
              <div v-for="c in commands[domain]" class="block-with-border">
                <h6>{{ c.name }} <button type="button" class="btn btn-sm btn-outline-primary" @click="toggleCommand(c)"><i
                      class="fa fa-chevron-down" aria-hidden="true"></i></button></h6>
                <div v-if="c._expanded">
                  <label v-for="param in c.params" class="d-block">
                    {{ param.name }}:

                    <select v-if="param.type === 'dmPart'" v-model="param.value">
                      <option v-for="part in dmService.filterPartsByName(param.filterPartsByName)" :key="part.id"
                        :value="part.name">{{ part.name }}</option>
                    </select>

                    <select v-if="param.type === 'enum'" v-model="param.value">
                      <option v-for="v in param.values" :value="v">{{ v }}</option>
                    </select>

                    <input v-if="['r', 'i', 't'].indexOf(param.type) >= 0" v-model="param.value" type="text">
                    <input v-if="param.type === 'b'" v-model="param.value" type="checkbox">

                    <input v-if="param.type === 'p3'" v-model="param.value[0]" type="text" style="width: 50px">
                    <input v-if="param.type === 'p3'" v-model="param.value[1]" type="text" style="width: 50px">
                    <input v-if="param.type === 'p3'" v-model="param.value[2]" type="text" style="width: 50px">
                  </label>

                  <button type="button" class="btn btn-sm btn-outline-primary"
                    @click="coreService.buildAndSendCommand(c);">Send</button>
                </div>
              </div>
            </b-card-body>
          </b-collapse>
        </b-card>
      </div>
    </div>
    <div v-if="currentTab === 'DM'">
      <dm></dm>
    </div>
    <div v-if="currentTab === 'ECS'">
      <ecs></ecs>
    </div>
  </div>
</template>