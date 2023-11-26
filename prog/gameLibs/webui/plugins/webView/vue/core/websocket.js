const Observable = rxjs.Observable;
const Subject = rxjs.Subject;

export class $WebSocket {
  static Helpers = class {
    static isPresent(obj) {
      return obj !== undefined && obj !== null;
    }

    static isString(obj) {
      return typeof obj === 'string';
    }

    static isArray(obj) {
      return Array.isArray(obj);
    }

    static isFunction(obj) {
      return typeof obj === 'function';
    }
  };

  reconnectAttempts = 0;
  sendQueue = [];
  onOpenCallbacks = [];
  onMessageCallbacks = [];
  onErrorCallbacks = [];
  onCloseCallbacks = [];
  readyStateConstants = {
    'CONNECTING': 0,
    'OPEN': 1,
    'CLOSING': 2,
    'CLOSED': 3,
    'RECONNECT_ABORTED': 4
  };
  normalCloseCode = 1000;
  reconnectableStatusCodes = [4000];
  socket;
  dataStream;
  internalConnectionState;

  constructor(url, protocols = undefined, config = undefined, binaryType = undefined) {
    this.url = url;
    this.protocols = protocols;
    this.config = config;
    this.binaryType = binaryType;

    let match = new RegExp('wss?:\/\/').test(url);
    if (!match) {
      throw new Error('Invalid url provided');
    }
    this.config = config || { initialTimeout: 500, maxTimeout: 300000, reconnectIfNotNormalClose: false };
    this.binaryType = binaryType || "blob";
    this.dataStream = new Subject();
    this.connect(true);
  }

  connect(force = false) {
    // console.log("WebSocket connecting...");
    let self = this;
    if (force || !this.socket || this.socket.readyState !== this.readyStateConstants.OPEN) {
      self.socket = this.protocols ? new WebSocket(this.url, this.protocols) : new WebSocket(this.url);
      self.socket.binaryType = self.binaryType.toString();

      self.socket.onopen = (ev) => {
        // console.log('onOpen: ', ev);
        this.onOpenHandler(ev);
      };
      self.socket.onmessage = (ev) => {
        // console.log('onNext: ', ev.data);
        self.onMessageHandler(ev);
        this.dataStream.next(ev);
      };
      this.socket.onclose = (ev) => {
        // console.log('onClose ', ev);
        self.onCloseHandler(ev);
      };

      this.socket.onerror = (ev) => {
        // console.log('onError ', ev);
        self.onErrorHandler(ev);
        this.dataStream.error(ev);
      };

    }
  }

  /**
   * Run in Block Mode
   * Return true when can send and false in socket closed
   * @param data
   * @returns {boolean}
   */
  send4Direct(data, binary = undefined) {
    let self = this;
    if (this.getReadyState() !== this.readyStateConstants.OPEN
      && this.getReadyState() !== this.readyStateConstants.CONNECTING) {
      this.connect();
    }
    self.sendQueue.push({ message: data, binary: binary });
    if (self.socket.readyState === self.readyStateConstants.OPEN) {
      self.fireQueue();
      return true;
    } else {
      return false;
    }
  }

  /**
   * Return Promise
   * When can Send will resolve Promise
   * When Socket closed will reject Promise
   * @param data
   * @returns {Promise<any>}
   */
  send4Promise(data, binary = undefined) {
    return new Promise(
      (resolve, reject) => {
        if (this.send4Direct(data, binary)) {
          return resolve();
        } else {
          return reject(Error('Socket connection has been closed'));
        }
      }
    )
  }

  /**
   * Return cold Observable
   * When can Send will complete observer
   * When Socket closed will error observer
   * @param data
   * @returns {Observable<any>}
   */
  send4Observable(data, binary = undefined) {
    return Observable.create((observer) => {
      if (this.send4Direct(data, binary)) {
        return observer.complete();
      } else {
        return observer.error('Socket connection has been closed');
      }
    });
  }

  send4Mode = WebSocketSendMode.Observable;

  /**
   * Set send(data) function return mode
   * @param mode
   */
  setSend4Mode(mode) {
    this.send4Mode = mode;
  }

  /**
   * Use {mode} mode to send {data} data
   * If no specify, Default SendMode is Observable mode
   * @param data
   * @param mode
   * @param binary
   * @returns {any}
   */
  send(data, mode = undefined, binary = undefined) {
    switch (typeof mode !== "undefined" ? mode : this.send4Mode) {
      case WebSocketSendMode.Direct:
        return this.send4Direct(data, binary);
      case WebSocketSendMode.Promise:
        return this.send4Promise(data, binary);
      case WebSocketSendMode.Observable:
        return this.send4Observable(data, binary);
      default:
        throw Error("WebSocketSendMode Error.");
    }
  }

