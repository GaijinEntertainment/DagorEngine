import { ModuleWithProviders, NgModule, Optional, SkipSelf } from '@angular/core';
import { CommonModule } from '@angular/common';

import { StreamService } from './stream.service';
import { CoreService } from './core.service';
import { SettingsService } from './settings.service';
import { HttpClientModule } from '@angular/common/http';

@NgModule({
  imports: [CommonModule, HttpClientModule],
  providers: [StreamService, CoreService, SettingsService]
})
export class CoreModule {
  static forRoot(/*config: UserServiceConfig*/): ModuleWithProviders {
    return {
      ngModule: CoreModule,
      providers: [/*{ provide: UserServiceConfig, useValue: config }*/]
    };
  }

  constructor( @Optional() @SkipSelf() parentModule: CoreModule) {
    if (parentModule) {
      throw new Error('CoreModule is already loaded. Import it in the AppModule only');
    }
  }
}
