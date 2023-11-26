import { jsonToBLK, compToString } from './common.js';

const { ref, reactive } = Vue;

function fixBool(v) {
  if (typeof v !== 'object') {
    if (v === 'true') return true;
    if (v === 'false') return false;

    return v;
  }

  for (let k in v) {
    v[k] = fixBool(v[k]);
  }

  return v;
}

export class ECSService {
  templates = reactive([]);
  _templates = [];
  _templatesByName = {};
  systems = reactive([]);
  _systems = [];
  queries = reactive([]);
  _queries = [];
  components = reactive([]);
  _components = [];
  entities = reactive([]);
  _entities = [];
  _entitiesByEid = {};
  dynamicQueryEntities = reactive([]);
  _dynamicQueryEntities = [];
  events = reactive([]);
  _events = [];
  _filter = '';
  _filterEid = '';
  _filteredEntities = null;
  _filterComponent = '';
  _filteredComponents = null;
  _filterTemplate = '';
  _filteredTemplates = null;
  _filterSystem = '';
  _filterQuery = '';
  _filterEvent = '';
  _filteredSystems = null;
  _filteredQueries = null;
  _filteredEvents = null;
  _resolveEid = {};
  _dynamicQuery = { comps_ro: [], comps_rq: [], comps_no: [] };
  dynamicQueryFilter = ref('');
  dynamicQueryColumns = reactive([]);
  _dynamicQueryColumns = [];

  _fullDataCounter = 0;

  constructor(coreService) {
    console.log('ECSService::ECSService');

    this.coreService = coreService;

    this.coreService.addListener(this);

    const query = this.coreService.cookieService.get('_dynamicQuery');
    if (query) {
      this._dynamicQuery = query;
    }

    const queryFilter = this.coreService.cookieService.get('_dynamicQueryFilter');
    if (queryFilter) {
      this.dynamicQueryFilter.value = queryFilter;
    }
  }

  fetchData() {
    this._fullDataCounter += 5;
    this.coreService.sendCommand('getTemplates');
    this.coreService.sendCommand('getSystems');
    this.coreService.sendCommand('getComponents');
    this.coreService.sendCommand('getQueries');
    this.coreService.sendCommand('getEvents');
  }

  onOpen() {
    console.log('ECSService::onOpen');

    this.fetchData();

    ++this._fullDataCounter;
    this.coreService.sendCommand('getEntities');
  }

  findTemplate(name) {
    for (let n of name.split('+')) {
      const res = this._templatesByName[n];
      if (!!res) {
        return res;
      }
    }
  }

  findTemplateIndex(name) {
    for (let n of name.split('+')) {
      const res = this._templates.findIndex(t => t.name === n);
      if (res >= 0) {
        return res;
      }
    }
  }

