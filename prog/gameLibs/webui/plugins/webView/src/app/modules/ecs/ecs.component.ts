import { Component, OnInit, OnDestroy, ViewChild, Pipe, PipeTransform, TemplateRef } from '@angular/core';

import * as common from '../../core/common';

import { StreamService, deserializeBSON } from '../../core/stream.service';
import { CoreService } from '../../core/core.service';
import { ECSService } from './ecs.service';

import { Observable } from 'rxjs/Observable';
import { CookieService } from 'ngx-cookie';
import { DatatableComponent, NgxDatatableModule } from '@swimlane/ngx-datatable';
import { JsonEditorOptions, JsonEditorComponent } from 'ang-jsoneditor';

const UPDATE_INTERVAL_MS = 500;

@Pipe({ name: 'attrValue' })
export class AttrValuePipe implements PipeTransform {
  transform(attr: any) {
    if (attr.type === 'E3DCOLOR')
    {
      console.log('pipe', attr.value);
      const toh = v => ('0' + v.toString(16)).substr(-2);
      return '#' + toh(attr.value.r) + toh(attr.value.g) + toh(attr.value.b);
    }
    return attr.value;
  }
}

@Component
  ({
    selector: 'ecs',
    templateUrl: './ecs.component.html',
    styleUrls: ['./ecs.component.css']
  })
export class ECSComponent extends common.BaseComponent implements OnInit, OnDestroy
{
  @ViewChild('ecsTable') table: DatatableComponent;
  @ViewChild('ecsComponentsTable') componentsTable: DatatableComponent;
  @ViewChild('ecsTemplatesTable') templatesTable: DatatableComponent;
  @ViewChild('ecsSystemsTable') systemsTable: DatatableComponent;
  @ViewChild('ecsQueriesTable') queriesTable: DatatableComponent;
  @ViewChild('ecsEventsTable') eventsTable: DatatableComponent;
  @ViewChild('ecsDynamicQueryTable') dynamicQueryTableTable: DatatableComponent;

  @ViewChild('ecsInfoPopup') ecsInfoPopupTemplate: TemplateRef<any>;
  @ViewChild('systemsRow') systemsRowTemplate: TemplateRef<any>;
  @ViewChild('queriesRow') queriesRowTemplate: TemplateRef<any>;
  @ViewChild('eventsRow') eventsRowTemplate: TemplateRef<any>;

  currentRow: any = {};
  currentAttr: any = {};

  currentTemplate: TemplateRef<any>;
  currentContext: any = {};

  editorOptions: JsonEditorOptions;

  constructor(public coreService: CoreService, public ecsService: ECSService)
  {
    super(coreService);

    this.editorOptions = new JsonEditorOptions();
    this.editorOptions.modes = ['code', 'tree'];
  }

  ngOnInit(): void 
  {
    console.log('ECSComponent::OnInit');
    super.ngOnInit();
  }

  ngOnDestroy(): void 
  {
    console.log('ECSComponent::OnDestroy');
    super.ngOnDestroy();
  }

  getRowHeight(row)
  {
    return 50;
  }

  getRowDetailHeight(row)
  {
    if (!row)
      return;
    let h = 80 + 30 * (row.components ? row.components.length : 0);
    let h1 = 80 + 19 * (row._systemsFn ? row._systemsFn().length : 0);
    let h2 = 80 + 19 * (row._queriesFn ? row._queriesFn().length : 0);
    return Math.max(h, h1 + h2);
  }

  getRowDetailHeightComponent(row)
  {
    if (!row)
      return;

    let h = 80;
    if (row._templatesFn) { h += 30 * row._templatesFn().length; }

    let h1 = 80;
    if (row._systems) { h1 += 30 * row._systems.length; }

    let h2 = 80;
    if (row._queries) { h1 += 30 * row._queries.length; }

    return Math.max(h, h1, h2);
  }

  getRowDetailHeightTemplate(row)
  {
    if (!row)
      return;

    let h = 80 + 30;

    if (row.components) { h += 30 * row.components.length; }
    if (row.parents)
    {
      let calcHeight = p => { h += 30 + 30 * p._template.components.length; p._template.parents.forEach(p => calcHeight(p)); }
      row.parents.forEach(p => calcHeight(p));
    }

    let h1 = 80 + 19 * (row._systems ? row._systems.length : 0);
    let h2 = 80 + 19 * (row._queries ? row._queries.length : 0);

    return Math.max(h, h1 + h2);
  }

  getRowDetailHeightSystem(row)
  {
    if (!row)
      return;
    let h = 80;
    if (row.componentsRW && row.componentsRW.length) { h += 30 + 30 * row.componentsRW.length; }
    if (row.componentsRO && row.componentsRO.length) { h += 30 + 30 * row.componentsRO.length; }
    if (row.componentsRQ && row.componentsRQ.length) { h += 30 + 30 * row.componentsRQ.length; }
    if (row.componentsNO && row.componentsNO.length) { h += 30 + 30 * row.componentsNO.length; }

    let h1 = 80;
    if (row._templateNames) { h1 += 30 * row._templateNames.length; }

    let h2 = 80;
    if (row.events) { h2 += 30 * row.events.length; }

    return Math.max(h, h1, h2);
  }

