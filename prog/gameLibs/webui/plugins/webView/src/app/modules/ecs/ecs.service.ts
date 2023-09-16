import { Injectable } from '@angular/core';

import { Observable } from 'rxjs/Observable';
import { Subscription } from 'rxjs/Subscription';

import { StreamService } from '../../core/stream.service';
import { CoreService } from '../../core/core.service';

import * as common from '../../core/common';
import { THIS_EXPR } from '@angular/compiler/src/output/output_ast';
import { JSONP_ERR_NO_CALLBACK } from '@angular/common/http/src/jsonp';
import { retry } from 'rxjs/operator/retry';

const UPDATE_INTERVAL_MS: number = 500;

interface IECSPromise
{
  resolve: Array<Function>;
  reject: Array<Function>;
}

function fixBool(v)
{
  if (typeof v !== 'object')
  {
    if (v === 'true') return true;
    if (v === 'false') return false;

    return v;
  }

  for (let k in v)
  {
    v[k] = fixBool(v[k]);
  }

  return v;
}

@Injectable()
export class ECSService implements common.IMessageListener
{
  private _templates: any = [];
  private _templatesByName: any = {};
  private _systems: any = [];
  private _queries: any = [];
  private _components: any = [];
  private _entities: any = [];
  private _entitiesByEid: any = {};
  private _dynamicQueryEntities: any = [];
  private _events: any = [];
  private _filter: string = '';
  private _filterEid: string = '';
  private _filteredEntities: any = null;
  private _filterComponent: string = '';
  private _filteredComponents: any = null;
  private _filterTemplate: string = '';
  private _filteredTemplates: any = null;
  private _filterSystem: string = '';
  private _filterQuery: string = '';
  private _filterEvent: string = '';
  private _filteredSystems: any = null;
  private _filteredQueries: any = null;
  private _filteredEvents: any = null;
  private _resolveEid: common.Dictionary<IECSPromise> = {};
  private _dynamicQuery: any = { comps_ro:[], comps_rq:[], comps_no:[] };
  private _dynamicQueryFilter: string = '';
  private _dynamicQueryColumns: any = [];

  private _fullDataCounter = 0;

  constructor(protected coreService: CoreService)
  {
    console.log('ECSService::ECSService');
    this.coreService.addListener(this);

    const query = this.coreService.cookieService.getObject('_dynamicQuery');
    if (query)
    {
      this._dynamicQuery = query;
    }

    const queryFilter = this.coreService.cookieService.get('_dynamicQueryFilter');
    if (queryFilter)
    {
      this._dynamicQueryFilter = queryFilter;
    }
  }

  fetchData()
  {
    this._fullDataCounter += 5;
    this.coreService.sendCommand('getTemplates');
    this.coreService.sendCommand('getSystems');
    this.coreService.sendCommand('getComponents');
    this.coreService.sendCommand('getQueries');
    this.coreService.sendCommand('getEvents');
  }

  onOpen(): void
  {
    console.log('ECSService::onOpen');

    this.fetchData();

    {
      ++this._fullDataCounter;
      this.coreService.sendCommand('getEntities');
    }
  }

  findTemplate(name: string): any
  {
    for (let n of name.split('+'))
    {
      const res = this._templatesByName[n];
      if (!!res)
      {
        return res;
      }
    }
  }

  findTemplateIndex(name: string): any
  {
    for (let n of name.split('+'))
    {
      const res = this._templates.findIndex(t => t.name === n);
      if (res >= 0)
      {
        return res;
      }
    }
  }

