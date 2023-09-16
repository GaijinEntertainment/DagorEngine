import { Injectable } from '@angular/core';
import { NgbModal, ModalDismissReasons } from '@ng-bootstrap/ng-bootstrap';

import { Observable } from 'rxjs/Observable';
import { Subscription } from 'rxjs/Subscription';
import { map } from 'rxjs/operator/map';
import { debounceTime } from 'rxjs/operator/debounceTime';
import { distinctUntilChanged } from 'rxjs/operator/distinctUntilChanged';

import { StreamService, deserializeBSON } from './stream.service';

import { CookieService } from 'ngx-cookie';
import { UUID } from 'angular2-uuid';

import * as common from './common';

const UPDATE_INTERVAL_MS: number = 500;

@Injectable()
export class CoreService
{
  private _listeners: Array<common.IMessageListener> = [];

  private _observable: Observable<any> = null;

  private _messagesObservable: Observable<any> = null;

  private _updateCount: number = 0;

  private _updateTimeout: NodeJS.Timer = null;

  private _commonParams: any = {};

  private _commandDomains: string[] = [];
  private _commands: any = [];

  private _consoleCommands: any = [];
  private _consoleCommandNames: any = [];
  private _searchConsoleCommand: Function = null;

  private _lastCommandName: string = '';
  private _lastCommand: any = null;

  constructor(public streamService: StreamService, public cookieService: CookieService, public modalService: NgbModal)
  {
    console.log('CoreService::CoreService');

    this._searchConsoleCommand = (text$: Observable<string>) =>
    {
      return map.call(distinctUntilChanged.call(debounceTime.call(text$, 200)),
        term => term.length < 2 ? [] : this._consoleCommandNames.filter(v => v.toLowerCase().indexOf(term.toLowerCase()) > -1).slice(0, 10));
    };

    this._lastCommandName = this.cookieService.get("_lastCommandName");
    if (!this._lastCommandName)
      this._lastCommandName = '';

    this.streamService.onOpen(() => { this.processOpen(); });

    this._observable = Observable.create(observer =>
    {
      this.streamService.onMessage((message) => { this.processMessage(observer, message); });
    });

    this._observable.subscribe(value => { this.processValue(value); });

    if (this.streamService.isConnected())
      this.onOpen();
  }

  addListener(listener: common.IMessageListener)
  {
    this._listeners.push(listener);

    if (this.streamService.isConnected())
      listener.onOpen();
  }

  onOpen(): void
  {
    console.log('CoreService::onOpen');

    this.sendCommand('onOpen');
    this.sendCommand('getConsoleCommandsList');

    if (!this._commandDomains.length)
      this.sendCommand('getCommandsList');

    for (let listener of this._listeners)
    {
      listener.onOpen();
    }
  }

  onMessage(data): void
  {
    if (data._update)
    {
      ++this._updateCount;
    }

    if (data.commonParams)
      this._commonParams = data.commonParams;

    if (data.commandsJson && !this._commandDomains.length)
    {
      this._commands = JSON.parse(data.commandsJson);

      if (this._commands.common)
      {
        this._commands.common = this._commands.common.filter(v => v.cmd !== '__sample');
        if (!this._commands.common.length)
          delete this._commands.common;
      }

      this._commandDomains = Object.keys(this._commands);

      if (this._lastCommandName !== '')
      {
        for (let domain of this._commandDomains)
        {
          for (let command of this._commands[domain])
          {
            let cmd = command.console ? command.console : command.cmd;
            if (this._lastCommandName === cmd)
            {
              this._lastCommandName;
              this._lastCommand = command;
              break;
            }
          }
        }
      }
    }

    if (data.consoleCommands)
    {
      this._consoleCommands = data.consoleCommands;
      this._consoleCommandNames = this._consoleCommands.map(v => v.name + ' x'.repeat(v.minArgs - 1) + ' [x]'.repeat(v.maxArgs - v.minArgs));
    }

    for (let listener of this._listeners)
    {
      listener.onMessage(data);
    }
  }

