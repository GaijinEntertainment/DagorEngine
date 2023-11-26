import * as common from './common.js';
import { $WebSocket } from './websocket.js';

export class StreamService {
  socket;

  _socketPromise = null;

  constructor(settingsService) {
    this.settingsService = settingsService;
  }

  connect() {
    if (this._socketPromise === null) {
      this._socketPromise = new Promise((resolve, reject) => {
        if (this.socket) {
          this._socketPromise = null;
          resolve(this.socket);
          return;
        }

        this.settingsService.getSettings().then(settings => {
          this.socket = new $WebSocket(`ws://localhost:${settings.port}/stream`, null, { initialTimeout: 500, maxTimeout: 5000, reconnectIfNotNormalClose: true });

          this._socketPromise = null;
          resolve(this.socket);
        });
      });
    }
    return this._socketPromise;
  }

  onOpen(callback) {
    this.connect().then(socket => socket.onOpen(() => callback()));
  }

  onMessage(callback) {
    this.connect().then(socket => socket.onMessage((event) => callback(event.data), null));
  }

  isConnected() {
    this.connect();
    return this.socket && this.socket.getReadyState() == 1;
  }

  send(json) {
    if (this.socket === null || this.socket.getReadyState() !== 1) {
      return;
    }

    this.socket.send(common.jsonToBLK(json)).subscribe();
  }

  sendString(jsonString) {
    if (this.socket === null || this.socket.getReadyState() !== 1) {
      return;
    }

    this.socket.send(jsonString).subscribe();
  }
}