  onMessage(data): void
  {
    if (data.selectEid)
    {
      this.filterEntitiesByEid = data.selectEid;
      this.fetchEntities();
    }

    if (data.templates)
    {
      console.time('data.templates');
      --this._fullDataCounter;

      this._templatesByName = {};
      this._templates = data.templates;
      this._templates.forEach(t => t.components.sort((a, b) => a.name < b.name ? -1 : a.name > b.name ? 1 : 0));
      this._templates.forEach(t => { t._systems = []; t._queries = []; this._templatesByName[t.name] = t; });
      this._templates.forEach(t => t.parents.forEach(p => p._template = this.findTemplate(p.name)));

      const countComponents = (t, level = 0, acc = { componentsCount: 0, trackedCount: 0, replicatedCount: 0 }) => {
        acc.componentsCount += t.components.length;
        acc.trackedCount += t.components.reduce((res, v) => res + (v.isTracked ? 1 : 0), 0);
        acc.replicatedCount += t.components.reduce((res, v) => res + (v.isReplicated ? 1 : 0), 0);

        t.parents.forEach(p => countComponents(p._template, level + 1, acc));

        if (level === 0)
        {
          t._componentsCount = acc.componentsCount;
          t._trackedCount = acc.trackedCount;
          t._replicatedCount = acc.replicatedCount;
        }
      };
      this._templates.forEach(t => countComponents(t));
      console.timeEnd('data.templates');
    }
    if (data.systems)
    {
      --this._fullDataCounter;
      this._systems = data.systems;
      this._systems.forEach(s => s.events.sort())
    }
    if (data.queries)
    {
      --this._fullDataCounter;
      this._queries = data.queries;
    }
    if (data.components)
    {
      --this._fullDataCounter;
      this._components = data.components;
    }
    if (data.events)
    {
      --this._fullDataCounter;
      this._events = data.events;
    }
    if (data.dynamicQueryEntities)
    {
      console.time('data.dynamicQueryEntities');
      console.log(data.dynamicQueryEntities);
      this._dynamicQueryEntities = data.dynamicQueryEntities
      if (this._dynamicQueryEntities.length > 0) {
        const e = this._dynamicQueryEntities[0]
        this._dynamicQueryColumns = e.components.map(c => c.name)
        this._dynamicQueryEntities.forEach(function (e) {
          for (let c of e.components) {
            e[`_${c.name}`] = common.compToString(c.value)
          }
        })
        this._dynamicQueryEntities.forEach(e => e.components.forEach(fixBool))
      }
      else {
        this._dynamicQueryColumns = []
      }
      console.timeEnd('data.dynamicQueryEntities');
    }
    if (data.entities)
    {
      console.time('data.entities');
      --this._fullDataCounter;
      this._entitiesByEid = {};
      this._entities = data.entities.map(v => { v.id = v.eid; this._entitiesByEid[v.eid] = v; v._systemsFn = () => []; v._queriesFn = () => []; return v; });
      if (this._filterEid !== null && this._filterEid !== '')
      {
        this.filterEntitiesByEid = this._filterEid;
      }
      else
      {
        this.filterEntities = this._filter;
      }
      console.timeEnd('data.entities');
    }
    if (data.components)
    {
      console.time('data.components');
      let entity = this._entities.find(v => v.eid == data.eid);
      if (entity)
      {
        entity.components = data.components.sort((a, b) => a.name < b.name ? -1 : a.name > b.name ? 1 : 0);
        const toh = v => ('0' + v.toString(16)).substr(-2);
        entity.components.filter(v => v.type === 'E3DCOLOR').forEach(v => v._color = '#' + toh(v.value.r) + toh(v.value.g) + toh(v.value.b) + toh(typeof v.value.a !== undefined ? v.value.a : 255));
        entity.components.forEach(fixBool);
      }
      let promise = this._resolveEid[data.eid];
      if (promise)
      {
        promise.resolve.forEach(cb => cb(entity));
        delete this._resolveEid[data.eid];
      }
      console.timeEnd('data.components');
    }

    if (this._fullDataCounter === 0 && this._components.length > 0 && this._systems.length > 0 && this._templates.length > 0)
    {
      this._fullDataCounter = -1;

      console.time('dataProcess:systems');
      this._queries = this._queries.filter(q => this._systems.findIndex(s => s.name === q.name) < 0)
      this._systems.forEach(s => { s.componentsRO.forEach(c => c._mode = 'RO'); s.componentsRW.forEach(c => c._mode = 'RW'); s.componentsRQ.forEach(c => c._mode = 'RQ'); s.componentsNO.forEach(c => c._mode = 'NO')})
      this._queries.forEach(s => { s.componentsRO.forEach(c => c._mode = 'RO'); s.componentsRW.forEach(c => c._mode = 'RW'); s.componentsRQ.forEach(c => c._mode = 'RQ'); s.componentsNO.forEach(c => c._mode = 'NO')})
      this._systems.forEach(s => s._allComponents = [].concat(s.componentsRO, s.componentsRW, s.componentsRQ, s.componentsNO))
      this._queries.forEach(s => s._allComponents = [].concat(s.componentsRO, s.componentsRW, s.componentsRQ, s.componentsNO))
      console.timeEnd('dataProcess:systems');

      console.time('dataProcess:components');
      for (let c of this._components) {
        c._systems = [];
        c._queries = [];

        for (let sys of this._systems) {
          let compInSys = sys._allComponents.find(v => c.name === v.name)
          if (compInSys) {
            c._systems.push({system: sys, mode: compInSys._mode})
          }
        }
        c._systems.sort((a, b) => a.name < b.name ? -1 : a.name > b.name ? 1 : 0)

        for (let q of this._queries) {
          let compInQuery = q._allComponents.find(v => c.name === v.name)
          if (compInQuery) {
            c._queries.push({query: q, mode: compInQuery._mode})
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
            for (let parent of template.parents)
            {
              let parentId = this.findTemplateIndex(parent.name);
              let parentData = this._templates[parentId];

              const idx = parentData.components.findIndex(v => v.name === self.name);
              if (idx >= 0)
              {
                self._templates.push({ templateId: id, sourceTemplateId: parentId });
                break;
              }

              searchTempate(id, parentData);
            }
          };

          for (let templateId = 0; templateId < this._templates.length; ++templateId) {
            const idx = this._templates[templateId].components.findIndex(v => v.name == self.name);
            if (idx >= 0) {
              self._templates.push({ templateId: templateId, sourceTemplateId: templateId });
              break;
            }

            searchTempate(templateId, this._templates[templateId]);
          }
          return self._templates;
        };
      }
      console.timeEnd('dataProcess:components');

      console.time('dataProcess:entities');
      if (this._entities.length > 0)
      {
        const addSystemToTemplate = (sys: any, template_name: string) => {
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

        const addQueryToTemplate = (sys: any, template_name: string) => {
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
          t._systems = t._systems.reduce((res, s) => res.findIndex(r => r.name === s.name) >=0 ? res : [].concat(s, res), []);
          t._queries = t._queries.reduce((res, s) => res.findIndex(r => r.name === s.name) >=0 ? res : [].concat(s, res), []);
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

      console.log('Templates', this._templates);
      console.log('Components', this._components);
      console.log('Systems', this._systems);
      console.log('Queries', this._queries);
      console.log('Events', this._events);
    }
  }

  fetchEntities(withComponent: string = null): void
  {
    if (withComponent !== null)
    {
      ++this._fullDataCounter;
      this.coreService.sendCommand('getEntities', { withComponent: withComponent });
      this.fetchData();
    }
    else
    {
      ++this._fullDataCounter;
      this.coreService.sendCommand('getEntities');
      this.fetchData();
    }
  }

  fetchEntityAttributes(eid): Promise<any>
  {
    return new Promise((resolve, reject) =>
    {
      if (!this._resolveEid[eid])
      {
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

  setEntityAttribute(eid, attr, value): void
  {
    const toBlk = v =>
    {
      const isSet = v => typeof v !== 'undefined';

      if (typeof v !== 'object')
      {
        if (v === 'true') return true;
        if (v === 'false') return false;

        return v;
      }
      if (isSet(v.r) && isSet(v.g) && isSet(v.b) && isSet(v.a)) return { c : [v.r, v.g, v.b, v.a] };
      if (isSet(v.x) && isSet(v.y) && isSet(v.z) && isSet(v.w)) return { p4 : [v.x, v.y, v.z, v.w] };
      if (isSet(v.x) && isSet(v.y) && isSet(v.z)) return { p3 : [v.x, v.y, v.z] };
      if (isSet(v.x) && isSet(v.y)) return { p2 : [v.x, v.y] };

      for (let k in v)
      {
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
      TMatrix: v => { return { tm: [[v[0], v[1], v[2]], [v[3], v[4], v[5]], [v[6], v[7], v[8]], [v[9], v[10], v[11]]] } },
      'ecs::Object': v =>
      {
        let arr = attr.name.split('.');
        let obj = {};
        if (arr.length === 1)
        {
          obj[`${attr.name}:object`] = v;
        }
        else
        {
          obj[arr[0]] = { [`${ arr[1]}:object`]: v };
        }
        return obj;
      },
      'ecs::Array': v =>
      {
        const tmpVal = v.map(vv => toBlk(vv));
        const typeKey = typeof tmpVal[0] === 'object' ? Object.keys(tmpVal[0])[0] : null;

        const k2 = typeof tmpVal[0] === 'object' && ['c', 'p2', 'p3', 'p4', 'i', 'r', 'eid'].indexOf(typeKey) < 0 ? `${attr.name}:object` : attr.name;
        return { [`${attr.name}:array`]: {[k2]: tmpVal} };
      },
      'ecs::EntityId': v => { return {i: v} }
    };

    const valueToSend = convert[attr.type] ? convert[attr.type](value) : value;
    const message = common.jsonToBLK({ cmd: "setEntityAttribute", eid: { i: eid }, comp: attr.name, type: attr.type, value: valueToSend }, { markBlockWithTypes: true });

    console.log('setEntityAttribute', attr, value, valueToSend);
    console.log(message);

    attr.value = value;
    this.coreService.streamService.sendString(message);
  }

  performDynamicQuery()
  {
    this.coreService.cookieService.putObject('_dynamicQuery', this._dynamicQuery);
    this.coreService.cookieService.put('_dynamicQueryFilter', this._dynamicQueryFilter);

    let query = JSON.parse(JSON.stringify(this._dynamicQuery))
    query.filter = this._dynamicQueryFilter
    console.log(common.jsonToBLK(query))
    this.coreService.sendCommand('performDynamicQuery', query)
  }

  set filterEntities(template: string)
  {
    if (this._filterEid !== null && this._filterEid === '')
    {
      this.filterEntitiesByEid = null;
    }

    this._filter = template;

    if (template === null || template === '')
    {
      this._filteredEntities = null;
      return;
    }

    let re = new RegExp(template, 'i');
    this._filteredEntities = this._entities.filter(e => re.test(e.template));
  }

  set filterEntitiesByEid(eid: string)
  {
    if (this._filter !== null && this._filter !== '')
    {
      this.filterEntities = null;
    }

    this._filterEid = eid;

    if (eid === null || eid === '')
    {
      this._filteredEntities = null;
      return;
    }

    this._filteredEntities = this._entities.filter(v => v.eid == eid);
  }

  set filterComponents(name: string)
  {
    this._filterComponent = name;

    if (name === null || name === '')
    {
      this._filteredComponents = null;
      return;
    }

    let re = new RegExp(name, 'i');
    this._filteredComponents = this._components.filter(c => re.test(c.name));
  }

  set filterTemplates(name: string)
  {
    this._filterTemplate = name;

    if (name === null || name === '')
    {
      this._filteredTemplates = null;
      return;
    }

    let re = new RegExp(name, 'i');
    this._filteredTemplates = this._templates.filter(t => re.test(t.name));
  }

  set filterSystems(name: string)
  {
    this._filterSystem = name;

    if (name === null || name === '')
    {
      this._filteredSystems = null;
      return;
    }

    let re = new RegExp(name, 'i');
    this._filteredSystems = this._systems.filter(s => re.test(s.name));
  }

  set filterQueries(name: string)
  {
    this._filterQuery = name;

    if (name === null || name === '')
    {
      this._filteredQueries = null;
      return;
    }

    let re = new RegExp(name, 'i');
    this._filteredQueries = this._queries.filter(q => re.test(q.name));
  }

  set filterEvents(name: string)
  {
    this._filterEvent = name;

    if (name === null || name === '')
    {
      this._filteredEvents = null;
      return;
    }

    let re = new RegExp(name, 'i');
    this._filteredEvents = this._events.filter(e => re.test(e.name));
  }

  get templates() { return this._filteredTemplates === null ? this._templates : this._filteredTemplates; }
  get allTemplates() { return this._templates; }
  get systems() { return this._filteredSystems === null ? this._systems : this._filteredSystems; }
  get allSystems() { return this._systems; }
  get queries() { return this._filteredQueries === null ? this._queries : this._filteredQueries; }
  get allQueries() { return this._queries; }
  get events() { return this._filteredEvents === null ? this._events : this._filteredEvents; }
  get allEvents() { return this._events; }
  get components() { return this._filteredComponents === null ? this._components : this._filteredComponents; }
  get allComponents() { return this._components; }
  get entities() { return this._filteredEntities === null ? this._entities : this._filteredEntities; }
  get allEntities() { return this._entities; }
  get dynamicQueryEntities() { return this._dynamicQueryEntities; }

  get filterEntities() { return this._filter; }
  get filterEntitiesByEid() { return this._filterEid; }
  get filterComponents() { return this._filterComponent; }
  get filterTemplates() { return this._filterTemplate; }
  get filterSystems() { return this._filterSystem; }
  get filterQueries() { return this._filterQuery; }
  get filterEvents() { return this._filterEvent; }

  set dynamicQuery(value) { this._dynamicQuery = value; }
  get dynamicQuery() { return this._dynamicQuery; }

  set dynamicQueryFilter(value) { this._dynamicQueryFilter = value; }
  get dynamicQueryFilter() { return this._dynamicQueryFilter; }

  get dynamicQueryColumns() { return this._dynamicQueryColumns; }
}