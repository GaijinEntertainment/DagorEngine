import { BLKArray } from './common.js';

const { ref, reactive } = Vue;

const UPDATE_INTERVAL_MS = 500;

export class DMService {
  _allParts = [];
  _nodes = [];
  parts = reactive([]);
  _parts = [];
  metaParts = reactive([]);
  _metaParts = [];
  effects = reactive([]);
  _effects = [];
  _collNodes = [];
  _dmgNodes = [];
  _wreckedParts = [];

  _weaponList = [];

  _curWeaponName = '';
  _ammoSpeed = 200;
  _fuseDelay = 2;

  _drawDamagedParts = false;
  _drawKilledParts = false;
  drawCollisionNames = ref(false);

  objects = reactive([]);
  currentObject = ref(null);

  shownHitEvents = ref('all');

  events = reactive([]);
  _events = [];
  eventRendererElementsNames = [];
  eventRendererElementsMask = [];

  _updateTimeout = null;

  constructor(coreService) {
    console.log('DMService::DMService');

    this.coreService = coreService;

    this.coreService.addListener(this);

    let v = this.coreService.cookieService.get('_currentWeapon');
    if (v)
      this._curWeaponName = v;
    v = this.coreService.cookieService.get('_currentSpeed');
    if (v)
      this._ammoSpeed = +v;
    v = this.coreService.cookieService.get('_currentFuse');
    if (v)
      this._fuseDelay = +v;
  }

  onOpen() {
    console.log('DMService::onOpen');

    this._allParts = [];
    this._nodes = [];
    this._parts = [];
    this._collNodes = [];
    this._dmgNodes = [];
    this._wreckedParts = [];

    this.parts.splice(0, this.parts.length);
    this.metaParts.splice(0, this.metaParts.length);
    this.effects.splice(0, this.effects.length);
    this.events.splice(0, this.events.length);

    this.coreService.sendCommand('getWeapons');
    this.coreService.sendCommand('dmViewer.open');
    this.continueUpdate();
  }

  continueUpdate() {
    clearTimeout(this._updateTimeout);
    this._updateTimeout = setTimeout(() => { this.coreService.sendCommand('dmViewer.update'); }, UPDATE_INTERVAL_MS);
  }

  onMessage(data) {
    if (data.allParts)
      this._allParts = data.allParts;
    if (data.nodes)
      this._nodes = data.nodes;
    if (data.collNodes)
      this._collNodes = data.collNodes;
    if (data.dmgNodes)
      this._dmgNodes = data.dmgNodes;
    if (data.wreckedParts)
      this._wreckedParts = data.wreckedParts;
    if (data.weaponList) {
      this._weaponList = data.weaponList;
      if (!this._curWeaponName && this._weaponList[0]) {
        this._curWeaponName = this._weaponList[0];
        this.coreService.cookieService.put('_currentWeapon', this._curWeaponName);
        this.coreService.cookieService.put('_currentSpeed', this._ammoSpeed.toString());
        this.coreService.cookieService.put('_currentFuse', this._fuseDelay.toString());
      }
    }

    if (data.exec) {
      if (data.exec.shot) {
        this.sendShot();
      }
    }

    if (data.applyDamageEffect) {
      let part = this._parts.find(v => v.id == data.applyDamageEffect.partId);
      if (part) {
        if (!part.effects)
          part.effects = {};

        for (let eff in data.applyDamageEffect.effects) {
          part.effects[eff] = true;
          part.effectNames = Object.keys(part.effects);
        }
      }
    }

    if (data.dmViewer)
      this.onViewerMessage(data.dmViewer);
  }

  onViewerMessage(data) {
    if (data._update)
      this.continueUpdate();
    if (data.parts) {
      if (data._update)
        this.updatePartsHp(data.parts);
      else {
        this._parts = data.parts.filter(v => v.enabled);
        this._drawKilledParts = false;
        this._drawDamagedParts = false;

        this.parts.splice(0, this.parts.length, ...this._parts);
      }
    }
    if (data.metaParts) {
      this._metaParts = data.metaParts;
      this.metaParts.splice(0, this.metaParts.length, ...this._metaParts);
    }
    if (data.dmEffects) {
      this._effects = data.dmEffects;
      this.effects.splice(0, this.effects.length, ...this._effects);
    }
    if (data.dmObjects) {
      const toKey = v => `${v.type}:${v.nid}:${v.id}`;
      const objects = data.dmObjects.map(v => ({ ...v, key: toKey(v), label: v.name ? `${v.name} #${v.id}` : `#${v.id}` }));
      this.objects.splice(0, this.objects.length, ...objects);

      const targetKey = data.dmTarget ? toKey(data.dmTarget) : null;
      this.currentObject.value = objects.find(v => v.key === targetKey) || null;
    }
    if (typeof data.shownHitEvents !== 'undefined') {
      this.shownHitEvents.value = data.shownHitEvents;
    }
    if (typeof data.drawCollisionNames !== 'undefined') {
      this.drawCollisionNames.value = data.drawCollisionNames;
    }
    if (data.dmEvents && data.dmEvents.events) {
      console.log('DMService::onMessage', data.dmEvents.events);
      this._events = data.dmEvents.events;
      this.events.splice(0, this.events.length, ...this._events);
    }
    if (data.eventRendererElementsNames) {
      this.eventRendererElementsNames = data.eventRendererElementsNames;
    }
    if (data.eventRendererElementsMask) {
      this.eventRendererElementsMask = data.eventRendererElementsMask;
    }
  }

