import { Component, OnInit, NgZone } from '@angular/core';

import { StreamService } from './core/stream.service';
import { CoreService } from './core/core.service';

@Component({
  selector: 'my-app',
  templateUrl: './app.component.html',
  styleUrls: ['./app.component.css']
})
export class AppComponent implements OnInit
{
  status: string = "Unknonwn";

  constructor(private streamService: StreamService, public coreService: CoreService, private zone: NgZone)
  {
  }

  ngOnInit(): void
  {
    console.log('AppComponent::OnInit');

    this.zone.runOutsideAngular(() =>
    {
      setInterval(() =>
      {
        let prev = this.status;
        let status = this.streamService.isConnected() ? "Online" : "Offline";
        if (prev !== status)
        {
          this.zone.run(() =>
          {
            this.status = status;
          })
        }
      }, 100);
    });
  }
}