  continueUpdate(force: boolean = false): void
  {
    if (force)
    {
      if (this._updateTimeout != null)
        clearTimeout(this._updateTimeout);
      this._updateTimeout = null;
    }

    if (this._updateTimeout === null)
      this._updateTimeout = global.setTimeout(() => { this.sendCommand('update'); }, UPDATE_INTERVAL_MS);
  }

  processOpen(): void
  {
    this.onOpen();
    this.continueUpdate(true);
  }

  processMessage(observer, message): void
  {
    if (typeof message === 'object' && message.constructor && message.constructor.name === 'Blob')
    {
      let reader = new FileReader();
      reader.addEventListener("loadend", () =>
      {
        observer.next(deserializeBSON(new Uint8Array(reader.result)));
        this.continueUpdate();
      });
      reader.readAsArrayBuffer(message);
    }
    else
    {
      let json = {};
      try
      {
        json = JSON.parse(message);
      }
      catch (err)
      {
        json = { log: message };
      }
      observer.next(json);
    }
  }

  processValue(data): void
  {
    this.onMessage(data);

    if (data._update)
    {
      if (this._updateTimeout !== null)
        clearTimeout(this._updateTimeout);
      this._updateTimeout = null;
    }
  }

  sendCommand(cmd: string, params: any = null): void
  {
    let tmp = params || {};
    tmp.cmd = cmd;
    this.streamService.send(tmp);
  }

  pollData(dataGetter: Function, cmd, params = null): Promise<any>
  {
    return new Promise((resolve, reject) =>
    {
      let isDataValid = d => { return (typeof (d.length) !== undefined && d.length > 0) || (typeof (d.length) === undefined && d); }

      let data = dataGetter();
      if (isDataValid(data))
      {
        resolve(data);
        return;
      }

      this.sendCommand(cmd, params);
      let pollId = global.setInterval(() =>
      {
        let data = dataGetter();
        if (isDataValid(data))
        {
          global.clearInterval(pollId);
          resolve(data);
        }
      }, 100);
    });
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

  buildAndSendCommand(command)
  {
    this._lastCommandName = command.console ? command.console : command.cmd;
    this._lastCommand = command;
    this.cookieService.put("_lastCommandName", this._lastCommandName);

    if (command.console)
    {
      let consoleStr = command.console;

      for (let p of command.params)
      {
        if (p.value === undefined)
          continue;

        if (Array.isArray(p.value))
          consoleStr += ' ' + (<Array<any>>p.value).concat(' ');
        else if (typeof (p.value) === 'boolean')
          consoleStr += ' ' + (p.value ? 'on' : 'off');
        else
          consoleStr += ' ' + p.value;
      }

      console.log(command, consoleStr);

      this.runConsoleRaw(consoleStr);

      return;
    }

    let params = {};

    for (let p of command.params)
    {
      if (p.value === undefined)
        continue;

      let type = p.type;
      if (['dmPart', 'enum', 'b'].indexOf(p.type) >= 0)
      {
        params[p.name] = p.value;
      }
      else
      {
        params[p.name] = {};
        params[p.name][type] = p.value;
      }
    }

    console.log(command, params);

    this.sendCommand(command.cmd, params);
  }

  resetTimeSpeed()
  {
    this._commonParams['timespeed'] = 1;
  }

  get isConnected() { return this.streamService.isConnected(); };

  get updateCount(): number { return +this._updateCount; }

  set messagesObservable(value) { this._messagesObservable = value; }
  get messagesObservable() { return this._messagesObservable; }

  get observable() { return this._observable; }

  get commonParams() { return this._commonParams; }

  get commandDomains() { return this._commandDomains; }
  get commands() { return this._commands; }

  get consoleCommands() { return this._consoleCommands; }
  get searchConsoleCommand() { return this._searchConsoleCommand; }

  get lastCommand() { return this._lastCommand; }
}