  onMessage(data) {
    if (data.selectEid) {
      this.filterEntitiesByEid = data.selectEid;
      this.fetchEntities();
    }

    if (data.templates) {
      console.time('data.templates');
      --this._fullDataCounter;

      this._templatesByName = {};
      this._templates = data.templates;
      var curId = 0;
      this._templates.forEach(t => t.id = curId++);
      this._templates.forEach(t => t.components.sort((a, b) => a.name < b.name ? -1 : a.name > b.name ? 1 : 0));
      this._templates.forEach(t => { t._systems = []; t._queries = []; this._templatesByName[t.name] = t; });
      this._templates.forEach(t => t.parents.forEach(p => p._template = this.findTemplate(p.name)));

      const countComponents = (t, level = 0, acc = { componentsCount: 0, trackedCount: 0, replicatedCount: 0 }) => {
        acc.componentsCount += t.components.length;
        acc.trackedCount += t.components.reduce((res, v) => res + (v.isTracked ? 1 : 0), 0);
        acc.replicatedCount += t.components.reduce((res, v) => res + (v.isReplicated ? 1 : 0), 0);

        t.parents.forEach(p => countComponents(p._template, level + 1, acc));

        if (level === 0) {
          t._componentsCount = acc.componentsCount;
          t._trackedCount = acc.trackedCount;
          t._replicatedCount = acc.replicatedCount;
        }
      };
      this._templates.forEach(t => countComponents(t));
      console.timeEnd('data.templates');
    }
    if (data.systems) {
      --this._fullDataCounter;
      this._systems = data.systems;
      this._systems.forEach(s => s.events.sort())
    }
    if (data.queries) {
      --this._fullDataCounter;
      this._queries = data.queries;
    }
    if (data.components) {
      --this._fullDataCounter;
      this._components = data.components;
    }
    if (data.events) {
      --this._fullDataCounter;
      this._events = data.events;
    }
    if (data.dynamicQueryEntities) {
      console.time('data.dynamicQueryEntities');
      console.log(data.dynamicQueryEntities);
      this._dynamicQueryEntities = data.dynamicQueryEntities
      if (this._dynamicQueryEntities.length > 0) {
        const e = this._dynamicQueryEntities[0]
        this._dynamicQueryColumns = e.components.map(c => c.name)
        this._dynamicQueryEntities.forEach(function (e) {
          for (let c of e.components) {
            e[`_${c.name}`] = compToString(c.value)
          }
        })
        this._dynamicQueryEntities.forEach(e => e.components.forEach(fixBool))
      }
      else {
        this._dynamicQueryColumns = []
      }
      this.dynamicQueryColumns.splice(0, this.dynamicQueryColumns.length, ...this._dynamicQueryColumns);
      this.dynamicQueryEntities.splice(0, this.dynamicQueryEntities.length, ...this._dynamicQueryEntities);
      console.timeEnd('data.dynamicQueryEntities');
    }
    if (data.entities) {
      console.time('data.entities');
      --this._fullDataCounter;
      this._entitiesByEid = {};
      this._entities = data.entities.map(v => { v.id = v.eid; this._entitiesByEid[v.eid] = v; v._systemsFn = () => []; v._queriesFn = () => []; return v; });
      if (this._filterEid !== null && this._filterEid !== '') {
        this.filterEntitiesByEid = this._filterEid;
      }
      else {
        this.filterEntities = this._filter;
      }
      console.timeEnd('data.entities');
    }
    if (data.components) {
      console.time('data.components');
      let entity = this.entities.find(v => v.eid == data.eid);
      if (entity) {
        entity.components = data.components.sort((a, b) => a.name < b.name ? -1 : a.name > b.name ? 1 : 0);
        const toh = v => ('0' + v.toString(16)).substr(-2);
        entity.components.filter(v => v.type === 'E3DCOLOR').forEach(v => v._color = '#' + toh(v.value.r) + toh(v.value.g) + toh(v.value.b) + toh(typeof v.value.a !== undefined ? v.value.a : 255));
        entity.components.forEach(fixBool);
      }
      let promise = this._resolveEid[data.eid];
      if (promise) {
        promise.resolve.forEach(cb => cb(entity));
        delete this._resolveEid[data.eid];
      }
      console.timeEnd('data.components');
    }

    if (this._fullDataCounter === 0 && this._components.length > 0 && this._systems.length > 0 && this._templates.length > 0) {
      this._fullDataCounter = -1;

      console.time('dataProcess:systems');
      this._queries = this._queries.filter(q => this._systems.findIndex(s => s.name === q.name) < 0)
      this._systems.forEach(s => { s.componentsRO.forEach(c => c._mode = 'RO'); s.componentsRW.forEach(c => c._mode = 'RW'); s.componentsRQ.forEach(c => c._mode = 'RQ'); s.componentsNO.forEach(c => c._mode = 'NO') })
      this._queries.forEach(s => { s.componentsRO.forEach(c => c._mode = 'RO'); s.componentsRW.forEach(c => c._mode = 'RW'); s.componentsRQ.forEach(c => c._mode = 'RQ'); s.componentsNO.forEach(c => c._mode = 'NO') })
      this._systems.forEach(s => s._allComponents = [].concat(s.componentsRO, s.componentsRW, s.componentsRQ, s.componentsNO))
      this._queries.forEach(s => s._allComponents = [].concat(s.componentsRO, s.componentsRW, s.componentsRQ, s.componentsNO))
      console.timeEnd('dataProcess:systems');

      console.time('dataProcess:components');
      var compId = 0;
      for (let c of this._components) {
        c._systems = [];
        c._queries = [];
        c.id = compId++;

        for (let sys of this._systems) {
          let compInSys = sys._allComponents.find(v => c.name === v.name)
          if (compInSys) {
            c._systems.push({ system: sys, mode: compInSys._mode })
          }
        }
        c._systems.sort((a, b) => a.name < b.name ? -1 : a.name > b.name ? 1 : 0)

        for (let q of this._queries) {
          let compInQuery = q._allComponents.find(v => c.name === v.name)
          if (compInQuery) {
            c._queries.push({ query: q, mode: compInQuery._mode })
          }
        }
        c._queries.sort((a, b) => a.name < b.name ? -1 : a.name > b.name ? 1 : 0)

        let self = c;
        c._templatesFn = () => {
          if (self._templates !== undefined) {
            return self._templates;
          }

          self._templates = [];

          const searchTempate = (id, template) => {
            for (let parent of template.parents) {
              let parentId = this.findTemplateIndex(parent.name);
              let parentData = this._templates[parentId];

              const idx = parentData.components.findIndex(v => v.name === self.name);
              if (idx >= 0) {
                self._templates.push({ templateId: id, sourceTemplateId: parentId });
                // break;
              }

              searchTempate(id, parentData);
            }
          };

          for (let templateId = 0; templateId < this._templates.length; ++templateId) {
            const idx = this._templates[templateId].components.findIndex(v => v.name == self.name);
            if (idx >= 0) {
              self._templates.push({ templateId: templateId, sourceTemplateId: templateId });
              // break;
            }

            searchTempate(templateId, this._templates[templateId]);
          }

          return self._templates;
        };
      }
      console.timeEnd('dataProcess:components');

      console.time('dataProcess:entities');
      if (this._entities.length > 0) {
        const addSystemToTemplate = (sys, template_name) => {
          for (let n of template_name.split('+')) {
            let res = this._templatesByName[n];
            if (!!res) {
              res._systems.push(sys);
            }
          }
        };

        this._systems.forEach(s => {
          s._templates = {};
          s.entities.forEach(eid => {
            let e = this._entitiesByEid[eid];
            s._templates[e.template] = 1 + (s._templates[e.template] || 0);

            let self = e;
            e._systemsFn = () => {
              if (self._systems !== undefined) {
                return self._systems;
              }
              self._systems = [];
              this._systems.forEach(es => {
                if (es.entities.indexOf(self.eid) >= 0) {
                  self._systems.push(es);
                }
              });
              self._systems = self._systems.reduce((res, s) => res.findIndex(r => r.name === s.name) >= 0 ? res : [].concat(s, res), []);
              return self._systems;
            }
            addSystemToTemplate(s, e.template);
          });

          s._templateNames = Object.keys(s._templates);
        });

        const addQueryToTemplate = (sys, template_name) => {
          for (let n of template_name.split('+')) {
            let res = this._templatesByName[n];
            if (!!res) {
              res._queries.push(sys);
            }
          }
        }

        this._queries.forEach(s => {
          s._templates = {};
          s.entities.forEach(eid => {
            let e = this._entitiesByEid[eid];
            s._templates[e.template] = 1 + (s._templates[e.template] || 0);

            let self = e;
            e._queriesFn = () => {
              if (self._queries !== undefined) {
                return self._queries;
              }
              self._queries = [];
              this._queries.forEach(es => {
                if (es.entities.indexOf(self.eid) >= 0) {
                  self._queries.push(es);
                }
              });
              self._queries = self._queries.reduce((res, s) => res.findIndex(r => r.name === s.name) >= 0 ? res : [].concat(s, res), []);
              return self._queries;
            }
            addQueryToTemplate(s, e.template);
          });

          s._templateNames = Object.keys(s._templates);
        });

        this._templates.forEach(t => {
          t._systems = t._systems.reduce((res, s) => res.findIndex(r => r.name === s.name) >= 0 ? res : [].concat(s, res), []);
          t._queries = t._queries.reduce((res, s) => res.findIndex(r => r.name === s.name) >= 0 ? res : [].concat(s, res), []);
        });
      }
      console.timeEnd('dataProcess:entities');

      console.time('dataProcess:events');
      for (let e of this._events) {
        e._systems = []
        for (let sys of this._systems) {
          if (sys.events.indexOf(e.name) >= 0) {
            e._systems.push(sys)
          }
        }
        e._systems.sort((a, b) => a.name < b.name ? -1 : a.name > b.name ? 1 : 0)
      }
      console.timeEnd('dataProcess:events');

      this.entities.splice(0, this.entities.length, ...this._entities);
      this.components.splice(0, this.components.length, ...this._components);
      this.systems.splice(0, this.systems.length, ...this._systems);
      this.queries.splice(0, this.queries.length, ...this._queries);
      this.events.splice(0, this.events.length, ...this._events);
      this.templates.splice(0, this.templates.length, ...this._templates);

      console.log('Templates', this._templates);
      console.log('Components', this._components);
      console.log('Systems', this._systems);
      console.log('Queries', this._queries);
      console.log('Events', this._events);
    }
  }

