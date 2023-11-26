import { BLKArray } from './common.js';

const { ref, reactive } = Vue;

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

  _debugDrawModes = {};

  _weaponList = [];

  _curWeaponName = '';
  _ammoSpeed = 200;
  _fuseDelay = 2;

  _drawDamagedParts = false;
  _drawKilledParts = false;

  eids = reactive([]);
  _eids = [];

  _shouldShowAllEvents = false;

  events = reactive([]);
  _events = [];
  eventRendererElementsNames = [];
  eventRendererElementsMask = [];

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
    this._debugDrawModes = {};

    this.parts.splice(0, this.parts.length);
    this.metaParts.splice(0, this.metaParts.length);
    this.effects.splice(0, this.effects.length);
    this.events.splice(0, this.events.length);

    this.coreService.sendCommand('getWeapons');
    this.coreService.sendCommand('getEntities', { withComponent: 'dm_debug' });
  }

  onMessage(data) {
    if (data.allParts)
      this._allParts = data.allParts;
    if (data.nodes)
      this._nodes = data.nodes;
    if (data.parts) {
      this._parts = data.parts.filter(v => v.enabled);
      this._drawKilledParts = false;
      this._drawDamagedParts = false;

      this.parts.splice(0, this.parts.length, ...this._parts);
    }
    if (data.metaParts) {
      this._metaParts = data.metaParts;
      this.metaParts.splice(0, this.metaParts.length, ...this._metaParts);
    }
    if (data.dmEffects) {
      this._effects = data.dmEffects;
      this.effects.splice(0, this.effects.length, ...this._effects);
    }
    if (data.collNodes)
      this._collNodes = data.collNodes;
    if (data.dmgNodes)
      this._dmgNodes = data.dmgNodes;
    if (data.wreckedParts)
      this._wreckedParts = data.wreckedParts;
    if (data.debugDrawModes)
      this._debugDrawModes = data.debugDrawModes;
    if (data.weaponList) {
      this._weaponList = data.weaponList;
      if (!this._curWeaponName && this._weaponList[0]) {
        this._curWeaponName = this._weaponList[0];
        this.coreService.cookieService.put('_currentWeapon', this._curWeaponName);
        this.coreService.cookieService.put('_currentSpeed', this._ammoSpeed.toString());
        this.coreService.cookieService.put('_currentFuse', this._fuseDelay.toString());
      }
    }

    if (data.entities_dm_debug) {
      this._eids = data.entities_dm_debug;
      console.log('DMService::onMessage', this._eids);
      this.eids.splice(0, this.eids.length, ...this._eids);
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
    if (data.onPartHit) {
      let part = this._parts.find(v => v.id == data.onPartHit.partId);
      if (part) {
        part.hp = data.onPartHit.hp;
        part.hpRel = data.onPartHit.hpRel;

        if (this._drawKilledParts && part.hpRel == 0.0) {
          part.draw = true;
          this.coreService.sendCommand('drawPart', { part: part.name, draw: part.draw });
        }
        else if (this._drawDamagedParts && part.hpRel != 1.0) {
          part.draw = true;
          this.coreService.sendCommand('drawPart', { part: part.name, draw: part.draw });
        }
      }
    }
    if (typeof data.shouldShowAllEvents !== 'undefined') {
      this._shouldShowAllEvents = data.shouldShowAllEvents;
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

  onEidSelect($event) {
    if ($event) {
      this.coreService.sendCommand('setCurrentEid', { eid: { i: $event.eid } });
    }
  }

  onChangeEventRendererElementsMask() {
    this.coreService.sendCommand('setEventRendererElementsMask', new BLKArray(this.eventRendererElementsMask));
  }

  get allParts() { return this._allParts; }
  get nodes() { return this._nodes; }
  get parts() { return this._parts; }
  // get metaParts() { return this._metaParts; }
  // get effects() { return this._effects; }
  get collNodes() { return this._collNodes; }
  get dmgNodes() { return this._dmgNodes; }
  get wreckedParts() { return this._wreckedParts; }
  get debugDrawModes() { return this._debugDrawModes; }

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

  get shouldShowAllEvents() { return this._shouldShowAllEvents; }
  set shouldShowAllEvents(value) {
    this._shouldShowAllEvents = value;
    this.coreService.sendCommand('shouldShowAllEvents', { value: value });
  }
}