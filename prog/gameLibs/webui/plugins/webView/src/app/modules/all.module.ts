import { NgModule } from '@angular/core';
import { SharedModule } from '../shared/shared.module';

import { DMModule } from './dm/dm.module';
import { EditorModule } from './editor/editor.module';
import { ECSModule } from './ecs/ecs.module';

@NgModule({
    imports: [SharedModule, DMModule, EditorModule, ECSModule],
    declarations: [],
    exports: [DMModule, EditorModule, ECSModule]
})
export class AllModule { }