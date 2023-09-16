import { Component, OnInit, OnDestroy, Pipe, PipeTransform } from '@angular/core';
import { NgbModal, ModalDismissReasons } from '@ng-bootstrap/ng-bootstrap';

import * as common from '../../core/common';

import { StreamService, deserializeBSON } from '../../core/stream.service';
import { CoreService } from '../../core/core.service';
import { DMService } from '../dm/dm.service';

import { Observable } from 'rxjs/Observable';
import { CookieService } from 'ngx-cookie';

const UPDATE_INTERVAL_MS = 500;

@Component({
  selector: 'editor',
  templateUrl: './editor.component.html',
  styleUrls: ['./editor.component.css']
})
export class EditorComponent extends common.BaseComponent implements OnInit, OnDestroy
{
  showCommands: boolean = false;

  constructor(public coreService: CoreService, public dmService: DMService)
  {
    super(coreService);
  }

  ngOnInit(): void
  {
    super.ngOnInit();

    let tab = this.coreService.cookieService.get('_currentTab');
    if (tab)
    {
      this.currentTab = tab;
      this.onTabChange(tab);
    }

    console.log('EditorComponent::OnInit');
  }

  ngOnDestroy(): void
  {
    super.ngOnDestroy();
  }

  resetTimeSpeed()
  {
    this.coreService.resetTimeSpeed();
  }

  needShowCopyAlert: boolean = false;

  showCopyAlery(): void
  {
    this.needShowCopyAlert = true;
    setTimeout(() => this.needShowCopyAlert = false, 1000);
  }

  needShowSaveAlert: boolean = false;

  showSaveAlert(): void
  {
    this.needShowSaveAlert = true;
    setTimeout(() => this.needShowSaveAlert = false, 5000);
  }

  menuVisible: boolean = false;

  toggleMenu()
  {
    this.menuVisible = !this.menuVisible;
  }

  currentTab: string = 'Main';

  selectTab($event, tab): void
  {
    $event.preventDefault();
    this.currentTab = tab;

    this.coreService.cookieService.put('_currentTab', this.currentTab);

    this.onTabChange(tab);
  }

  onTabChange(tab): void
  {
  }

  setTimeSpeed(speed: number)
  {
    this.sendCommand('setCommonParam', { param: 'timespeed', value: speed });
  }

  toggleCommand(c)
  {
    c._expanded = !c._expanded;
  }

  get updateCount() { return +this.coreService.updateCount; }

  get commonParams() { return this.coreService.commonParams; }

  get commandDomains() { return this.coreService.commandDomains; }
  get commands() { return this.coreService.commands; }

  get consoleCommands() { return this.coreService.consoleCommands; }
  get searchConsoleCommand() { return this.coreService.searchConsoleCommand; }

  get lastCommand() { return this.coreService.lastCommand; }
}