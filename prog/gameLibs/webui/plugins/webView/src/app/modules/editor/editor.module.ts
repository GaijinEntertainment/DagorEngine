import { NgModule, Pipe, PipeTransform, ChangeDetectionStrategy } from '@angular/core';
import { SharedModule } from '../../shared/shared.module';
import { DMModule } from '../dm/dm.module';
import { ECSModule } from '../ecs/ecs.module';
import { NgbModule, NgbTabsetModule, NgbAccordionModule, NgbTypeaheadModule } from '@ng-bootstrap/ng-bootstrap';

import { EditorComponent } from './editor.component';

@NgModule({
  imports: [SharedModule, DMModule, ECSModule, NgbModule, NgbTabsetModule, NgbAccordionModule, NgbTypeaheadModule],
  declarations: [EditorComponent],
  exports: [EditorComponent]
})
export class EditorModule { }