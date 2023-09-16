import { Component, OnInit, OnDestroy } from '@angular/core';
import { StreamService, deserializeBSON } from '../core/stream.service';
import { CoreService } from '../core/core.service';

import { Observable } from 'rxjs/Observable';
import { Subscription } from 'rxjs/Subscription';

export interface Dictionary<T>
{
  [K: string]: T;
}

export function compToString(v, pretty:boolean = false): string
{
  let formatFloat = (v) => { return parseFloat(v).toFixed(3); };
  let isSet = (v) => { return typeof v !== 'undefined'; }

  if (typeof v !== 'object') return '' + v;
  if (isSet(v.r) && isSet(v.g) && isSet(v.b) && isSet(v.a)) return '(' + [v.r, v.g, v.b, v.a].join(', ') + ')';
  if (isSet(v.x) && isSet(v.y) && isSet(v.z) && isSet(v.w)) return '(' + [formatFloat(v.x), formatFloat(v.y), formatFloat(v.z), formatFloat(v.w)].join(', ') + ')';
  if (isSet(v.x) && isSet(v.y) && isSet(v.z)) return '(' + [formatFloat(v.x), formatFloat(v.y), formatFloat(v.z)].join(', ') + ')';
  if (isSet(v.x) && isSet(v.y)) return '(' + [formatFloat(v.x), formatFloat(v.y)].join(', ') + ')';
  if (Array.isArray(v) && v.length == 12) return compToString({ x: v[0], y: v[1], z: v[2] }) + "\n" + compToString({ x: v[3], y: v[4], z: v[5] }) + "\n" + compToString({ x: v[6], y: v[7], z: v[8] }) + "\n" + compToString({ x: v[9], y: v[10], z: v[11] });
  if (isSet(v.r)) return formatFloat(v.r);
  if (isSet(v.i)) return v.i;
  if (isSet(v.eid)) return v.eid;
  return pretty ? JSON.stringify(v, null, 2) : JSON.stringify(v);
}

export abstract class BaseComponent implements OnInit, OnDestroy
{
  constructor(public coreService: CoreService)
  { 
  }

  ngOnInit(): void
  {
    console.log('BaseComponent::OnInit', this.constructor.name);
  }

  ngOnDestroy(): void 
  {
    console.log('BaseComponent::OnDestroy');
  }

  sendCommand(cmd: string, params: any = null): void
  {
    this.coreService.sendCommand(cmd, params);
  }

  runConsoleRaw(data: string): void
  {
    let cmd;
    let params = {};
    let args = data.split(' ');
    cmd = args[0];
    for (let i = 1; i < args.length; ++i)
    {
      params['arg' + (i - 1)] = args[i];
    }
    this.runConsole(cmd, params);
  }

  runConsole(cmd: string, params: any = null): void
  {
    let tmp = params || {};
    tmp.command = cmd;
    this.sendCommand('console', tmp);
  }

  runConsoleScript(commands: any): void
  {
    for (let cmd of commands)
    {
      this.runConsole(cmd[0], cmd[1] ? cmd[1] : null);
    }
  }

  formatValue(v, pretty:boolean = false): string
  {
    return compToString(v, pretty)
  }

  runCommand(cmd: string): void 
  {
    this.sendCommand('runCommand', { command: cmd });
  }

  buildAndSendCommand(command)
  {
    this.coreService.buildAndSendCommand(command);
  }

  trackById(index, item)
  {
    return item.id;
  }

  trackByEid(index, item)
  {
    return item.eid;
  }

  trackByName(index, item)
  {
    return item.name;
  }

  trackByValue(index, item)
  {
    return item;
  }

  open(content, opts?)
  {
    this.coreService.modalService.open(content, opts).result.then(result => { }, reason => { });
  }

  get isConnected() { return this.coreService.isConnected; };
}

export interface IMessageListener
{
  onOpen(): void;
  onMessage(data): void;
}

