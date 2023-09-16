import { NgModule, OnInit, OnDestroy, Directive, ElementRef, EventEmitter, Input, Output, NgZone } from '@angular/core';
import { CommonModule } from '@angular/common';
import { FormsModule } from '@angular/forms';

import { NgbModule } from '@ng-bootstrap/ng-bootstrap';
import { TreeModule } from 'angular-tree-component';

import * as Clipboard from 'clipboard';

@Directive({
  selector: '[ngIIclipboard]'
})
export class ClipboardDirective implements OnInit, OnDestroy
{
  clipboard: Clipboard;

  @Input('ngIIclipboard') targetElm: ElementRef;

  @Input() cbContent: string;

  @Output('cbOnSuccess') onSuccess: EventEmitter<boolean> = new EventEmitter<boolean>();

  @Output('cbOnError') onError: EventEmitter<boolean> = new EventEmitter<boolean>();

  constructor(private elmRef: ElementRef) { }

  ngOnInit()
  {
    let option: Clipboard.Options;
    option = !!this.targetElm ? { target: () => <any>this.targetElm } : { text: () => this.cbContent };
    this.clipboard = new Clipboard(this.elmRef.nativeElement, option);
    this.clipboard.on('success', () => this.onSuccess.emit(true));
    this.clipboard.on('error', () => this.onError.emit(true));
  }

  ngOnDestroy()
  {
    !!this.clipboard && this.clipboard.destroy();
  }
}

@NgModule({
  imports: [CommonModule, FormsModule, NgbModule, TreeModule],
  declarations: [ClipboardDirective],
  exports: [CommonModule, FormsModule, TreeModule, ClipboardDirective]
})
export class SharedModule { }