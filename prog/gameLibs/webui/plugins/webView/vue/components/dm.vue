<script setup>
const { ref, defineAsyncComponent } = Vue;
const { coreService, dmService } = Vue.services;

const DMParts = defineAsyncComponent(() => Vue.helpers.loadModule('./vue/components/dm-parts.vue'));
const DMMetaParts = defineAsyncComponent(() => Vue.helpers.loadModule('./vue/components/dm-meta-parts.vue'));
const DMEffects = defineAsyncComponent(() => Vue.helpers.loadModule('./vue/components/dm-effects.vue'));
const DMEvents = defineAsyncComponent(() => Vue.helpers.loadModule('./vue/components/dm-events.vue'));

const currentEid = ref(null);

function drawDamagedParts() {
  dmService.drawDamagedParts = !dmService.drawDamagedParts;
  dmService.drawKilledParts = false;

  for (let part of dmService.parts) {
    part.draw = dmService.drawDamagedParts && part.hpRel != 1.0;
    coreService.sendCommand('drawPart', { part: part.name, draw: part.draw });
  }
}

function drawKilledParts() {
  dmService.drawKilledParts = !dmService.drawKilledParts;
  dmService.drawDamagedParts = false;

  for (let part of dmService.parts) {
    part.draw = dmService.drawKilledParts && part.hpRel == 0.0;
    coreService.sendCommand('drawPart', { part: part.name, draw: part.draw });
  }
}
</script>

<style>
.clickable {
  cursor: pointer;
}
</style>

<template>
  <div class="container-fluid">
    <div class="row mb-1">
      <div class="float-xs-left ml-1">
        <button type="button" class="btn btn-sm btn-outline-primary" :class="{ 'active': dmService.drawDamagedParts }"
          @click="drawDamagedParts();">Draw damaged parts</button>
        <button type="button" class="btn btn-sm btn-outline-primary d-block mt-1"
          :class="{ 'active': dmService.drawKilledParts }" @click="drawKilledParts();">Draw killed parts</button>
      </div>
      <div class="float-xs-left ml-1">
        <button type="button" class="btn btn-sm btn-outline-primary mt-1" placement="top" container="body"
          ngbTooltip="Fetch entities" @click="ecsService.fetchEntities('dm_debug');"><i class="fa fa-refresh"
            aria-hidden="true"></i></button>
        <span>Select a vehicle:</span>
        <vue-multiselect v-model="currentEid" :options="dmService.eids" :close-on-select="true" :clear-on-select="false"
          :preserve-search="true" placeholder="Entites" label="template" track-by="eid" :preselect-first="false"
          @input="dmService.onEidSelect($event)">
        </vue-multiselect>
      </div>
    </div>
  </div>

  <b-tabs content-class="mt-3">
    <b-tab title="Damage Parts" active>
      <DMParts :parts="dmService.parts" />
    </b-tab>
    <b-tab title="Meta Parts">
      <DMMetaParts />
    </b-tab>
    <b-tab title="Damage Effects">
      <DMEffects />
    </b-tab>
    <b-tab title="Events">
      <DMEvents />
    </b-tab>
  </b-tabs>
</template>