  getRowDetailHeightQuery(row)
  {
    if (!row)
      return;
    let h = 80;
    if (row.componentsRW && row.componentsRW.length) { h += 30 + 30 * row.componentsRW.length; }
    if (row.componentsRO && row.componentsRO.length) { h += 30 + 30 * row.componentsRO.length; }
    if (row.componentsRQ && row.componentsRQ.length) { h += 30 + 30 * row.componentsRQ.length; }
    if (row.componentsNO && row.componentsNO.length) { h += 30 + 30 * row.componentsNO.length; }

    let h1 = 80;
    if (row._templateNames) { h1 += 30 * row._templateNames.length; }

    return Math.max(h, h1);
  }

  getRowDetailHeightEvent(row)
  {
    if (!row)
      return;
    return 80 + 30 * (row._systems ? row._systems.length : 0);
  }

  fetchEnities(): void
  {
    this.ecsService.fetchEntities();
  }

  toggleExpandRow(row, expanded)
  {
    console.log('Toggled Expand Row!', row, expanded);

    if (expanded) {
      this.table.rowDetail.toggleExpandRow(row);
    }
    else {
      this.ecsService.fetchEntityAttributes(row.eid).then((entity) => {
        console.log('onFetchEntityAttributes', entity);
        console.time('EntityAttributesExpand');
        this.table.rowDetail.toggleExpandRow(row);
        console.timeEnd('EntityAttributesExpand');
      });
    }
  }

  toggleExpandRowDynamicQuery(row, expanded)
  {
    console.log('Toggled Expand Row!', row, expanded);

    if (expanded) {
      this.dynamicQueryTableTable.rowDetail.toggleExpandRow(row);
    }
    else {
      this.dynamicQueryTableTable.rowDetail.toggleExpandRow(row);
    }
  }

  toggleExpandRowComponent(row)
  {
    console.log('Toggled Expand Row!', row);
    this.componentsTable.rowDetail.toggleExpandRow(row);
  }

  toggleExpandRowTemplate(row)
  {
    console.log('Toggled Expand Row!', row);
    this.templatesTable.rowDetail.toggleExpandRow(row);
  }

  toggleExpandRowSystem(row)
  {
    console.log('Toggled Expand Row!', row);
    this.systemsTable.rowDetail.toggleExpandRow(row);
  }

  toggleExpandRowQuery(row)
  {
    console.log('Toggled Expand Row!', row);
    this.queriesTable.rowDetail.toggleExpandRow(row);
  }

  toggleExpandRowEvent(row)
  {
    console.log('Toggled Expand Row!', row);
    this.eventsTable.rowDetail.toggleExpandRow(row);
  }

  onDetailToggle(event)
  {
    console.log('Detail Toggled', event);
  }

  updateFilter(event)
  {
    const val = event.target.value.toLowerCase();
    this.ecsService.filterEntities = val;
    this.table.offset = 0;
  }

  updateFilterByEid(event)
  {
    const val = event.target.value.toLowerCase();
    this.ecsService.filterEntitiesByEid = val;
    this.table.offset = 0;
  }

  updateFilterComponents(event)
  {
    const val = event.target.value.toLowerCase();
    this.ecsService.filterComponents = val;
    this.componentsTable.offset = 0;
  }

  updateFilterTemplates(event)
  {
    const val = event.target.value.toLowerCase();
    this.ecsService.filterTemplates = val;
    this.templatesTable.offset = 0;
  }

  updateFilterSystems(event)
  {
    const val = event.target.value.toLowerCase();
    this.ecsService.filterSystems = val;
    this.systemsTable.offset = 0;
  }

  updateFilterQueries(event)
  {
    const val = event.target.value.toLowerCase();
    this.ecsService.filterQueries = val;
    this.queriesTable.offset = 0;
  }

  updateFilterEvents(event)
  {
    const val = event.target.value.toLowerCase();
    this.ecsService.filterEvents = val;
    this.eventsTable.offset = 0;
  }

  shouldOpenEditor(comp: any)
  {
    return [
      'ecs::Object',
      'ecs::Array',
      'ecs::IntList',
      'ecs::UInt16List',
      'ecs::StringList',
      'ecs::EidList',
      'ecs::FloatList',
      'ecs::Point2List',
      'ecs::Point3List',
      'ecs::Point4List',
      'ecs::IPoint2List',
      'ecs::IPoint3List',
      'ecs::IPoint4List',
      'ecs::BoolList',
      'ecs::ColorList',
      'ecs::TMatrixList',
      'ecs::Int64List',
    ].indexOf(comp.type) >= 0;
  }

