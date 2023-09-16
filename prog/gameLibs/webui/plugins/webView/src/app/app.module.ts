import { NgModule } from '@angular/core';
import { BrowserModule } from '@angular/platform-browser';

import { CoreModule } from './core/core.module';
import { AllModule } from './modules/all.module';

import { AppComponent } from './app.component';

import { NgbModule } from '@ng-bootstrap/ng-bootstrap';
import { CookieModule } from 'ngx-cookie';

@NgModule({
  imports: [BrowserModule, NgbModule.forRoot(), CoreModule.forRoot(/*{userName: 'Miss Marple'}*/), CookieModule.forRoot(), AllModule],
  declarations: [AppComponent],
  bootstrap: [AppComponent]
})
export class AppModule { }
