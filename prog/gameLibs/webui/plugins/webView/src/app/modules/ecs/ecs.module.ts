import { NgModule } from '@angular/core';
import { SharedModule } from '../../shared/shared.module';

import { ECSComponent, AttrValuePipe } from './ecs.component';
import { ECSService } from './ecs.service';

import { TreeModule } from 'angular-tree-component';
import { NgbModule, NgbTabsetModule } from '@ng-bootstrap/ng-bootstrap';

import { NgxDatatableModule } from '@swimlane/ngx-datatable';
import { ColorPickerModule } from 'ngx-color-picker';
import { NgJsonEditorModule } from 'ang-jsoneditor' 

@NgModule({
  imports: [SharedModule, TreeModule, NgbModule, NgbTabsetModule, NgxDatatableModule, ColorPickerModule, NgJsonEditorModule],
  declarations: [ECSComponent, AttrValuePipe],
  exports: [ECSComponent],
  providers: [ECSService]
})
export class ECSModule { }
