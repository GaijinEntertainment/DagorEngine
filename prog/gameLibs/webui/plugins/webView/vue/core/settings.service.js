const { ajax } = rxjs.ajax;
const { map, of, catchError } = rxjs;

export class Settings {
  constructor(port) {
    this.port = port;
  }
}

export class SettingsService {
  _defaultSettings = new Settings(9113);
  _settings = null;

  _settingsPromise = null;

  getSettings() {
    if (this._settingsPromise !== null) {
      return this._settingsPromise;
    }

    this._settingsPromise = new Promise((resolve, reject) => {
      let onDone = (data) => {
        console.log('Settings', data);

        this._settingsPromise = null;
        this._settings = data;
        resolve(this._settings);
      };

      if (this._settings !== null) {
        resolve(this._settings);
      }
      else {
        const obs$ = ajax(`${location.href}/settings.json`).pipe(
        // const obs$ = ajax(`http://127.0.0.1:23456/webview/settings.json`).pipe(
          map(data => data.response),
          catchError(error => {
            console.log('error: ', error);
            return of(error);
          })
        );

        obs$.subscribe({
          next: value => onDone(value),
          error: err => console.log(err)
        });
      }
    });

    return this._settingsPromise;
  }
}