  getDataStream() {
    return this.dataStream;
  }

  onOpenHandler(event) {
    this.reconnectAttempts = 0;
    this.notifyOpenCallbacks(event);
    this.fireQueue();
  }

  notifyOpenCallbacks(event) {
    for (let i = 0; i < this.onOpenCallbacks.length; i++) {
      this.onOpenCallbacks[i].call(this, event);
    }
  }

  fireQueue() {
    // console.log("fireQueue()");
    while (this.sendQueue.length && this.socket.readyState === this.readyStateConstants.OPEN) {
      let data = this.sendQueue.shift();

      // console.log("fireQueue: ", data);
      if (data.binary) {
        this.socket.send(data.message);
      } else {
        this.socket.send(
          $WebSocket.Helpers.isString(data.message) ? data.message : JSON.stringify(data.message)
        );
      }
      // data.deferred.resolve();
    }
  }

  notifyCloseCallbacks(event) {
    for (let i = 0; i < this.onCloseCallbacks.length; i++) {
      this.onCloseCallbacks[i].call(this, event);
    }
  }

  notifyErrorCallbacks(event) {
    for (let i = 0; i < this.onErrorCallbacks.length; i++) {
      this.onErrorCallbacks[i].call(this, event);
    }
  }

  onOpen(cb) {
    this.onOpenCallbacks.push(cb);
    return this;
  };

  onClose(cb) {
    this.onCloseCallbacks.push(cb);
    return this;
  }

  onError(cb) {
    this.onErrorCallbacks.push(cb);
    return this;
  };

  onMessage(callback, options = undefined) {
    if (!$WebSocket.Helpers.isFunction(callback)) {
      throw new Error('Callback must be a function');
    }

    this.onMessageCallbacks.push({
      fn: callback,
      pattern: options ? options.filter : undefined,
      autoApply: options ? options.autoApply : true
    });
    return this;
  }

  onMessageHandler(message) {
    let self = this;
    let currentCallback;
    for (let i = 0; i < self.onMessageCallbacks.length; i++) {
      currentCallback = self.onMessageCallbacks[i];
      currentCallback.fn.apply(self, [message]);
    }
  };

  onCloseHandler(event) {
    this.notifyCloseCallbacks(event);
    if ((this.config.reconnectIfNotNormalClose && event.code !== this.normalCloseCode)
      || this.reconnectableStatusCodes.indexOf(event.code) > -1) {
      this.reconnect();
    } else {
      this.sendQueue = [];
      this.dataStream.complete();
    }
  };

  onErrorHandler(event) {
    this.notifyErrorCallbacks(event);
  };

  reconnect() {
    this.close(true);
    let backoffDelay = this.getBackoffDelay(++this.reconnectAttempts);
    // let backoffDelaySeconds = backoffDelay / 1000;
    // console.log('Reconnecting in ' + backoffDelaySeconds + ' seconds');
    setTimeout(() => this.connect(), backoffDelay);
    return this;
  }

  close(force = false) {
    if (force || !this.socket.bufferedAmount) {
      this.socket.close(this.normalCloseCode);
    }
    return this;
  };

  // Exponential Backoff Formula by Prof. Douglas Thain
  // http://dthain.blogspot.co.uk/2009/02/exponential-backoff-in-distributed.html
  getBackoffDelay(attempt) {
    let R = Math.random() + 1;
    let T = this.config.initialTimeout;
    let F = 2;
    let N = attempt;
    let M = this.config.maxTimeout;

    return Math.floor(Math.min(R * T * Math.pow(F, N), M));
  };

  setInternalState(state) {
    if (Math.floor(state) !== state || state < 0 || state > 4) {
      throw new Error('state must be an integer between 0 and 4, got: ' + state);
    }

    this.internalConnectionState = state;

  }

  /**
   * Could be -1 if not initzialized yet
   * @returns {number}
   */
  getReadyState() {
    if (this.socket == null) {
      return -1;
    }
    return this.internalConnectionState || this.socket.readyState;
  }
}

// export interface WebSocketConfig {
//   initialTimeout: number;
//   maxTimeout: number;
//   reconnectIfNotNormalClose;
// }

export const WebSocketSendMode = {
  Direct: 0,
  Promise: 1,
  Observable: 2
}

// export type BinaryType = "blob" | "arraybuffer";