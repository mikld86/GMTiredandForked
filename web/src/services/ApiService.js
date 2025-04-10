import { createContext } from 'preact';
import { signal } from '@preact/signals';

function randomId() {
  return Math.random().toString(36).replace(/[^a-z]+/g, '').substr(2, 10);
}

export default class ApiService {
  socket = null;
  listeners = {};

  constructor() {
    console.log("Established websocket connection");
    this.connect();
  }

  connect() {
    const apiHost = window.location.host;
    this.socket = new WebSocket(((window.location.protocol === "https:") ? "wss://" : "ws://") + apiHost + "/ws");
    this.socket.addEventListener("message", (e) => { this._onMessage(e); });
    this.socket.addEventListener("close", () => {
      setTimeout(() => {
        this.connect();
      }, 1000);
    });
    this.socket.addEventListener("error", () => {
      this.socket.close();
      setTimeout(() => {
        this.connect();
      }, 1000);
    });
  }

  _onMessage(event) {
    const message = JSON.parse(event.data);
    const listeners = Object.values(this.listeners[message.tp] || {});
    if (message.tp === 'evt:status') {
      this._onStatus(message);
    }
    for (const listener of listeners) {
      listener(message);
    }
  }

  send(event) {
    this.socket.send(JSON.stringify(event));
  }

  on(type, listener) {
    const id = randomId();
    if (!this.listeners[type]) {
      this.listeners[type] = {};
    }
    this.listeners[type][randomId()] = listener;
    return randomId();
  }

  off(type, id) {
    delete this.listeners[type][id];
  }

  _onStatus(message) {
    const newStatus = {
      currentTemperature: message.ct,
      targetTemperature: message.tt,
      mode: message.m,
      timestamp: new Date(),
    };
    const newValue = {
      ...machine.value,
      status: {
        ...machine.value.status,
        ...newStatus,
      },
      history: [
        ...machine.value.history,
        newStatus,
      ]
    };
    newValue.history = newValue.history.slice(-600);
    machine.value = newValue;
  }
}

export const ApiServiceContext = createContext(null);

export const machine = signal({
  status: {
    currentTemperature: 0,
    targetTemperature: 0,
    mode: 0,
    selectedProfile: ''
  },
  history: []
});
