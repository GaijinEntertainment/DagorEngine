<script setup>
const { dmService } = Vue.services;
</script>

<template>
  <table class="table table-sm table-bordered table-hover">
    <thead>
      <th>id</th>
      <th>part</th>
      <th>hp</th>
      <th>partsCount</th>
      <th>fireParamsId</th>
      <th>effects</th>
    </thead>
    <tbody>
      <tr v-for="part in dmService.metaParts">
        <td>{{ part.id }}</td>
        <td>
          <div v-for="n in dmService.getPartNamesForMetaPart(part)">{{ n }}</div>
        </td>
        <td>{{ part.hp }}</td>
        <td>{{ part.partsCount }}</td>
        <td>{{ part.fireParamsId }}</td>
        <td>
          <table class="table table-sm table-bordered table-hover">
            <thead>
              <th>conditionHp</th>
              <th>conditionDamage</th>
              <th>conditionDamageType</th>
              <th>conditionPartsLeft</th>
              <th>conditionPartsDead</th>
              <th>fire</th>
              <th>kill</th>
              <th>killEffect</th>
            </thead>
            <tbody>
              <tr v-for="eff in part.effects">
                <td>{{ eff.conditionHp }}</td>
                <td>{{ eff.conditionDamage }}</td>
                <td>{{ eff.conditionDamageType }}</td>
                <td>{{ eff.conditionPartsLeft }}</td>
                <td>{{ eff.conditionPartsDead }}</td>
                <td>{{ eff.fire }}</td>
                <td>{{ eff.kill }}</td>
                <td>{{ eff.killEffect }}</td>
              </tr>
            </tbody>
          </table>
        </td>
      </tr>
    </tbody>
  </table>
</template>