  getAttrValue(attr: any)
  {
    if (attr.type === 'E3DCOLOR')
    {
      const toh = v => ('0' + v.toString(16)).substr(-2);
      return '#' + toh(attr.value.r) + toh(attr.value.g) + toh(attr.value.b);
    }
    return attr.value;
  }

  openAttr(row, attr, content)
  {
    if (this.shouldOpenEditor(attr) || attr.type === 'E3DCOLOR')
    {
      this.currentRow = row;
      this.currentAttr = attr;
      this.open(content);
    }
  }

  startEdit(attr)
  {
    if (attr.type === 'E3DCOLOR')
    {
      return;
    }

    attr._edit = true;
    attr._new_value = null;

    console.log('startEdit', attr);
  }

  cancelEdit(attr)
  {
    attr._edit = false;
    attr._new_value = null;
  }

  saveEdit(row, attr)
  {
    if (row.eid === undefined)
    {
      return;
    }

    console.log('saveEdit', attr, attr._new_value);

    attr._edit = false;
    if (attr._new_value !== null && attr._new_value !== undefined)
    {
      this.ecsService.setEntityAttribute(row.eid, attr, attr._new_value);
    }
  }

  updateValue(ev, row, val, attr, key = null)
  {
    const setByKey = v => 
    {
      let res = {};
      let curVal = attr._new_value === null ? attr.value : attr._new_value;
      for (let k in curVal)
      {
        res[k] = curVal[k];
      }
      res[key] = +val;
      return res;
    };

    const convert =
      {
        E3DCOLOR: v =>
        {
          const toi = (v, s) => parseInt(v.substring(1 + s * 2, 3 + s * 2), 16);
          return {
            r: toi(attr._color, 0), g: toi(attr._color, 1), b: toi(attr._color, 2), a: toi(attr._color, 3)
          }
        },
        int: v => setByKey(v),
        float: v => setByKey(v),
        Point2: v => setByKey(v),
        Point3: v => setByKey(v),
        Point4: v => setByKey(v),
      };

    const valueToSet = convert[attr.type] ? convert[attr.type](val) : val;

    console.log('updateValue', attr, valueToSet);

    attr._new_value = valueToSet;

    console.log(ev);
    if (ev.code === "Enter" || ev.type === "change") {
      this.saveEdit(row, attr);
    }
    else if (ev.code === "Escape") {
      this.cancelEdit(attr);
    }
  }

  saveObject(row, attr, editor: JsonEditorComponent = null)
  {
    if (row.eid === undefined)
    {
      return;
    }

    if (editor)
    {
      this.updateValue({}, row, editor.get(), attr);
      this.ecsService.setEntityAttribute(row.eid, attr, attr._new_value);
    }
    else
    {
      this.saveEdit(row, attr);
    }
  }

  getDisplayForEditor(): string
  {
    return this.currentAttr.type === 'E3DCOLOR' ? 'none' : 'block';
  }

  clickSystem(sys)
  {
    this.currentTemplate = this.systemsRowTemplate;
    this.currentContext = { $implicit: sys };

    this.open(this.ecsInfoPopupTemplate, { size: 'lg' });
  }

  clickQuery(q)
  {
    this.currentTemplate = this.queriesRowTemplate;
    this.currentContext = { $implicit: q };

    this.open(this.ecsInfoPopupTemplate, { size: 'lg' });
  }

  clickEvents(q)
  {
    this.currentTemplate = this.eventsRowTemplate;
    this.currentContext = { $implicit: q };

    this.open(this.ecsInfoPopupTemplate, { size: 'lg' });
  }

  saveDynamicQuery(editor: JsonEditorComponent = null, filter: HTMLInputElement = null)
  {
    if (filter)
    {
      this.ecsService.dynamicQueryFilter = filter.value;
    }
    if (editor)
    {
      this.ecsService.dynamicQuery = editor.get();
    }
  }

  performDynamicQuery()
  {
    this.ecsService.performDynamicQuery()
  }

  get entities() { return this.ecsService.entities; }
  get dynamicQueryEntities() { return this.ecsService.dynamicQueryEntities; }
  get components() { return this.ecsService.components; }
  get systems() { return this.ecsService.systems; }
  get queries() { return this.ecsService.queries; }
  get events() { return this.ecsService.events; }
  get templates() { return this.ecsService.templates; }

  set dynamicQuery(value) { this.ecsService.dynamicQuery = value; }
  get dynamicQuery() { return this.ecsService.dynamicQuery; }

  set dynamicQueryFilter(value) { this.ecsService.dynamicQueryFilter = value; }
  get dynamicQueryFilter() { return this.ecsService.dynamicQueryFilter; }

  get dynamicQueryColumns() { return this.ecsService.dynamicQueryColumns; }
}