  fetchEntities(withComponent = null) {
    console.log('fetchEntities', withComponent);
    if (withComponent !== null) {
      ++this._fullDataCounter;
      this.coreService.sendCommand('getEntities', { withComponent: withComponent });
      this.fetchData();
    }
    else {
      ++this._fullDataCounter;
      this.coreService.sendCommand('getEntities');
      this.fetchData();
    }
  }

  fetchEntityAttributes(eid) {
    return new Promise((resolve, reject) => {
      if (!this._resolveEid[eid]) {
        this._resolveEid[eid] =
        {
          resolve: [],
          reject: []
        };
      }

      this._resolveEid[eid].resolve.push(resolve);
      this._resolveEid[eid].reject.push(reject);

      if (this._fullDataCounter === 0) {
        this._fullDataCounter = -1;
      }
      this.coreService.sendCommand('getEntityAttributes', { eid: { i: eid } });
    });
  }

  setEntityAttribute(eid, attr, value) {
    const toBlk = v => {
      const isSet = v => typeof v !== 'undefined';

      if (typeof v !== 'object') {
        if (v === 'true') return true;
        if (v === 'false') return false;

        return v;
      }
      if (isSet(v.r) && isSet(v.g) && isSet(v.b) && isSet(v.a)) return { c: [v.r, v.g, v.b, v.a] };
      if (isSet(v.x) && isSet(v.y) && isSet(v.z) && isSet(v.w)) return { p4: [v.x, v.y, v.z, v.w] };
      if (isSet(v.x) && isSet(v.y) && isSet(v.z)) return { p3: [v.x, v.y, v.z] };
      if (isSet(v.x) && isSet(v.y)) return { p2: [v.x, v.y] };

      for (let k in v) {
        v[k] = toBlk(v[k]);
      }

      return v;
    }

    const convert =
    {
      Point2: v => { return { p2: [v.x, v.y] } },
      Point3: v => { return { p3: [v.x, v.y, v.z] } },
      Point4: v => { return { p4: [v.x, v.y, v.z, v.w] } },
      E3DCOLOR: v => { return { c: [v.r, v.g, v.b, v.a] } },
      TMatrix: v => { return { m: [[v[0], v[1], v[2]], [v[3], v[4], v[5]], [v[6], v[7], v[8]], [v[9], v[10], v[11]]] } },
      'ecs::Object': v => {
        let arr = attr.name.split('.');
        let obj = {};
        if (arr.length === 1) {
          obj[`${attr.name}:object`] = v;
        }
        else {
          obj[arr[0]] = { [`${arr[1]}:object`]: v };
        }
        return obj;
      },
      'ecs::Array': v => {
        const tmpVal = v.map(vv => toBlk(vv));
        const typeKey = typeof tmpVal[0] === 'object' ? Object.keys(tmpVal[0])[0] : null;

        const k2 = typeof tmpVal[0] === 'object' && ['c', 'p2', 'p3', 'p4', 'i', 'r', 'eid'].indexOf(typeKey) < 0 ? `${attr.name}:object` : attr.name;
        return { [`${attr.name}:array`]: { [k2]: tmpVal } };
      },
      'ecs::EntityId': v => { return { i: v } }
    };

    const valueToSend = convert[attr.type] ? convert[attr.type](value) : value;
    const message = jsonToBLK({ cmd: "setEntityAttribute", eid: { i: eid }, comp: attr.name, type: attr.type, value: valueToSend }, { markBlockWithTypes: true });

    console.log('setEntityAttribute', attr, value, valueToSend);
    console.log(message);

    attr.value = value;
    this.coreService.streamService.sendString(message);
  }

