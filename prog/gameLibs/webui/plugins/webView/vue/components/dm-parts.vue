<script setup>
const { ref, computed, watch, defineProps } = Vue;
const { coreService, dmService } = Vue.services;
const { sortValues } = Vue.helpers;

const props = defineProps({
  parts: Array,
});

function getHPColor(rel_hp) {
  if (rel_hp == 0)
    return 'maroon';
  if (rel_hp < 0.25)
    return 'red';
  if (rel_hp < 0.5)
    return 'orange';
  if (rel_hp != 1)
    return 'gold';
  return 'green';
}

function toggleDrawPart(part) {
  part.draw = !part.draw;
  coreService.sendCommand('drawPart', { part: part.name, draw: part.draw });
}

function runEffect(part_no, expl) {
  coreService.sendCommand('runEffect', { id: part_no, expl: expl });
}

function hitPart(part_name, once = false) {
  coreService.sendCommand('hitPart', { part: part_name, once: once });
}

</script>

<template>
  <div class="container-fluid m-0 border-bottom">
    <div class="row">
      <SortableColumn @sort="sortValues(parts, 'id')" columnTitle="ID" columnClass="col-sm-1 p-0" />
      <SortableColumn @sort="sortValues(parts, 'name')" columnTitle="Name" columnClass="col-sm-7 p-0" />
      <SortableColumn @sort="sortValues(parts, 'hp')" columnTitle="HP" columnClass="col-sm-1 p-0" />
      <SortableColumn @sort="sortValues(parts, 'hpFull')" columnTitle="Full HP" columnClass="col-sm-1 p-0" />
      <div class="col-sm-1 p-0">Action</div>
      <div class="col-sm-1 p-0">Draw</div>
    </div>
  </div>

  <RecycleScroller class="scroller" :items="parts" :item-size="32" key-field="id">
    <template #default="props">
      <div
        :class="['container-fluid', 'm-0', 'scroller-row', 'border-bottom', props.index % 2 == 0 ? 'scroller-row-even' : 'scroller-row-odd']">
        <div class="row">
          <div class="col-sm-1 p-0">{{ props.item.id }}</div>
          <div class="col-sm-7 p-0">{{ props.item.name }}</div>
          <div class="col-sm-1 p-0">
            <span :style="{ 'color': getHPColor(props.item.hpRel) }">{{ props.item.hp.toFixed(2) }}</span>
          </div>
          <div class="col-sm-1 p-0">{{ props.item.hpFull }}</div>
          <div class="col-sm-1 p-0">
            <button type="button" class="btn btn-sm btn-outline-primary" @click="hitPart(props.item.name);">kill</button>
            <button type="button" class="btn btn-sm btn-outline-primary"
              @click="hitPart(props.item.name, true);">damage</button>
          </div>
          <div class="col-sm-1 p-0">
            <button type="button" class="btn btn-sm btn-outline-primary" :class="{ 'bg-success': props.item.draw }"
              @click="toggleDrawPart(props.item)">
              <span v-if="props.item.draw">Hide</span>
              <span v-if="!props.item.draw">Draw</span>
            </button>
          </div>
        </div>
      </div>
    </template>
  </RecycleScroller>
</template>