<script setup>
const { ref, watch, defineComponent } = Vue;
const { formatValue } = Vue.helpers;
const { dmService } = Vue.services;

const EventsTree = defineComponent({
  name: 'EventsTree',
  props: {
    nodes: Object,
    rootClass: String,
  },
  methods: {
    formatValue(value) {
      return formatValue(value);
    },
  },
  template: `
    <div class="container" :class="rootClass" style="min-width: 500px;">
      <template v-for="node in nodes">
        <div class="row">
          <b-button class="col-sm-1 p-0" style=" max-width: 32px;" v-if="node.children && node.children.length > 0" size="sm" variant="outline-primary" @click="node._opened = !node._opened">
            <i v-if="!node._opened" class="fa fa-plus"></i>
            <i v-if="node._opened" class="fa fa-minus"></i>
          </b-button>

          <span class="col-sm-5 p-0 font-weight-bold" :class="{ 'ml-2': node.children && node.children.length > 0 }">{{ node.name }}</span>
          <span class="col-sm-6 p-0" v-if="node.value">{{ formatValue(node.value) }}</span>
        </div>

        <template v-if="node._opened && node.children && node.children.length > 0">
          <EventsTree :nodes="node.children" rootClass="ml-4" />
        </template>
      </template>
    </div>
  `,
});

const selectedShowEvents = ref(false);
const showEventsOptions = [
  { text: 'Last Event', value: true },
  { text: 'All Events', value: false },
];

watch(selectedShowEvents, (val) => {
  console.log('selectedShowEvents', val);
  dmService.shouldShowAllEvents = val;
});
</script>

<template>
  <div class="d-flex d-flex-row">
    <div>
      <b-form-group>
        <b-form-radio-group v-model="selectedShowEvents" :options="showEventsOptions" button-variant="outline-primary"
          size="sm" name="radio-btn-outline" buttons></b-form-radio-group>
      </b-form-group>
      <div v-if="dmService.eventRendererElementsNames.length == dmService.eventRendererElementsMask.length">
        <div class="form-check" v-for="(item, i) in dmService.eventRendererElementsNames">
          <label class="form-check-label">
            <input class="form-check-input" type="checkbox" v-model="dmService.eventRendererElementsMask[i].value"
              @change="dmService.onChangeEventRendererElementsMask()">{{ item.name }}
          </label>
        </div>
      </div>
    </div>
    <div class="ml-1">
      <span>Events:</span>
      <EventsTree :nodes="dmService.events" />
    </div>
  </div>
</template>