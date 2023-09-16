import { Injectable } from '@angular/core';
import { HttpClient } from '@angular/common/http';

export interface Settings
{
  port: number;
}

@Injectable()
export class SettingsService
{
  private _defaultSettings: Settings = { port: 9113 };
  private _settings: Settings = null;

  private _settingsPromise: Promise<Settings> = null;

  constructor(public http: HttpClient)
  {
  }

  getSettings(): Promise<Settings>
  {
    if (this._settingsPromise === null)
    {
      this._settingsPromise = new Promise((resolve, reject) =>
      {
        let onDone = (data: Settings) =>
        {
          console.log('Settings', data);

          this._settingsPromise = null;
          this._settings = data;
          resolve(this._settings);
        };

        if (this._settings !== null)
        {
          resolve(this._settings);
        }
        else
        {
          this.http.get(`${location.href}/settings.json`).subscribe((data: Settings) => onDone(data), (error) => { console.log('Settings error', error); onDone(this._defaultSettings); });
        }
      });
    }
    return this._settingsPromise;
  }
}