  saveDynamicQuery() {
    this.coreService.cookieService.put('_dynamicQuery', this._dynamicQuery);
    this.coreService.cookieService.put('_dynamicQueryFilter', this.dynamicQueryFilter.value);

    console.log(this._dynamicQuery);
  }

  performDynamicQuery() {
    this.saveDynamicQuery();

    let query = JSON.parse(JSON.stringify(this._dynamicQuery))
    query.filter = this.dynamicQueryFilter.value;
    console.log(jsonToBLK(query))
    this.coreService.sendCommand('performDynamicQuery', query)
  }

  set filterEntities(template) {
    if (this._filterEid !== null && this._filterEid === '') {
      this.filterEntitiesByEid = null;
    }

    this._filter = template;

    if (template === null || template === '') {
      this._filteredEntities = null;
      this.entities.splice(0, this.entities.length, ...this._entities);
      return;
    }

    let re = new RegExp(template, 'i');
    this._filteredEntities = this._entities.filter(e => re.test(e.template));

    this.entities.splice(0, this.entities.length, ...this._filteredEntities);
  }

  set filterEntitiesByEid(eid) {
    if (this._filter !== null && this._filter !== '') {
      this.filterEntities = null;
    }

    this._filterEid = eid;

    if (eid === null || eid === '') {
      this._filteredEntities = null;
      this.entities.splice(0, this.entities.length, ...this._entities);
      return;
    }

    this._filteredEntities = this._entities.filter(v => v.eid == eid);
    this.entities.splice(0, this.entities.length, ...this._filteredEntities);
  }

