import { Injectable } from '@angular/core';
import { $WebSocket } from 'angular2-websocket/angular2-websocket';

import { Observable } from 'rxjs/Observable';

import * as common from './common';
import { SettingsService } from './settings.service';
import { resolve } from 'path';

var bson = require('../../vendor/bson.js');

export function deserializeBSON(buffer: any): any
{
  let BSON = bson().BSON
  return BSON.deserialize(buffer);
}

@Injectable()
export class StreamService
{
  socket: $WebSocket;

  private _socketPromise: Promise<$WebSocket> = null;

  constructor(public settingsService: SettingsService)
  {
  }

  init(): Promise<$WebSocket>
  {
    if (this._socketPromise === null)
    {
      this._socketPromise = new Promise((resolve, reject) =>
      {
        if (this.socket)
        {
          this._socketPromise = null;
          resolve(this.socket);
          return;
        }

        this.settingsService.getSettings().then(settings =>
        {
          this.socket = new $WebSocket(`ws://localhost:${settings.port}/stream`, null, { initialTimeout: 500, maxTimeout: 5000, reconnectIfNotNormalClose: true });

          this._socketPromise = null;
          resolve(this.socket);
        });
      });
    }
    return this._socketPromise;
  }

  onOpen(callback: Function): void
  {
    this.init().then(socket => socket.onOpen(() => callback()));
  }

  onMessage(callback: Function): void
  {
    this.init().then(socket => socket.onMessage((event: MessageEvent) => callback(event.data), null));
  }

  isConnected(): boolean
  {
    this.init();
    return this.socket && this.socket.getReadyState() == 1;
  }

  send(json: any): void
  {
    if (this.socket === null || this.socket.getReadyState() !== 1)
    {
      return;
    }

    this.socket.send(common.jsonToBLK(json)).subscribe();
  }

  sendString(jsonString: string): void
  {
    if (this.socket === null || this.socket.getReadyState() !== 1)
    {
      return;
    }

    this.socket.send(jsonString).subscribe();
  }
}