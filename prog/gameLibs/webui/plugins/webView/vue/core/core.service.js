import { deserializeBSON } from './common.js';

const { ref, reactive, computed, triggerRef } = Vue;
const { Observable } = rxjs;
const { map, debounceTime, distinctUntilChanged } = rxjs.operators;

const UPDATE_INTERVAL_MS = 500;

export class CoreService {
  _listeners = [];

  _observable = null;

  _messagesObservable = null;

  _updateCount = 0;

  _updateTimeout = null;

  commandDomains = reactive([]);
  commands = reactive({});

  _consoleCommands = [];
  _consoleCommandNames = [];

  _lastCommandName = '';
  _lastCommand = null;

  constructor(streamService, cookieService) {
    console.log('CoreService::CoreService');

    this.streamService = streamService;
    this.cookieService = cookieService;

    this._lastCommandName = this.cookieService.get("_lastCommandName");
    if (!this._lastCommandName)
      this._lastCommandName = '';

    this.streamService.onOpen(() => { this.processOpen(); });

    this._observable = Observable.create(observer => {
      this.streamService.onMessage((message) => { this.processMessage(observer, message); });
    });

    this._observable.subscribe(value => { this.processValue(value); });

    if (this.streamService.isConnected())
      this.onOpen();
  }

  addListener(listener) {
    this._listeners.push(listener);

    if (this.streamService.isConnected())
      listener.onOpen();
  }

  onOpen() {
    console.log('CoreService::onOpen');

    this.sendCommand('onOpen');
    this.sendCommand('getConsoleCommandsList');

    if (!this.commandDomains.length)
      this.sendCommand('getCommandsList');

    for (let listener of this._listeners) {
      listener.onOpen();
    }
  }

  onMessage(data) {
    if (data._update) {
      ++this._updateCount;
    }

    if (data.commandsJson && !this.commandDomains.length) {
      const _commands = JSON.parse(data.commandsJson);

      if (_commands.common) {
        _commands.common = _commands.common.filter(v => v.cmd !== '__sample');
        if (!_commands.common.length)
          delete _commands.common;
      }

      for (let c in _commands) {
        this.commands[c] = _commands[c];
      }

      this.commandDomains.push(...Object.keys(_commands));

      if (this._lastCommandName !== '') {
        for (let domain of this.commandDomains) {
          for (let command of _commands[domain]) {
            let cmd = command.console ? command.console : command.cmd;
            if (this._lastCommandName === cmd) {
              this._lastCommandName;
              this._lastCommand = command;
              break;
            }
          }
        }
      }
    }

    if (data.consoleCommands) {
      this._consoleCommands = data.consoleCommands;
      this._consoleCommandNames = this._consoleCommands.map(v => v.name + ' x'.repeat(v.minArgs - 1) + ' [x]'.repeat(v.maxArgs - v.minArgs));
    }

    for (let listener of this._listeners) {
      listener.onMessage(data);
    }
  }

  continueUpdate(force = false) {
    if (force) {
      if (this._updateTimeout != null)
        clearTimeout(this._updateTimeout);
      this._updateTimeout = null;
    }

    if (this._updateTimeout === null)
      this._updateTimeout = setTimeout(() => { this.sendCommand('update'); }, UPDATE_INTERVAL_MS);
  }

  processOpen() {
    this.onOpen();
    this.continueUpdate(true);
  }

  processMessage(observer, message) {
    if (typeof message === 'object' && message.constructor && message.constructor.name === 'Blob') {
      let reader = new FileReader();
      reader.addEventListener("loadend", () => {
        observer.next(deserializeBSON(new Uint8Array(reader.result)));
        this.continueUpdate();
      });
      reader.readAsArrayBuffer(message);
    }
    else {
      let json = {};
      try {
        json = JSON.parse(message);
      }
      catch (err) {
        json = { log: message };
      }
      observer.next(json);
    }
  }

  processValue(data) {
    this.onMessage(data);

    if (data._update) {
      if (this._updateTimeout !== null)
        clearTimeout(this._updateTimeout);
      this._updateTimeout = null;
    }
  }

  sendCommand(cmd, params = null) {
    let tmp = params || {};
    tmp.cmd = cmd;
    this.streamService.send(tmp);
  }

  pollData(dataGetter, cmd, params = null) {
    return new Promise((resolve, reject) => {
      let isDataValid = d => { return (typeof (d.length) !== undefined && d.length > 0) || (typeof (d.length) === undefined && d); }

      let data = dataGetter();
      if (isDataValid(data)) {
        resolve(data);
        return;
      }

      this.sendCommand(cmd, params);
      let pollId = global.setInterval(() => {
        let data = dataGetter();
        if (isDataValid(data)) {
          global.clearInterval(pollId);
          resolve(data);
        }
      }, 100);
    });
  }

  runConsoleRaw(data) {
    let cmd;
    let params = {};
    let args = data.split(' ');
    cmd = args[0];
    for (let i = 1; i < args.length; ++i) {
      params['arg' + (i - 1)] = args[i];
    }
    this.runConsole(cmd, params);
  }

  runConsole(cmd, params = null) {
    let tmp = params || {};
    tmp.command = cmd;
    this.sendCommand('console', tmp);
  }

  runConsoleScript(commands) {
    for (let cmd of commands) {
      this.runConsole(cmd[0], cmd[1] ? cmd[1] : null);
    }
  }

  buildAndSendCommand(command) {
    this._lastCommandName = command.console ? command.console : command.cmd;
    this._lastCommand = command;
    this.cookieService.put("_lastCommandName", this._lastCommandName);

    if (command.console) {
      let consoleStr = command.console;

      for (let p of command.params) {
        if (p.value === undefined)
          continue;

        if (Array.isArray(p.value))
          consoleStr += ' ' + p.value.concat(' ');
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

    for (let p of command.params) {
      if (p.value === undefined)
        continue;

      let type = p.type;
      if (['dmPart', 'enum', 'b'].indexOf(p.type) >= 0) {
        params[p.name] = p.value;
      }
      else {
        params[p.name] = {};
        params[p.name][type] = p.value;
      }
    }

    console.log(command, params);

    this.sendCommand(command.cmd, params);
  }

  searchConsoleCommand(query) {
    return query.length < 2 ? [] : this._consoleCommandNames.filter(v => v.toLowerCase().indexOf(query.toLowerCase()) > -1).slice(0, 10);
  }

  get isConnected() { return this.streamService.isConnected(); };

  get updateCount() { return +this._updateCount; }

  set messagesObservable(value) { this._messagesObservable = value; }
  get messagesObservable() { return this._messagesObservable; }

  get observable() { return this._observable; }

  get consoleCommands() { return this._consoleCommands; }
  get consoleCommandNames() { return this._consoleCommandNames; }

  get lastCommand() { return this._lastCommand; }
}