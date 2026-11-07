<script setup>
const { defineAsyncComponent } = Vue;
const { coreService, dmService } = Vue.services;

const DMParts = defineAsyncComponent(() => Vue.helpers.loadModule('./vue/components/dm-parts.vue'));
const DMMetaParts = defineAsyncComponent(() => Vue.helpers.loadModule('./vue/components/dm-meta-parts.vue'));
const DMEffects = defineAsyncComponent(() => Vue.helpers.loadModule('./vue/components/dm-effects.vue'));
const DMEvents = defineAsyncComponent(() => Vue.helpers.loadModule('./vue/components/dm-events.vue'));

function drawDamagedParts() {
  dmService.drawDamagedParts = !dmService.drawDamagedParts;
  dmService.drawKilledParts = false;

  for (let part of dmService.parts) {
    part.draw = dmService.drawDamagedParts && part.hpRel != 1.0;
    coreService.sendCommand('dmViewer.drawPart', { part: part.name, draw: part.draw });
  }
}

function drawKilledParts() {
  dmService.drawKilledParts = !dmService.drawKilledParts;
  dmService.drawDamagedParts = false;

  for (let part of dmService.parts) {
    part.draw = dmService.drawKilledParts && part.hpRel == 0.0;
    coreService.sendCommand('dmViewer.drawPart', { part: part.name, draw: part.draw });
  }
}
</script>

<style>
.clickable {
  cursor: pointer;
}

.dm-object-select .multiselect__option {
  min-height: 24px;
  padding: 4px 12px;
}

.dm-object-select .multiselect__option:after {
  line-height: 24px;
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
        <button type="button" class="btn btn-sm btn-outline-primary d-block mt-1"
          :class="{ 'active': dmService.drawCollisionNames.value }" @click="dmService.toggleDrawCollisionNames();">Draw names</button>
      </div>
      <div class="float-xs-left ml-1">
        <span>Select a DM object:</span>
        <vue-multiselect class="dm-object-select" v-model="dmService.currentObject.value" :options="dmService.objects" :close-on-select="true" :clear-on-select="false"
          :preserve-search="true" placeholder="DM objects" label="label" track-by="key" :preselect-first="false"
          :allow-empty="false" :max-height="600" :option-height="24" @input="dmService.onObjectSelect($event)">
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