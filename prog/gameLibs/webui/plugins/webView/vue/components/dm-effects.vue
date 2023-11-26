<script setup>
const { dmService } = Vue.services;
</script>

<template>
  <table class="table table-sm table-bordered table-hover">
    <thead>
      <th>id</th>
      <th>part</th>
      <th>relativeDamage</th>
      <th>onHitDefault</th>
      <th>onKillDefault</th>
      <th>onHitByDamageType</th>
      <th>onKillByDamageType</th>
    </thead>
    <tbody>
      <tr v-for="eff in dmService.effects">
        <td>{{ eff.id }}</td>
        <td>
          <div v-for="n in dmService.getPartNamesForEffect(eff)">{{ n }}</div>
        </td>
        <td>{{ eff.relativeDamage }}</td>
        <td>
          <table class="table table-sm table-bordered table-hover">
            <thead>
              <th>damageThreshold</th>
              <th>isCritical</th>
              <th>effects</th>
            </thead>
            <tbody>
              <tr v-for="effOfType in eff.onHitDefault">
                <td>{{ effOfType.damageThreshold }}</td>
                <td>{{ effOfType.isCritical }}</td>
                <td>
                  <div v-for="p in effOfType.parts">
                    <span class="badge badge-pill badge-dark">{{ p.type }}</span>:
                    {{ p.ratio.toFixed(2) }} <span v-if="p.partId >= 0">({{ p.partName }})</span>
                  </div>
                </td>
              </tr>
            </tbody>
          </table>
        </td>
        <td>
          <table class="table table-sm table-bordered table-hover">
            <thead>
              <th>damageThreshold</th>
              <th>isCritical</th>
              <th>effects</th>
            </thead>
            <tbody>
              <tr v-for="effOfType in eff.onKillDefault">
                <td>{{ effOfType.damageThreshold }}</td>
                <td>{{ effOfType.isCritical }}</td>
                <td>
                  <div v-for="p in effOfType.parts"><span class="badge badge-pill badge-dark">{{ p.type }}</span>:
                    {{ p.ratio.toFixed(2) }} <span v-if="p.partId >= 0">({{ p.partName }})</span></div>
                </td>
              </tr>
            </tbody>
          </table>
        </td>
        <td>
          <table v-for="t in dmService.getDamageTypesForEffect(eff.onHitByDamageType)"
            class="table table-sm table-bordered table-hover">
            <thead>
              <th><span class="badge badge-pill badge-info clickable"
                  (click)="dmService.toggleExpanded(eff.onHitByDamageType[t])">{{ t }}</span> <span
                  v-if="dmService.isExpanded(eff.onHitByDamageType[t])">damageThreshold</span></th>
              <th v-if="dmService.isExpanded(eff.onHitByDamageType[t])">isCritical</th>
              <th v-if="dmService.isExpanded(eff.onHitByDamageType[t])">effects</th>
            </thead>
            <tbody v-if="dmService.isExpanded(eff.onHitByDamageType[t])">
              <tr v-for="effOfType in eff.onHitByDamageType[t]">
                <td>{{ effOfType.damageThreshold }}</td>
                <td>{{ effOfType.isCritical }}</td>
                <td>
                  <div v-for="p in effOfType.parts"><span class="badge badge-pill badge-dark">{{ p.type }}</span>:
                    {{ p.ratio.toFixed(2) }} <span v-if="p.partId >= 0">({{ p.partName }})</span></div>
                </td>
              </tr>
            </tbody>
          </table>
        </td>
        <td>
          <table v-for="t in dmService.getDamageTypesForEffect(eff.onKillByDamageType)"
            class="table table-sm table-bordered table-hover">
            <thead>
              <th><span class="badge badge-pill badge-info clickable"
                  (click)="dmService.toggleExpanded(eff.onKillByDamageType[t])">{{ t }}</span> <span
                  v-if="dmService.isExpanded(eff.onKillByDamageType[t])">damageThreshold</span></th>
              <th v-if="dmService.isExpanded(eff.onKillByDamageType[t])">isCritical</th>
              <th v-if="dmService.isExpanded(eff.onKillByDamageType[t])">effects</th>
            </thead>
            <tbody v-if="dmService.isExpanded(eff.onKillByDamageType[t])">
              <tr v-for="effOfType in eff.onKillByDamageType[t]">
                <td>{{ effOfType.damageThreshold }}</td>
                <td>{{ effOfType.isCritical }}</td>
                <td>
                  <div v-for="p in effOfType.parts">
                    <span class="badge badge-pill badge-dark">{{ p.type }}</span>:
                    {{ p.ratio.toFixed(2) }} <span v-if="p.partId >= 0">({{ p.partName }})</span>
                  </div>
                </td>
              </tr>
            </tbody>
          </table>
        </td>
      </tr>
    </tbody>
  </table>
</template>