export function valueToBLK(type: string, value: any, key: string, options: any): string
{
  if (key && key[0] === '_' && !key.endsWith(":object") && !key.endsWith(":array"))
    return '';

  const wrap = v => '"'+ v + '"';

  if (type === 'string')
  {
    return wrap(key) + ':t="' + value + '"';
  }
  if (type === 'number')
  {
    return wrap(key) + ':r=' + value;
  }
  if (type === 'boolean')
  {
    return wrap(key) + ':b=' + (value ? 'yes' : 'no');
  }
  if (type === 'object')
  {
    if (typeof value.toBlk == 'function')
      return valueToBLK('object', value.toBlk(), key, options);

    let isSet = (v) => { return typeof v !== 'undefined'; }

    if (isSet(value.r) && isSet(value.g) && isSet(value.b) && isSet(value.a)) return `${wrap(key)}:c=${value.r},${value.g},${value.b},${value.a}`;
    if (isSet(value.x) && isSet(value.y) && isSet(value.z) && isSet(value.w)) return `${wrap(key)}:p4=${value.x},${value.y},${value.z},${value.w}`;
    if (isSet(value.x) && isSet(value.y) && isSet(value.z)) return `${wrap(key)}:p3=${value.x},${value.y},${value.z}`;
    if (isSet(value.x) && isSet(value.y)) return `${wrap(key)}:p2=${value.x},${value.y}`;

    let subtype: string = Object.keys(value)[0];
    let subvalue = value[subtype];
    if (subtype === 'r')
    {
      return wrap(key) + ':' + subtype + '=' + subvalue;
    }
    if (subtype === 'i')
    {
      return wrap(key) + ':' + subtype + '=' + subvalue;
    }
    if (subtype === 'eid')
    {
      return `"${key}:eid"{value:i=${subvalue};}`;
    }
    if (subtype === 'p2' || subtype === 'p3' || subtype === 'p4' || subtype === 'c')
    {
      return wrap(key) + ':' + subtype + '=' + subvalue.join(',');
    }
    if (subtype === 'm')
    {
      return wrap(key) + ':' + subtype + '=[[' + subvalue[0].join(',') + '] [' + subvalue[1].join(',') + '] [' + subvalue[2].join(',') + '] [' + subvalue[3].join(',') + ']]';
    }
    if (subtype === 'array')
    {
      let tmp = '';
      for (let item of subvalue)
      {
        tmp += wrap(key) + '{' + jsonToBLK(item) + '}';
        tmp += "\n";
      }
      return tmp;
    }

    if (Array.isArray(value))
    {
      let res = '';
      for (let itm of value)
      {
        res += valueToBLK(typeof itm, itm, key, options) + "\n";
      }
      if (options.markBlockWithTypes && !key.endsWith(":object") && !key.endsWith(":array"))
        res = `"${key}:array"{ ${res} }`
      return res;
    }

    let block = options.markBlockWithTypes && !key.endsWith(":object") && !key.endsWith(":array") ? `"${key}:object"` : `"${key}"`
    return block + '{' + jsonToBLK(value, options) + '}';
  }
  return '';
}

export function jsonToBLK(json: any, options = { markBlockWithTypes: false }): string
{
  let blk = "";
  if (Array.isArray(json))
  {
    for (let itm of json)
    {
      blk += valueToBLK(typeof itm, itm, "array", options) + "\n";
    }
  }
  else
  {
    for (let key in json)
    {
      let value = json[key];
      let type: string = null;

      blk += valueToBLK(typeof value, value, key, options);
      blk += "\n";
    }
  }

  return blk;
}

export class Point2
{
  x: number = 0.0;
  y: number = 0.0;

  toBlk(): any
  {
    return { p2: [this.x, this.y] };
  }
}

export class Point3
{
  constructor(public x: number = 0, public y: number = 0, public z: number = 0)
  {
  }

  toBlk(): any
  {
    return { p3: [this.x, this.y, this.z] };
  }
}

export class Point4
{
  x: number = 0.0;
  y: number = 0.0;
  z: number = 0.0;
  w: number = 0.0;

  toBlk(): any
  {
    return { p4: [this.x, this.y, this.z, this.w] };
  }
}

export class TMatrix
{
  col0: Point3 = new Point3;
  col1: Point3 = new Point3;
  col2: Point3 = new Point3;
  col3: Point3 = new Point3;

  toBlk(): any
  {
    return { tm: [this.col0.toBlk().p3, this.col1.toBlk().p3, this.col2.toBlk().p3, this.col3.toBlk().p3] };
  }

  static fromArray(data: any): TMatrix
  {
    let mat = new TMatrix;
    mat.col0 = new Point3(data[0], data[1], data[2]);
    mat.col1 = new Point3(data[3], data[4], data[5]);
    mat.col2 = new Point3(data[6], data[7], data[8]);
    mat.col3 = new Point3(data[9], data[10], data[11]);

    return mat;
  }
}

export class BLKArray
{
  constructor(public data: any)
  {
  }

  toBlk(): any
  {
    let tmp = [];
    for (let d of this.data)
    {
      if (typeof d.toBlk === 'function')
      {
        tmp.push(d.toBlk());
      }
      else
      {
        tmp.push(d);
      }
    }
    return { array: tmp };
  }
}