  updatePartsHp(parts) {
    const updates = new Map(parts.map(v => [v.id, v]));
    for (let part of this.parts) {
      const update = updates.get(part.id);
      if (!update || update.hpRel === part.hpRel)
        continue;

      part.hp = update.hp;
      part.hpRel = update.hpRel;
      if (!part.draw && ((this._drawKilledParts && part.hpRel == 0.0) || (this._drawDamagedParts && part.hpRel != 1.0))) {
        part.draw = true;
        this.coreService.sendCommand('dmViewer.drawPart', { part: part.name, draw: part.draw });
      }
    }
  }

  filterPartsByName(n) {
    return n ? this._parts.filter(v => v.name.indexOf(n) >= 0) : this._parts;
  }

  sendShot() {
    this.coreService.cookieService.put('_currentSpeed', this._ammoSpeed.toString());
    this.coreService.cookieService.put('_currentFuse', this._fuseDelay.toString());
    this.coreService.sendCommand('shot', { weapon: this._curWeaponName, speed: +this._ammoSpeed, fuseDelay: +this._fuseDelay });
  }

  getPartNamesForMetaPart(metaPart) {
    if (!this._parts.length || !this._metaParts.length)
      return [];

    if (!metaPart._parts)
      metaPart._parts = this._parts.filter(v => v.metapartId == metaPart.id);

    return metaPart._parts.map(v => v.name);
  }

  getPartNamesForEffect(eff) {
    if (!this._parts.length || !this._effects.length)
      return [];

    if (!eff._parts)
      eff._parts = this._parts.filter(v => v.effectPresetId == eff.id);

    return eff._parts.map(v => v.name);
  }

  getDamageTypesForEffect(eff) {
    if (!this._parts.length || !this._effects.length)
      return [];

    if (!eff._types)
      eff._types = Object.keys(eff);

    return eff._types;
  }

  isExpanded(v) {
    if (!v._expanded)
      v._expanded = false;
    return v._expanded;
  }

  toggleExpanded(v) {
    v._expanded = !v._expanded;
  }

  onShownHitEventsSelect(value) {
    this.coreService.sendCommand('dmViewer.setShownHitEvents', { value: value });
  }

  toggleDrawCollisionNames() {
    this.drawCollisionNames.value = !this.drawCollisionNames.value;
    this.coreService.sendCommand('dmViewer.setDrawCollisionNames', { draw: this.drawCollisionNames.value });
  }

  onObjectSelect(object) {
    if (object) {
      this.coreService.sendCommand('dmViewer.setCurrentObject', { object: { type: { i: object.type }, nid: { i: object.nid }, id: { i: object.id } } });
    }
  }

  onChangeEventRendererElementsMask() {
    this.coreService.sendCommand('dmViewer.setEventRendererElementsMask', new BLKArray(this.eventRendererElementsMask));
  }

  get allParts() { return this._allParts; }
  get nodes() { return this._nodes; }
  get parts() { return this._parts; }
  // get metaParts() { return this._metaParts; }
  // get effects() { return this._effects; }
  get collNodes() { return this._collNodes; }
  get dmgNodes() { return this._dmgNodes; }
  get wreckedParts() { return this._wreckedParts; }

  get weaponList() { return this._weaponList; }

  get curWeaponName() { return this._curWeaponName; }
  set curWeaponName(value) { this._curWeaponName = value; }
  get ammoSpeed() { return +this._ammoSpeed; }
  set ammoSpeed(value) { this._ammoSpeed = +value; }
  get fuseDelay() { return +this._fuseDelay; }
  set fuseDelay(value) { this._fuseDelay = +value; }

  get drawDamagedParts() { return this._drawDamagedParts; }
  set drawDamagedParts(value) { this._drawDamagedParts = value; }
  get drawKilledParts() { return this._drawKilledParts; }
  set drawKilledParts(value) { this._drawKilledParts = value; }

  get allPartsPromse() {
    return new Promise((resolve, reject) => {
      resolve(this._allParts);
    });
  }

  get weaponListPromise() {
    return this.coreService.pollData(() => this._weaponList, 'getWeapons');
  }
}