  set filterComponents(name) {
    this._filterComponent = name;

    if (name === null || name === '') {
      this._filteredComponents = null;
      this.components.splice(0, this.components.length, ...this._components);
      return;
    }

    let re = new RegExp(name, 'i');
    this._filteredComponents = this._components.filter(c => re.test(c.name));
    this.components.splice(0, this.components.length, ...this._filteredComponents);
  }

  set filterTemplates(name) {
    this._filterTemplate = name;

    if (name === null || name === '') {
      this._filteredTemplates = null;
      this.templates.splice(0, this.templates.length, ...this._templates);
      return;
    }

    let re = new RegExp(name, 'i');
    this._filteredTemplates = this._templates.filter(t => re.test(t.name));
    this.templates.splice(0, this.templates.length, ...this._filteredTemplates);
  }

  set filterSystems(name) {
    this._filterSystem = name;

    if (name === null || name === '') {
      this._filteredSystems = null;
      this.systems.splice(0, this.systems.length, ...this._systems);
      return;
    }

    let re = new RegExp(name, 'i');
    this._filteredSystems = this._systems.filter(s => re.test(s.name));
    this.systems.splice(0, this.systems.length, ...this._filteredSystems);
  }

  set filterQueries(name) {
    this._filterQuery = name;

    if (name === null || name === '') {
      this._filteredQueries = null;
      this.queries.splice(0, this.queries.length, ...this._queries);
      return;
    }

    let re = new RegExp(name, 'i');
    this._filteredQueries = this._queries.filter(q => re.test(q.name));
    this.queries.splice(0, this.queries.length, ...this._filteredQueries);
  }

  set filterEvents(name) {
    this._filterEvent = name;

    if (name === null || name === '') {
      this._filteredEvents = null;
      this.events.splice(0, this.events.length, ...this._events);
      return;
    }

    let re = new RegExp(name, 'i');
    this._filteredEvents = this._events.filter(e => re.test(e.name));
    this.events.splice(0, this.events.length, ...this._filteredEvents);
  }

  // get templates() { return this._filteredTemplates === null ? this._templates : this._filteredTemplates; }
  get allTemplates() { return this._templates; }
  // get systems() { return this._filteredSystems === null ? this._systems : this._filteredSystems; }
  get allSystems() { return this._systems; }
  // get queries() { return this._filteredQueries === null ? this._queries : this._filteredQueries; }
  get allQueries() { return this._queries; }
  // get events() { return this._filteredEvents === null ? this._events : this._filteredEvents; }
  get allEvents() { return this._events; }
  // get components() { return this._filteredComponents === null ? this._components : this._filteredComponents; }
  get allComponents() { return this._components; }
  // get entities() { return this._filteredEntities === null ? this._entities : this._filteredEntities; }
  get allEntities() { return this._entities; }
  // get dynamicQueryEntities() { return this._dynamicQueryEntities; }

  get filterEntities() { return this._filter; }
  get filterEntitiesByEid() { return this._filterEid; }
  get filterComponents() { return this._filterComponent; }
  get filterTemplates() { return this._filterTemplate; }
  get filterSystems() { return this._filterSystem; }
  get filterQueries() { return this._filterQuery; }
  get filterEvents() { return this._filterEvent; }

  set dynamicQuery(value) { this._dynamicQuery = value; }
  get dynamicQuery() { return this._dynamicQuery; }

  // get dynamicQueryColumns() { return this._dynamicQueryColumns; }
}